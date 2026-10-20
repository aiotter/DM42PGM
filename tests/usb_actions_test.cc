#include <cassert>
#include <cstdio>
#include <deque>

#include "dmcp_test_support.h"
extern "C" {
#include "dm42_fns.h"
}

static disp_stat_t font = {};
static bool connected, disk_ok, disconnect_on_save, disconnect_on_key, auto_off;
static int disconnect_on_release;
static int save_result, saves, flushes, releases, prompts, redraws, errors, mode;
static std::deque<int> keys;

int usb_powered() { return connected; }
void lcd_writeClr(disp_stat_t *) { prompts++; }
void lcd_clear_buf() {}
void lcd_writeText(disp_stat_t *, const char *) {}
void lcd_refresh_wait() {}
void start_buzzer_freq(uint32_t) {}
void sys_delay(uint32_t) {}
void stop_buzzer() {}
void wait_for_key_release(int) {
  releases++;
  if (releases == disconnect_on_release) connected = false;
}
int runner_get_key(int *) {
  assert(releases >= 1 && saves == 0 && flushes == 0);
  assert(!keys.empty());
  int key = keys.front();
  keys.pop_front();
  if (disconnect_on_key) connected = false;
  return key;
}
int is_menu_auto_off() { return auto_off; }
int sys_disk_ok() { return disk_ok; }
int run_menu_item_sys(uint8_t item) {
  assert(connected && saves == 1 && flushes == 2 && releases >= 2);
  assert(ST(STAT_MENU));
  mode = item;
  return 0;
}
void wait_for_key_press() { errors++; }
void prtof_buf_flush(int, int) { flushes++; }
extern "C" int statefile_save() {
  saves++;
  if (disconnect_on_save) connected = false;
  return save_result;
}
void calc_lcd_redraw() { redraws++; }

#include "dm42_usb_actions_custom.cc"

// main.h disables puts for firmware; keep the host test result visible.
#undef puts

static void reset() {
  connected = disk_ok = true;
  disconnect_on_save = disconnect_on_key = auto_off = false;
  disconnect_on_release = 0;
  save_result = saves = flushes = releases = prompts = redraws = errors = mode = 0;
  keys.clear();
  calc_state = 0;
  t24 = &font;
}

int main() {
  for (int action = 0; action < 2; action++) {
    void (*enter)() = action == 0 ? enter_usb_disk : enter_bootloader;
    int expected_mode = action == 0 ? MI_MSC : MI_BOOTLOADER;

    // Both actions ignore other keys and require Enter after releasing the shortcut.
    reset();
    keys.push_back(KEY_F4);
    keys.push_back(KEY_ENTER);
    enter();
    assert(mode == expected_mode && prompts == 1 && keys.empty());
    assert(redraws == 1 && releases == 3 && !ST(STAT_MENU));

    // Exit, auto-off, and a USB disconnect while reading confirmation do not save state.
    for (int scenario = 0; scenario < 3; scenario++) {
      reset();
      keys.push_back(scenario == 0 ? KEY_EXIT : KEY_ENTER);
      auto_off = scenario == 1;
      disconnect_on_key = scenario == 2;
      enter();
      assert(mode == 0 && saves == 0 && flushes == 0 && prompts == 1);
      assert(redraws == 1 && errors == 0 && !ST(STAT_MENU));
    }

    // USB must remain connected through shortcut release and confirmation-key release.
    for (int release = 1; release <= 2; release++) {
      reset();
      keys.push_back(KEY_ENTER);
      disconnect_on_release = release;
      enter();
      assert(mode == 0 && saves == 0 && flushes == 0 && prompts == 1);
      assert(redraws == 1 && errors == 0 && !ST(STAT_MENU));
    }

    // No USB power means no prompt; save failures and disconnection block activation.
    for (int scenario = 0; scenario < 4; scenario++) {
      reset();
      keys.push_back(KEY_ENTER);
      connected = scenario != 0;
      save_result = scenario == 1 ? 1 : 0;
      disk_ok = scenario != 2;
      disconnect_on_save = scenario == 3;
      enter();
      assert(mode == 0 && prompts == (scenario == 0 ? 0 : 1) && !ST(STAT_MENU));
      assert(saves == (scenario == 0 ? 0 : 1));
      assert(errors == ((scenario == 1 || scenario == 2) ? 1 : 0));
    }
  }
  puts("USB action tests passed");
}
