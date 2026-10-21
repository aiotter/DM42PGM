#ifndef DM42_RELOCATED_MAIN_MENU_CUSTOM_H
#define DM42_RELOCATED_MAIN_MENU_CUSTOM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int run_relocated_main_menu_item(uint8_t line_id);
const char *relocated_main_menu_line_str(uint8_t line_id, char *s, int slen);

#ifdef __cplusplus
}
#endif

#endif
