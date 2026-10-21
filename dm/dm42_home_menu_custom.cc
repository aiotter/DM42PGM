#include "dm42_home_menu_custom.h"
#include "dm42_menu_label.h"
#include "dm42_usb_actions_custom.h"
#include "core_main.h"
#include "core_aux.h"

extern "C" {
#include "dmcp.h"
}

static_assert(CORE_HOME_KEY_COUNT == MENU_KEY_COUNT, "DM42 needs one HOME slot per function key");

template<void (*Action)()>
static bool run_usb_action(bool *enqueued, int *repeat) {
  Action();
  *enqueued = false;
  *repeat = 0;
  return false;
}

static core_home_binding current_binding(unsigned int slot, bool shifted) {
  if (slot == KEY_F4 - KEY_F1 && !is_pgm_mode() && usb_powered()) {
    static constexpr char boot[] = "Boot";
    static constexpr char disk[] = "Disk";
    if (shifted)
      return { boot, sizeof(boot) - 1, run_usb_action<enter_bootloader> };
    return { disk, sizeof(disk) - 1, run_usb_action<enter_usb_disk> };
  }
  return core_home_menu_binding(slot, shifted);
}

void draw_custom_home_menu(bool shifted) {
  char text[MENU_KEY_COUNT][MENU_KEY_LABEL_LEN + 1] = {};
  const char *labels[MENU_KEY_COUNT];
  for (unsigned int slot = 0; slot < MENU_KEY_COUNT; slot++) {
    const core_home_binding binding = current_binding(slot, shifted);
    if (binding.label != nullptr)
      format_core_menu_label(text[slot], binding.label, binding.label_length);
    labels[slot] = text[slot];
  }
  lcd_draw_menu_keys(labels);
}

core_home_binding prepare_custom_home_key(int &key, bool &shifted, bool actions_allowed) {
  if (core_menu() || core_hex_menu() || core_alpha_menu() || key < KEY_F1 || key > KEY_F6)
    return {};

  core_home_binding binding = {};
  if (actions_allowed)
    binding = current_binding(key - KEY_F1, shifted);
  shifted = false;
  if (binding.action == nullptr) key = -1;
  return binding;
}

bool dispatch_custom_home_key(core_home_binding binding, int key, bool *enqueued,
                              int *repeat, int no_menu_key) {
  if (binding.action != nullptr)
    return binding.action(enqueued, repeat);
  return core_keydown_ex(key, enqueued, repeat, no_menu_key);
}
