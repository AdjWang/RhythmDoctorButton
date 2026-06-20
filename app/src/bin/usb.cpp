#include <memory>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "keyboard/keyboard.h"

int main() {
  printk("usb hid test: sends space every 2 seconds when connected\n");

  auto keyboard =
      std::make_unique<rdb::UsbKeyboard>("RDB USB Test", "TeamIO");
  keyboard->Begin();

  while (true) {
    if (keyboard->is_connected()) {
      printk("usb connected, sending space\n");
      keyboard->Press(' ');
      k_msleep(50);
      keyboard->Release(' ');
    } else {
      printk("usb not connected\n");
    }
    k_msleep(2000);
  }
}
