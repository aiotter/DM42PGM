#include <stddef.h>

#include "dm42_usb_actions_custom.h"

extern "C" {
#include <main.h>
#include <dmcp.h>
#include <dm42_fns.h>
}

extern "C" int statefile_save();

static int wait_for_usb_confirmation() {
  // Sound the prompt without repainting the calculator over the dialog.
  start_buzzer_freq(2000000);
  sys_delay(50);
  stop_buzzer();

  // The held shortcut key must not confirm or reopen the dialog.
  wait_for_key_release(-1);
  for (;;) {
    if (!usb_powered()) return 0;
    int key = runner_get_key(NULL);
    if (IS_EXIT_KEY(key) || is_menu_auto_off() || !usb_powered())
      return 0;
    if (key == KEY_ENTER) {
      wait_for_key_release(-1);
      return usb_powered();
    }
  }
}

static void run_usb_action(uint8_t menu_item) {
  if (!usb_powered()) return;
  SET_ST(STAT_MENU);
  lcd_writeClr(t24);
  lcd_clear_buf();
  if (menu_item == MI_BOOTLOADER) {
    lcd_putsR(t24, "Bootloader");
    t24->ln_offs = 8;
    lcd_puts(t24, "Enter USB bootloader mode?");
    lcd_puts(t24, "");
    lcd_puts(t24, "RESET is required to exit.");
    lcd_puts(t24, "");
  } else {
    lcd_putsR(t24, "USB Disk");
    t24->ln_offs = 8;
    lcd_puts(t24, "Activate USB disk?");
    lcd_puts(t24, "");
  }
  lcd_puts(t24, "Press [ENTER] to confirm.");
  lcd_puts(t24, "Press [EXIT] to cancel.");
  lcd_refresh_wait();

  if (wait_for_usb_confirmation()) {
    prtof_buf_flush(PRTOF_GRAPHICS, PRTOF_FULL_FLUSH);
    prtof_buf_flush(PRTOF_TEXT, PRTOF_FULL_FLUSH);
    if (statefile_save() == 0 && sys_disk_ok()) {
      if (usb_powered()) run_menu_item_sys(menu_item);
    } else {
      // Keep the save error visible and leave the calculator running.
      wait_for_key_press();
    }
  }
  CLR_ST(STAT_MENU);
  calc_lcd_redraw();
  wait_for_key_release(-1);
}

void enter_bootloader(void) {
  run_usb_action(MI_BOOTLOADER);
}

void enter_usb_disk(void) {
  run_usb_action(MI_MSC);
}
