#ifndef DM42_HOME_MENU_CUSTOM_H
#define DM42_HOME_MENU_CUSTOM_H

#include "core_home_menu_custom.h"

void draw_custom_home_menu(bool shifted);
// Select a HOME binding without executing its action. Clear shifted for a
// HOME key and set key to -1 when actions are disabled or the slot is empty.
// For other keys, return an empty binding and leave key and shifted unchanged.
core_home_binding prepare_custom_home_key(int &key, bool &shifted, bool actions_allowed);
// Run the selected action or pass key to core_keydown_ex if the binding is empty.
bool dispatch_custom_home_key(core_home_binding binding, int key, bool *enqueued,
                              int *repeat, int no_menu_key);

#endif
