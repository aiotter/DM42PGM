/* DM42-facing declarations for the custom Free42 BASE extension. */
#ifndef CORE_BASE_CUSTOM_H
#define CORE_BASE_CUSTOM_H

#ifdef __cplusplus
extern "C" {
#endif

// A zero base suppresses the radix label; an empty X can still have a label.
int base_display_base();
// Keep the alternate pane visible even when an empty stack has no value to format.
int altbase_visible();
int altbase2str(char *buf, int buflen, int *base);

// Use one bit per radix in the low 4 bits; zero disables the alternate pane.
unsigned char base_display_save();
void base_display_restore(unsigned char data);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
