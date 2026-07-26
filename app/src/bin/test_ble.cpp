#include "keyboard/keyboard.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main() {
  const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));

  if (device_is_ready(gpio0)) {
    gpio_pin_configure(gpio0, 2, GPIO_OUTPUT_INACTIVE);
    for (int i = 0; i < 8; ++i) {
      gpio_pin_toggle(gpio0, 2);
      k_sleep(K_MSEC(100));
    }
  }

  rdb::BleKeyboard keyboard("RDB BLE Test", "TeamIO");
  keyboard.Begin();
  keyboard.SetBatteryLevel(87);

  while (true) {
    if (device_is_ready(gpio0)) {
      gpio_pin_toggle(gpio0, 2);
    }

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
