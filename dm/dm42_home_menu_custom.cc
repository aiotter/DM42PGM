#include <stddef.h>

#include "dm42_home_menu_custom.h"
#include "dm42_usb_actions_custom.h"
#include "core_main.h"
#include "core_aux.h"
#include "core_history_custom.h"
#include "core_program_tools_custom.h"

extern "C" {
#include "dmcp.h"
}

struct custom_key_action {
  enum class Operation : int {
    LabelJump,
    Bootloader,
    UsbDisk,
    Follow,
    Find,
    FindReverse,
    Repeat,
    Undo,
    Redo
  };

  enum class Kind { Command, Operation };
  Kind kind;

  union {
    const char *command_name;
    Operation operation_id;
  };

  constexpr explicit custom_key_action(const char *name)
      : kind(Kind::Command), command_name(name) {}
  constexpr explicit custom_key_action(Operation operation)
      : kind(Kind::Operation), operation_id(operation) {}

  static constexpr custom_key_action command(const char *name) {
    return custom_key_action(name);
  }

  static constexpr custom_key_action operation(Operation operation_id) {
    return custom_key_action(operation_id);
  }

  bool execute(bool *enqueued, int *repeat) const;
};

struct key_binding {
  enum Condition : unsigned int {
    None = 0,
    Shifted = 1u << 0,
    UsbConnected = 1u << 1
  };

  int key;
  unsigned int conditions;
  const char *label;
  custom_key_action action;

  bool has_condition(Condition condition) const {
    return (conditions & condition) != 0;
  }

  bool matches(unsigned int active_conditions) const {
    return (active_conditions & conditions) == conditions;
  }
};

using Condition = key_binding::Condition;

struct home_bindings {
  const key_binding *keys[MENU_KEY_COUNT];

  template <size_t N>
  static home_bindings select(const key_binding (&bindings)[N], unsigned int active_conditions) {
    home_bindings selected = {};
    for (const key_binding &binding : bindings) {
      bool requires_shift = binding.has_condition(Condition::Shifted);
      if (!binding.matches(active_conditions)) continue;
      const key_binding *&current = selected.keys[binding.key - KEY_F1];
      // A matching Shift binding takes precedence regardless of table order.
      if (current == nullptr || requires_shift)
        current = &binding;
    }
    return selected;
  }

  void draw() const {
    const char *labels[MENU_KEY_COUNT] = {};
    for (int i = 0; i < MENU_KEY_COUNT; i++) {
      const key_binding *binding = keys[i];
      labels[i] = binding != nullptr ? binding->label : "";
    }
    lcd_draw_menu_keys(labels);
  }

  const custom_key_action *action_for_key(int key) const {
    if (key < KEY_F1 || key > KEY_F6) return nullptr;
    const key_binding *binding = keys[key - KEY_F1];
    return binding != nullptr ? &binding->action : nullptr;
  }
};

bool custom_key_action::execute(bool *enqueued, int *repeat) const {
  if (kind == Kind::Command)
    return core_keydown_command(command_name, false, enqueued, repeat);
  switch (operation_id) {
    case Operation::Bootloader: enter_bootloader(); break;
    case Operation::UsbDisk: enter_usb_disk(); break;
    case Operation::LabelJump: return core_program_label_jump(enqueued, repeat);
    case Operation::Follow: return core_program_follow(enqueued, repeat);
    case Operation::Find: return core_program_find(false, enqueued, repeat);
    case Operation::FindReverse: return core_program_find(true, enqueued, repeat);
    case Operation::Repeat: return core_repeat_last_command(enqueued, repeat);
    case Operation::Undo: return core_undo(enqueued, repeat);
    case Operation::Redo: return core_redo(enqueued, repeat);
    default: break;
  }
  *enqueued = false;
  *repeat = 0;
  return false;
}

using Operation = custom_key_action::Operation;
constexpr auto command = custom_key_action::command;
constexpr auto operation = custom_key_action::operation;

// Bindings without a Shift condition also apply while Shift is active.
static const key_binding normal_home_keys[] = {
  { KEY_F1, Condition::None, "%", command("%") },
  { KEY_F2, Condition::None, "nCr", command("COMB") },
  { KEY_F2, Condition::Shifted, "nPr", command("PERM") },
  { KEY_F3, Condition::None, "TIME", command("T.CALC") },
  { KEY_F4, Condition::UsbConnected, "Disk", operation(Operation::UsbDisk) },
  { KEY_F4, Condition::Shifted | Condition::UsbConnected, "Boot", operation(Operation::Bootloader) },
  { KEY_F5, Condition::None, "AGAIN", operation(Operation::Repeat) },
  { KEY_F6, Condition::None, "UNDO", operation(Operation::Undo) },
  { KEY_F6, Condition::Shifted, "REDO", operation(Operation::Redo) }
};

static const key_binding program_home_keys[] = {
  { KEY_F1, Condition::None, "LBLs", operation(Operation::LabelJump) },
  { KEY_F2, Condition::None, "JUMP", operation(Operation::Follow) },
  // DMCP keeps HP's ↑ at 0x5e and maps the ↓ control character to 0x8e.
  { KEY_F3, Condition::None, "FIND\216", operation(Operation::Find) },
  { KEY_F3, Condition::Shifted, "FIND^", operation(Operation::FindReverse) },
  { KEY_F5, Condition::None, "AGAIN", operation(Operation::Repeat) },
  { KEY_F6, Condition::None, "UNDO", operation(Operation::Undo) },
  { KEY_F6, Condition::Shifted, "REDO", operation(Operation::Redo) }
};

static home_bindings current_bindings(bool shifted) {
  unsigned int active = Condition::None;
  if (shifted) active |= Condition::Shifted;
  if (usb_powered()) active |= Condition::UsbConnected;
  return is_pgm_mode() ? home_bindings::select(program_home_keys, active)
                       : home_bindings::select(normal_home_keys, active);
}

void draw_custom_home_menu(bool shifted) {
  current_bindings(shifted).draw();
}

const custom_key_action *prepare_custom_home_key(int &key, bool &shifted, bool actions_allowed) {
  const custom_key_action *selected_action = nullptr;
  if (core_menu() || core_hex_menu() || core_alpha_menu()
      || key < KEY_F1 || key > KEY_F6)
    return selected_action;

  selected_action = current_bindings(shifted).action_for_key(key);
  shifted = false;
  if (!actions_allowed)
    selected_action = nullptr;
  if (selected_action == nullptr) key = -1;
  return selected_action;
}

bool dispatch_custom_home_key(const custom_key_action *action, int key, bool *enqueued,
                              int *repeat, int no_menu_key) {
  if (action != nullptr)
    return action->execute(enqueued, repeat);
  return core_keydown_ex(key, enqueued, repeat, no_menu_key);
}
