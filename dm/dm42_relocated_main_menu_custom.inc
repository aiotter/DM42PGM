/* Settings relocated from the main menu's F-keys to Setup.
 * Included by dm42_fns.cc beside the register/font implementation.
 */
/* Preview and commit/cancel handling for the relocated display and sound settings. */
static const char *relocated_main_menu_font_name(int font) {
  return font < 0 ? "HP" : font >= 0x10 ? "Mono" : "Sans";
}

static disp_stat_t relocated_main_menu_font_state(int font) {
  disp_stat_t ds = {};
  lcd_switchFont(&ds, font);
  ds.fixed = ds.bgfill = ds.lnfill = ds.newln = 1;
  return ds;
}

static int relocated_main_menu_font_height(int font) {
  disp_stat_t ds = relocated_main_menu_font_state(font);
  return lcd_lineHeight(&ds);
}

static void relocated_main_menu_adjust_header(const char *title, int value, int font) {
  lcd_clear_buf();
  lcd_writeClr(t24);
  lcd_putsR(t24, title);
  char text[48];
  if (font)
    snprintf(text, sizeof(text), "%s: %d px", relocated_main_menu_font_name(value), relocated_main_menu_font_height(value));
  else
    snprintf(text, sizeof(text), "Volume: %d", value);
  lcd_setXY(t24, 0, 40);
  lcd_puts(t24, text);

  lcd_writeClr(t20);
  lcd_setXY(t20, 0, 194);
  lcd_puts(t20, "UP/DOWN: Adjust");
  lcd_setXY(t20, 0, 216);
  lcd_puts(t20, "ENTER: Apply  EXIT: Cancel");
}

static void relocated_main_menu_volume_restore(int original) {
  while (get_beep_volume() != original) {
    int before = get_beep_volume();
    if (before < original) beep_volume_up(); else beep_volume_down();
    if (get_beep_volume() == before) break;
  }
}

static void relocated_main_menu_adjust_volume() {
  const int original = get_beep_volume();
  wait_for_key_release(-1);
  for (;;) {
    relocated_main_menu_adjust_header("Volume", get_beep_volume(), 0);
    lcd_refresh_wait();
    int key = runner_get_key(NULL);
    if (is_menu_auto_off() || IS_EXIT_KEY(key)) {
      relocated_main_menu_volume_restore(original);
      break;
    }
    if (key == KEY_ENTER) break;
    if (key == KEY_UP || key == KEY_DOWN) {
      if (key == KEY_UP) beep_volume_up(); else beep_volume_down();
      // Preview volume without invoking the core's beeper, which redraws the calculator.
      if (!is_beep_mute()) {
        start_buzzer_freq(2000000);
        sys_delay(50);
        stop_buzzer();
      }
    }
  }
  wait_for_key_release(-1);
}

static void relocated_main_menu_adjust_font(int program) {
  int font = program ? pgm_font_ix : reg_font_ix;
  wait_for_key_release(-1);
  for (;;) {
    relocated_main_menu_adjust_header(program ? "Program Font Size" : "Stack Font Size", font, 1);
    disp_stat_t sample = relocated_main_menu_font_state(font);
    sample.y_top_grd = 78;
    lcd_setXY(&sample, 0, 78);
    lcd_puts(&sample, program ? "01 LBL \"DEMO\"" : "Y: 123.456");
    lcd_puts(&sample, program ? "02 123.456" : "X: -789.012");
    lcd_refresh_wait();
    int key = runner_get_key(NULL);
    if (is_menu_auto_off() || IS_EXIT_KEY(key)) break;
    if (key == KEY_ENTER) {
      // Keep the other mode's font and the state-file encoding unchanged.
      if (program) pgm_font_ix = font; else reg_font_ix = font;
      break;
    }
    if (key == KEY_UP) font = lcd_nextFontNr(font);
    if (key == KEY_DOWN) font = lcd_prevFontNr(font);
  }
  wait_for_key_release(-1);
}

// Host adjustment tests omit routing that depends on frontend display state.
#ifndef CUSTOM_TEST
#include "dm42_relocated_main_menu_custom.h"

enum {
  MI_VOLUME = 80,
  MI_STACK_ALIGNMENT,
  MI_STACK_FONT,
  MI_STACK_FONT_SIZE,
  MI_PROGRAM_FONT,
  MI_PROGRAM_FONT_SIZE,
  MI_HOME_MENU
};

static const uint8_t mid_relocated_main_menu[] = {
  MI_VOLUME, MI_STACK_ALIGNMENT, MI_STACK_FONT, MI_STACK_FONT_SIZE,
  MI_PROGRAM_FONT, MI_PROGRAM_FONT_SIZE, MI_HOME_MENU, 0
};

static const smenu_t MID_RELOCATED_MAIN_MENU = {
  "Display & Sound", mid_relocated_main_menu, NULL, NULL
};

int run_relocated_main_menu_item(uint8_t line_id) {
  switch (line_id) {
    case MI_HELP:
      start_help();
      return MRET_EXIT;
    case MI_RELOCATED_MAIN_MENU:
      return handle_menu(&MID_RELOCATED_MAIN_MENU, MENU_ADD, 0);
    case MI_VOLUME: relocated_main_menu_adjust_volume(); break;
    case MI_STACK_ALIGNMENT:
      draw_reg_mode = (draw_reg_mode + 1) % (DRAW_REG_MODE_MAX + 1);
      break;
    case MI_STACK_FONT: reg_font_ix = lcd_toggleFontT(reg_font_ix); break;
    case MI_STACK_FONT_SIZE: relocated_main_menu_adjust_font(0); break;
    case MI_PROGRAM_FONT: pgm_font_ix = lcd_toggleFontT(pgm_font_ix); break;
    case MI_PROGRAM_FONT_SIZE: relocated_main_menu_adjust_font(1); break;
    case MI_HOME_MENU: set_disp_main_menu(!is_disp_main_menu()); break;
    default: return MRET_UNIMPL;
  }
  return 0;
}

const char *relocated_main_menu_line_str(uint8_t line_id, char *s, int slen) {
  switch (line_id) {
    case MI_HELP: return "Help";
    case MI_RELOCATED_MAIN_MENU: return "Display & Sound >";
    case MI_VOLUME:
      snprintf(s, slen, "Volume > [%d]", get_beep_volume()); break;
    case MI_STACK_ALIGNMENT:
      snprintf(s, slen, "Stack Alignment [%s]", is_REG_ALIGN_LEFT ? "Left"
               : is_REG_LINES ? "Right+Lines" : "Right"); break;
    case MI_STACK_FONT:
      snprintf(s, slen, "Stack Font [%s]", relocated_main_menu_font_name(reg_font_ix)); break;
    case MI_STACK_FONT_SIZE:
      snprintf(s, slen, "Stack Font Size > [%dpx]", relocated_main_menu_font_height(reg_font_ix)); break;
    case MI_PROGRAM_FONT:
      snprintf(s, slen, "Program Font [%s]", relocated_main_menu_font_name(pgm_font_ix)); break;
    case MI_PROGRAM_FONT_SIZE:
      snprintf(s, slen, "Program Font Size > [%dpx]", relocated_main_menu_font_height(pgm_font_ix)); break;
    case MI_HOME_MENU:
      snprintf(s, slen, "Home Menu [%s]", is_disp_main_menu() ? "Shown" : "Hidden"); break;
    default: return NULL;
  }
  return s;
}
#endif // CUSTOM_TEST
