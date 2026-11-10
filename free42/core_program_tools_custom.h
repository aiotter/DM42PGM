/* DM42-facing declarations for the custom Free42 program-editor tools. */
#ifndef CORE_PROGRAM_TOOLS_CUSTOM_H
#define CORE_PROGRAM_TOOLS_CUSTOM_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool core_program_label_jump(bool *enqueued, int *repeat);
bool core_program_follow(bool *enqueued, int *repeat);
bool core_program_insert_after(bool *enqueued, int *repeat);
bool core_program_request_delete(bool *enqueued, int *repeat);

void core_get_program_position(int *index, int *count);

#ifdef __cplusplus
}
#endif

#endif
