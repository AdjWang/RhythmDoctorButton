#include "keyboard/keyboard.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main() {
  rdb::BleKeyboard keyboard("RDB BLE Test", "TeamIO");
  keyboard.Begin();
  keyboard.SetBatteryLevel(87);

  while (true) {
    if (keyboard.is_connected()) {
      printk("ble connected, sending space\n");
      keyboard.Press(' ');
      k_sleep(K_MSEC(50));
      keyboard.Release(' ');
    } else {
      printk("ble advertising or waiting for host\n");
    }
    k_sleep(K_SECONDS(2));
  }
}
