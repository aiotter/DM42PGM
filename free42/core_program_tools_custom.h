/* DM42-facing declarations for the custom Free42 program-editor tools. */
#ifndef CORE_PROGRAM_TOOLS_CUSTOM_H
#define CORE_PROGRAM_TOOLS_CUSTOM_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// These editor actions consume a key press; use the normal core_keyup() afterward.
bool core_program_label_jump(bool *enqueued, int *repeat);
bool core_program_follow(bool *enqueued, int *repeat);
bool core_program_find(bool reverse, bool *enqueued, int *repeat);

#ifdef __cplusplus
}
#endif

#endif
