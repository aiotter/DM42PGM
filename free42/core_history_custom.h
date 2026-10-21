/* DM42-facing declarations for the custom Free42 command and editor history. */
#ifndef CORE_HISTORY_CUSTOM_H
#define CORE_HISTORY_CUSTOM_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// These actions consume a key press; use the normal core_keyup() afterward.
bool core_repeat_last_command(bool *enqueued, int *repeat);
bool core_undo(bool *enqueued, int *repeat);
bool core_redo(bool *enqueued, int *repeat);

#ifdef __cplusplus
}
#endif

#endif
