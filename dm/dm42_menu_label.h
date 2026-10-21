#ifndef DM42_MENU_LABEL_H
#define DM42_MENU_LABEL_H

#include <stddef.h>

// Convert length bytes of Free42 text, including any zero bytes, to a DMCP label.
// Write at most Capacity - 1 characters and terminate the result with a zero byte.
template<size_t Capacity>
void format_core_menu_label(char (&text)[Capacity], const char *label, size_t length) {
  size_t written = 0;
  for (size_t i = 0; i < length && written + 1 < Capacity; i++) {
    unsigned char c = label[i];
    // DM42 menu labels skip high-bit bytes, including Free42 abbreviation markers.
    if (c & 0x80) continue;
    // DMCP encodes Free42 control characters in the range 0x80-0x9f.
    text[written++] = c < ' ' ? c | 0x80 : c;
  }
  text[written] = '\0';
}

#endif
