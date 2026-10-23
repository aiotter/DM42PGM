/* DM42-facing declarations for the custom Free42 time display. */
#ifndef CORE_TIME_CUSTOM_H
#define CORE_TIME_CUSTOM_H

#ifdef __cplusplus
extern "C" {
#endif

// The low 4 bits store display precision, rounding and the active time application.
unsigned char time_display_save();
void time_display_restore(unsigned char data);

#ifdef __cplusplus
}
#endif

#endif
