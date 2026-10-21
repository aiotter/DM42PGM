#ifndef DM42_HOME_MENU_CUSTOM_H
#define DM42_HOME_MENU_CUSTOM_H

struct custom_key_action;

void draw_custom_home_menu(bool shifted);
// Consume the Shift snapshot only when the key belongs to the active home menu.
// The frontend applies the updated snapshot to its annunciator state.
// The returned pointer refers to a static binding; nullptr means no home action.
const custom_key_action *prepare_custom_home_key(int &key, bool &shifted, bool actions_allowed);
bool dispatch_custom_home_key(const custom_key_action *action, int key, bool *enqueued,
                              int *repeat, int no_menu_key);

#endif
