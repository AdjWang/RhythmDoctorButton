#include "keyboard/keyboard.h"
#include "keyboard/usb_device.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main() {
  rdb::UsbKeyboard keyboard(rdb::GetUsbHidDevice());
  keyboard.Begin();

  while (true) {
    if (keyboard.is_connected()) {
      printk("usb connected, sending space\n");
      keyboard.Press(' ');
      k_sleep(K_MSEC(50));
      keyboard.Release(' ');
    } else {
      printk("usb waiting for host\n");
    }
    k_sleep(K_SECONDS(2));
  }
}
