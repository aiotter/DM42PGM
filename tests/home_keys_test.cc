// Stub the core bindings so this suite tests the DM42 input and drawing boundary.
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <string>

#include "dmcp_test_support.h"
#include "core_main.h"
#include "core_aux.h"
#include "core_home_menu_custom.h"
#include "dm42_usb_actions_custom.h"

static bool program_mode, menu, hex_menu, alpha_menu, usb_connected;
static int boot_calls, usb_disk_calls, core_calls, received_key, received_no_menu;
static int selected_slot;
static bool selected_shift, returned_queued, returned_running;
static int returned_repeat;
static std::string drawn_labels[MENU_KEY_COUNT];
static constexpr int empty_test_slot = CORE_HOME_KEY_COUNT - 1;

int is_pgm_mode() { return program_mode; }
int usb_powered() { return usb_connected; }
bool core_menu() { return menu; }
bool core_hex_menu() { return hex_menu; }
bool core_alpha_menu() { return alpha_menu; }
void enter_bootloader() { boot_calls++; }
void enter_usb_disk() { usb_disk_calls++; }
void lcd_draw_menu_keys(const char *labels[]) {
  for (int i = 0; i < MENU_KEY_COUNT; i++) drawn_labels[i] = labels[i];
}

static bool core_action(bool *queued, int *repeat) {
  core_calls++;
  *queued = returned_queued;
  *repeat = returned_repeat;
  return returned_running;
}

bool core_keydown_ex(int key, bool *queued, int *repeat, int no_menu_key) {
  received_key = key;
  received_no_menu = no_menu_key;
  return core_action(queued, repeat);
}

core_home_binding core_home_menu_binding(unsigned int slot, bool shifted) {
  assert(slot < CORE_HOME_KEY_COUNT);
  selected_slot = slot;
  selected_shift = shifted;
  if (slot == empty_test_slot) return {};
  // Include two Free42 glyphs that need conversion, including an embedded zero for division.
  static const char label[] = "CORE\000\016";
  return { label, sizeof(label) - 1, core_action };
}

#include "dm42_home_menu_custom.cc"

static void reset() {
  program_mode = menu = hex_menu = alpha_menu = false;
  usb_connected = true;
  boot_calls = usb_disk_calls = core_calls = 0;
  received_key = received_no_menu = selected_slot = -1;
  returned_queued = returned_running = false;
  returned_repeat = 0;
}

static void check_home_input_restrictions() {
  for (bool shift : {false, true}) {
    for (int physical_key = KEY_F1; physical_key <= KEY_F6; physical_key++) {
      for (int scenario = 0; scenario < 4; scenario++) {
        reset();
        const bool actions_allowed = scenario != 0;
        menu = scenario == 1;
        hex_menu = scenario == 2;
        alpha_menu = scenario == 3;
        int key = physical_key;
        bool shifted = shift;
        const auto action = prepare_custom_home_key(key, shifted, actions_allowed);

        assert(action.action == nullptr && selected_slot == -1);
        assert(boot_calls == 0 && usb_disk_calls == 0 && core_calls == 0);
        if (actions_allowed) {
          assert(key == physical_key && shifted == shift);
        } else {
          assert(key == -1 && !shifted);
        }
      }
    }
  }
}

static void check_home_preparation() {
  for (bool program : {false, true}) {
    for (bool shift : {false, true}) {
      for (bool usb : {false, true}) {
        reset();
        program_mode = program;
        usb_connected = usb;
        draw_custom_home_menu(shift);
        assert(boot_calls == 0 && usb_disk_calls == 0 && core_calls == 0);

        int usb_overrides = 0;
        for (int physical_key = KEY_F1; physical_key <= KEY_F6; physical_key++) {
          const int slot = physical_key - KEY_F1;
          selected_slot = -1;
          int key = physical_key;
          bool shifted = shift;
          const auto binding = prepare_custom_home_key(key, shifted, true);
          const bool usb_override = selected_slot == -1;
          const bool empty = !usb_override && slot == empty_test_slot;
          if (usb_override) {
            usb_overrides++;
            assert(drawn_labels[slot] == (shift ? "Boot" : "Disk"));
          } else {
            assert(selected_slot == slot && selected_shift == shift);
            assert(drawn_labels[slot] == (empty ? "" : "CORE\200\216"));
          }
          assert(!shifted && key == (empty ? -1 : physical_key));
          assert(boot_calls == 0 && usb_disk_calls == 0 && core_calls == 0);
          if (empty) {
            assert(binding.action == nullptr && binding.label == nullptr);
            continue;
          }

          // Changes between preparation and dispatch must not reselect the binding.
          program_mode = !program;
          usb_connected = !usb;
          returned_queued = returned_running = true;
          returned_repeat = 2;
          bool queued = false;
          int repeat = -1;
          assert(dispatch_custom_home_key(binding, key, &queued, &repeat, 0) == !usb_override);
          if (usb_override) {
            assert(!queued && repeat == 0 && core_calls == 0);
            assert(boot_calls == shift && usb_disk_calls == !shift);
          } else {
            assert(queued && repeat == 2 && core_calls == 1);
            assert(boot_calls == 0 && usb_disk_calls == 0);
          }
          program_mode = program;
          usb_connected = usb;
          boot_calls = usb_disk_calls = core_calls = 0;
        }
        assert(usb_overrides == (usb && !program ? 1 : 0));
      }
    }
  }
}

int main() {
  check_home_preparation();
  check_home_input_restrictions();

  // Non-HOME input preserves Shift and forwards the native API's input and lifecycle outputs.
  for (bool active : {false, true}) {
    reset();
    bool shifted = true;
    int key = KEY_1;
    const auto binding = prepare_custom_home_key(key, shifted, true);
    assert(key == KEY_1 && shifted && selected_slot == -1);
    bool queued = !active;
    int repeat = -1;
    returned_queued = returned_running = active;
    returned_repeat = active ? 2 : 0;
    int no_menu_key = active ? 1 : 0;
    bool keep_running = dispatch_custom_home_key(binding, key, &queued, &repeat, no_menu_key);
    assert(received_key == KEY_1 && received_no_menu == no_menu_key && core_calls == 1);
    assert(keep_running == active && queued == active && repeat == returned_repeat);
  }
  puts("HOME binding selection and input boundary tests passed");
}
