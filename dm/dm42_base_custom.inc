/* Local BASE rendering extension, included after the register/font primitives. */
static const char *base_label(int base) {
  return base == 2 ? "2" : base == 8 ? "8" : base == 10 ? "10" : "16";
}

static int base_label_scale(disp_stat_t *ds) {
  return lcd_baseHeight(ds) >= 20 ? 2 : 1;
}

// Reserve the bitmap label and its gap before shortening an editable number.
static int base_value_columns(disp_stat_t *ds, int columns, int base) {
  if (!base) return columns;
  const int reserved = (strlen(base_label(base)) * 4 + 1) * base_label_scale(ds);
  const int width = lcd_fontWidth(ds);
  return columns - (reserved + width - 1) / width;
}

// Draw 3x5 digits, scaled for the main value; these also fit the smallest auxiliary font.
static void draw_base_label(disp_stat_t *ds, int x, int base) {
  static const unsigned char glyphs[5][5] = {
    {7, 5, 5, 5, 7}, // 0
    {2, 6, 2, 2, 7}, // 1
    {7, 1, 7, 4, 7}, // 2
    {7, 4, 7, 5, 7}, // 6
    {7, 5, 7, 5, 7}  // 8
  };
  const int scale = base_label_scale(ds);
  int bottom = ds->y + lcd_baseHeight(ds) + scale * 2;
  const int line_bottom = ds->y + lcd_lineHeight(ds) - 1;
  if (bottom > line_bottom) bottom = line_bottom;
  if (is_REG_LINES && bottom > ds->y + lcd_baseHeight(ds) + 1)
    bottom = ds->y + lcd_baseHeight(ds) + 1;
  const int top = bottom - 5 * scale + 1;
  const char *label = base_label(base);
  for (; *label; label++, x += 4 * scale) {
    const int digit = *label == '0' ? 0 : *label == '1' ? 1 : *label == '2' ? 2
                    : *label == '6' ? 3 : 4;
    for (int row = 0; row < 5; row++)
      for (int col = 0; col < 3; col++)
        if (glyphs[digit][row] & (4 >> col)) {
          const int px = x + col * scale, py = top + row * scale;
          if (px >= 0 && px + scale <= LCD_X && py >= ds->y_top_grd
              && py + scale <= LCD_Y)
            lcd_fill_rect(px, py, scale, scale, ds->inv ? 0 : 1);
        }
  }
}

// Reserve space beside the numeric value for the lower-right base label.
static int draw_base_value(disp_stat_t *ds, const char *text, int base, int align_right) {
  if (!base) return 0;
  // With no value, keep the selected radix at the right margin in either alignment mode.
  if (!text[0]) align_right = 1;
  const int scale = base_label_scale(ds);
  const int label_width = (strlen(base_label(base)) * 4 - 1) * scale;
  const int gap = 2 * scale;
  int right = LCD_X - ds->xoffs;
  if (right > LCD_X) right = LCD_X;
  const int width = right - ds->x - label_width - gap;
  if (width <= 0) return 0;
  char value[128];
  strncpy(value, text, sizeof(value) - 1);
  value[sizeof(value) - 1] = 0;
  disp_stat_t number = *ds;
  number.lnfill = number.newln = 0;
  int text_width = lcd_textWidth(&number, value);
  if (text_width > width) text_width = width;
  lcd_textToBox(&number, ds->x, width, value, 0, align_right);
  // Position independently of whether lcd_textToBox advances the caller's cursor.
  const int label_x = align_right ? right - label_width : ds->x + text_width + gap;
  draw_base_label(ds, label_x, base);
  ds->x = label_x + label_width;
  return 1;
}

// Reduce toward half the X register's line height, stopping at the smallest font.
static void alternate_reg_font(disp_stat_t *ds) {
  set_reg_font(get_reg_font_offset(LINE_REG_X));
  // Keep X's right margin even when switching to a smaller auxiliary font.
  const int right_margin = fReg->xoffs > 0 ? fReg->xoffs : 0;
  *ds = *fReg;
  const int target_height = lcd_lineHeight(ds) / 2;
  int font = current_font_nr();
  const int offset = get_reg_font_offset(LINE_REG_X);
  for (int i = 0; i < abs(offset); i++)
    font = offset < 0 ? lcd_prevFontNr(font) : lcd_nextFontNr(font);
  while (lcd_lineHeight(ds) > target_height) {
    const int smaller = lcd_prevFontNr(font);
    if (smaller == font) break;
    font = smaller;
    lcd_switchFont(ds, font);
  }
  ds->fixed = 1;
  ds->lnfill = ds->newln = ds->inv = 0;
  ds->xoffs = right_margin;
  set_reg_font(0);
}

// Keep the input's trailing digits and cursor when shortening it from the left.
static void format_reg_input(char *dst, int size, int columns) {
  if (size <= 0) return;
  char input[EDIT_LINE_LEN + 2];
  snprintf(input, sizeof(input), "%s_", edit_line);
  if (base_display_base() == 16)
    for (char *p = input; *p; p++)
      if (*p >= 'A' && *p <= 'F') *p += 'a' - 'A';
  const int length = strlen(input);
  if (columns >= size) columns = size - 1;
  if (columns < 0) columns = 0;
  if (length <= columns) {
    memcpy(dst, input, length + 1);
  } else if (columns > 0) {
    dst[0] = '\x9a';
    const int tail = columns - 1;
    memcpy(dst + 1, input + length - tail, tail);
    dst[columns] = 0;
  } else {
    memcpy(dst, input, columns);
    dst[columns] = 0;
  }
}

static bool custom_draw_base_input() {
  const int base = base_display_base();
  if (!base) return false;
  const int plen = strlen(edit_prompt);
  const int linelen = (LCD_X + fReg->xoffs) / lcd_fontWidth(fReg);
  char text[linelen + 2];
  format_reg_input(text, sizeof(text), base_value_columns(fReg, linelen - plen, base));
  disp_aligned_line(edit_prompt, text, base);
  return true;
}

struct alternate_reg_line;
static alternate_reg_line *active_alternate_line = NULL;

struct alternate_reg_line {
  disp_stat_t font;
  char text[128];
  int length;
  int base;
  int visible;
  bool separator_pending;

  // Limit suppression to this stack drawing scope, including when MAIN is not refreshed.
  alternate_reg_line() : length(0), base(0), visible(0), separator_pending(false),
                         previous(active_alternate_line) {
    active_alternate_line = this;
  }
  ~alternate_reg_line() {
    active_alternate_line = previous;
  }

private:
  alternate_reg_line *previous;
  alternate_reg_line(const alternate_reg_line &) = delete;
  alternate_reg_line &operator=(const alternate_reg_line &) = delete;
};

static bool custom_register_separator_allowed() {
  if (!active_alternate_line || !active_alternate_line->separator_pending) return true;
  // Skip the separator between the bottom register and the alternate pane.
  // Registers are drawn bottom-up, so this is the first separator.
  active_alternate_line->separator_pending = false;
  return false;
}

static int prepare_alternate_reg_line(alternate_reg_line *line, int bottom) {
  alternate_reg_font(&line->font);
  int capacity = (LCD_X - line->font.xoffs) / lcd_fontWidth(&line->font) + 1;
  if (capacity > (int) sizeof(line->text)) capacity = sizeof(line->text);
  line->length = altbase2str(line->text, capacity, &line->base);
  line->visible = altbase_visible();
  line->separator_pending = line->visible != 0;
  if (!line->visible) return bottom;
  const int top = bottom - lcd_lineHeight(&line->font) - 2;
  line->font.x = 0;
  line->font.y = top + 2;
  line->font.lnfill = line->font.newln = 0;
  return top;
}

static void draw_alternate_reg_line(alternate_reg_line *line) {
  if (!line->visible) return;
  const int len = hp2font(line->text, line->text, line->length);
  line->text[len] = 0;
  if (!draw_base_value(&line->font, line->text, line->base, !is_REG_ALIGN_LEFT))
    lcd_textToBox(&line->font, 0, LCD_X - line->font.xoffs, line->text, 0, !is_REG_ALIGN_LEFT);
}
