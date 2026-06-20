#include <memory>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "keyboard/keyboard.h"

int main() {
  printk("ble hid test: advertises and sends space every 2 seconds when paired\n");

  auto keyboard =
      std::make_unique<rdb::BleKeyboard>("RDB BLE Test", "TeamIO");
  keyboard->Begin();
  keyboard->SetBatteryLevel(87);

  while (true) {
    if (keyboard->is_connected()) {
      printk("ble connected, sending space\n");
      keyboard->Press(' ');
      k_msleep(50);
      keyboard->Release(' ');
    } else {
      printk("ble not connected\n");
    }
    k_msleep(2000);
  }
}
