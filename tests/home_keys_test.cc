// Do not test production key bindings against fixed key, label, or action expectations.
// Changing a key binding must not require updating tests. Test binding-independent
// behavior, using separate test bindings when selection rules need verification.

#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "dmcp_test_support.h"
#include "core_main.h"
#include "core_aux.h"
#include "dm42_usb_actions_custom.h"

static bool program_mode, shifted, running, menu, hex_menu, alpha_menu;
static bool usb_connected;
static int usb_disk_calls;
static int is_graphics, boot_calls, core_calls, received_key, received_no_menu;
static bool returned_queued, returned_running;
static int returned_repeat;
static const char *drawn_labels[MENU_KEY_COUNT];

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

static bool result(bool *queued, int *repeat) {
  core_calls++;
  *queued = returned_queued;
  *repeat = returned_repeat;
  return returned_running;
}

bool core_keydown_command(const char *, bool, bool *queued, int *repeat) {
  return result(queued, repeat);
}
bool core_keydown_ex(int key, bool *queued, int *repeat, int no_menu_key) {
  received_key = key;
  received_no_menu = no_menu_key;
  return result(queued, repeat);
}
extern "C" bool core_program_label_jump(bool *q, int *r) { return result(q, r); }
extern "C" bool core_program_follow(bool *q, int *r) { return result(q, r); }
extern "C" bool core_program_find(bool, bool *q, int *r) { return result(q, r); }
extern "C" bool core_repeat_last_command(bool *q, int *r) { return result(q, r); }
extern "C" bool core_undo(bool *q, int *r) { return result(q, r); }
extern "C" bool core_redo(bool *q, int *r) { return result(q, r); }

#include "dm42_home_menu_custom.cc"

static void reset() {
  program_mode = shifted = running = menu = hex_menu = alpha_menu = false;
  usb_connected = true;
  is_graphics = boot_calls = core_calls = 0;
  usb_disk_calls = 0;
  received_key = received_no_menu = -1;
  returned_queued = returned_running = false;
  returned_repeat = 0;
}

static void check_home_input_restrictions() {
  for (bool program : {false, true}) {
    for (bool shift : {false, true}) {
      for (int physical_key = KEY_F1; physical_key <= KEY_F6; physical_key++) {
        // Existing menus retain their input; execution and graphics suppress HOME actions.
        for (int scenario = 0; scenario < 5; scenario++) {
          reset();
          program_mode = program;
          shifted = shift;
          running = scenario == 0;
          is_graphics = scenario == 1 ? 2 : 0;
          menu = scenario == 2;
          hex_menu = scenario == 3;
          alpha_menu = scenario == 4;
          int key = physical_key;
          const custom_key_action *action = prepare_custom_home_key(key, shifted, !running && is_graphics <= 1);

          assert(action == nullptr);
          assert(boot_calls == 0 && usb_disk_calls == 0 && core_calls == 0);
          if (scenario >= 2) {
            assert(key == physical_key);
            assert(shifted == shift);
          } else {
            assert(key < 0);
            assert(!shifted);
          }
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
        shifted = shift;
        usb_connected = usb;
        draw_custom_home_menu(shifted);
        for (const char *label : drawn_labels) assert(label != nullptr);

        for (int physical_key = KEY_F1; physical_key <= KEY_F6; physical_key++) {
          shifted = shift;
          int key = physical_key;
          const custom_key_action *action = prepare_custom_home_key(key, shifted, !running && is_graphics <= 1);
          assert(!shifted);
          assert(key == (action == nullptr ? -1 : physical_key));
          if (action != nullptr) {
            // A later selection must not invalidate or overwrite a prepared action.
            const custom_key_action saved = *action;
            program_mode = !program;
            usb_connected = !usb;
            bool other_shifted = !shift;
            int other_key = physical_key;
            prepare_custom_home_key(other_key, other_shifted, true);
            assert(action->kind == saved.kind);
            if (saved.kind == custom_key_action::Kind::Command)
              assert(std::strcmp(action->command_name, saved.command_name) == 0);
            else
              assert(action->operation_id == saved.operation_id);
            program_mode = program;
            usb_connected = usb;
          }
        }
        // Drawing and preparation must defer all operations to dispatch.
        assert(boot_calls == 0 && usb_disk_calls == 0 && core_calls == 0);
      }
    }
  }
}

static void check_shift_precedence() {
  using Condition = key_binding::Condition;
  // Put Shift bindings first to verify priority does not depend on table order.
  const key_binding bindings[] = {
    { KEY_F1, key_binding::Condition::Shifted | key_binding::Condition::UsbConnected, "shift+usb", custom_key_action::operation(custom_key_action::Operation::Redo) },
    { KEY_F1, key_binding::Condition::None, "fallback", custom_key_action::operation(custom_key_action::Operation::Undo) },
    { KEY_F2, key_binding::Condition::Shifted, "shift", custom_key_action::operation(custom_key_action::Operation::Redo) },
    { KEY_F2, key_binding::Condition::None, "fallback", custom_key_action::operation(custom_key_action::Operation::Undo) }
  };
  for (bool shift : {false, true}) {
    for (bool usb : {false, true}) {
      reset();
      unsigned int active_conditions = shift ? Condition::Shifted : Condition::None;
      if (usb) active_conditions |= Condition::UsbConnected;
      // Selection must use the supplied conditions even when device state differs.
      shifted = !shift;
      usb_connected = !usb;
      home_bindings selected = home_bindings::select(bindings, active_conditions);
      assert(std::strcmp(selected.keys[0]->label, shift && usb ? "shift+usb" : "fallback") == 0);
      assert(std::strcmp(selected.keys[1]->label, shift ? "shift" : "fallback") == 0);
      for (int i = 2; i < MENU_KEY_COUNT; i++) assert(selected.keys[i] == nullptr);
    }
  }
}

static void check_execute_results() {
  using Action = custom_key_action;
  // Test common dispatch and return-value handling with one command and one operation.
  // Do not test each delegated core function here; its behavior belongs in Free42 tests.
  for (const Action &action : {Action::command("COMB"), Action::operation(Action::Operation::Undo)}) {
    for (bool active : {false, true}) {
      reset();
      returned_queued = returned_running = active;
      returned_repeat = active ? 2 : 0;
      bool queued = !active;
      int repeat = -1;
      assert(dispatch_custom_home_key(&action, -1, &queued, &repeat, 0) == active);
      assert(queued == active && repeat == returned_repeat && core_calls == 1);
      assert(boot_calls == 0 && usb_disk_calls == 0);
    }
  }

  // USB callbacks return after cancellation or leaving disk mode, without core key-hold state.
  for (Action::Operation operation : {Action::Operation::Bootloader, Action::Operation::UsbDisk}) {
    reset();
    returned_queued = returned_running = true;
    returned_repeat = 2;
    bool queued = true;
    int repeat = -1;
    assert(!Action::operation(operation).execute(&queued, &repeat));
    assert(!queued && repeat == 0 && core_calls == 0);
    assert(boot_calls == (operation == Action::Operation::Bootloader ? 1 : 0));
    assert(usb_disk_calls == (operation == Action::Operation::UsbDisk ? 1 : 0));
  }
}

int main() {
  check_execute_results();
  check_home_preparation();
  check_shift_precedence();
  check_home_input_restrictions();

  // Non-HOME input preserves Shift and forwards the native API's input and lifecycle outputs.
  for (bool active : {false, true}) {
    reset();
    shifted = true;
    int key = KEY_1;
    const custom_key_action *action = prepare_custom_home_key(key, shifted, !running && is_graphics <= 1);
    assert(key == KEY_1 && shifted);
    bool queued = !active;
    int repeat = -1;
    returned_queued = returned_running = active;
    returned_repeat = active ? 2 : 0;
    int no_menu_key = active ? 1 : 0;
    bool keep_running = dispatch_custom_home_key(action, key, &queued, &repeat, no_menu_key);
    assert(received_key == KEY_1 && received_no_menu == no_menu_key && core_calls == 1);
    assert(keep_running == active && queued == active && repeat == returned_repeat);
  }
  puts("HOME binding selection and input boundary tests passed");
}
