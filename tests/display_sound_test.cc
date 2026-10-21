#include <cassert>
#include <cstdio>
#include <deque>
#include <initializer_list>

#include "dmcp_test_support.h"

// Use the SDK's display types and service table, substituting only hardware services.
static disp_stat_t title_font = {}, footer_font = {};
static int8_t reg_font_ix, pgm_font_ix;
static int volume;
static int next_font, previous_font;
static bool auto_off;
struct key_event { int key; bool auto_off; };
static std::deque<key_event> keys;

void lcd_clear_buf() {}
void lcd_writeClr(disp_stat_t *) {}
void lcd_setXY(disp_stat_t *, int, int) {}
void lcd_writeText(disp_stat_t *, const char *) {}
void lcd_refresh_wait() {}
void lcd_switchFont(disp_stat_t *, int) {}
int lcd_lineHeight(disp_stat_t *) { return 1; }
int lcd_nextFontNr(int) { return next_font; }
int lcd_prevFontNr(int) { return previous_font; }
int get_beep_volume() { return volume; }
void beep_volume_up() { volume++; }
void beep_volume_down() { volume--; }
void start_buzzer_freq(uint32_t) {}
void stop_buzzer() {}
void sys_delay(uint32_t) {}
void wait_for_key_release(int) {}
static int muted() { return 1; }
int runner_get_key(int *) {
  assert(!keys.empty());
  const key_event event = keys.front();
  keys.pop_front();
  auto_off = event.auto_off;
  return event.key;
}
int is_menu_auto_off() { return auto_off; }

#include "dm42_relocated_main_menu_custom.inc"

static void input(std::initializer_list<key_event> events) {
  keys.assign(events);
  auto_off = false;
}

int main() {
  t20 = &footer_font;
  t24 = &title_font;
  is_beep_mute = muted;

  // Volume preview is committed by Enter and restored by explicit or automatic cancellation.
  volume = 5;
  input({{KEY_UP, false}, {KEY_ENTER, false}});
  relocated_main_menu_adjust_volume();
  assert(volume == 6 && keys.empty());
  input({{KEY_DOWN, false}, {KEY_EXIT, false}});
  relocated_main_menu_adjust_volume();
  assert(volume == 6 && keys.empty());
  input({{KEY_UP, false}, {0, true}});
  relocated_main_menu_adjust_volume();
  assert(volume == 6 && keys.empty());

  // Font adjustment commits the opaque value returned by the SDK without changing the other mode.
  reg_font_ix = 3;
  pgm_font_ix = 2;
  next_font = 6;
  input({{KEY_UP, false}, {KEY_ENTER, false}});
  relocated_main_menu_adjust_font(0);
  assert(reg_font_ix == next_font && pgm_font_ix == 2 && keys.empty());
  previous_font = 1;
  input({{KEY_DOWN, false}, {KEY_ENTER, false}});
  relocated_main_menu_adjust_font(1);
  assert(reg_font_ix == next_font && pgm_font_ix == previous_font && keys.empty());

  // Exit and auto-off discard font previews rather than committing the staged value.
  input({{KEY_UP, false}, {KEY_EXIT, false}});
  relocated_main_menu_adjust_font(1);
  assert(pgm_font_ix == previous_font && keys.empty());
  input({{KEY_DOWN, false}, {0, true}});
  relocated_main_menu_adjust_font(0);
  assert(reg_font_ix == next_font && keys.empty());
  puts("Adjustment commit/cancel tests passed");
}
