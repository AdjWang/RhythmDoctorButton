#include <memory>
#include <string_view>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "keyboard/keyboard.h"

static constexpr std::string_view kDeviceName = "RhythmDoctorButton";
static constexpr std::string_view kDeviceManufacturer = "TeamIO";

#define SLEEP_TIME_MS 500
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void) {
  int ret;
  bool led_state = true;

  printk("Hello from nRF52840 BLE HID\n");

  if (!gpio_is_ready_dt(&led)) {
    return 0;
  }

  ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    return 0;
  }

  auto keyboard = std::make_unique<rdb::BleKeyboard>(kDeviceName,
                                                      kDeviceManufacturer);
  keyboard->Begin();

  while (true) {
    ret = gpio_pin_toggle_dt(&led);
    if (ret < 0) {
      return 0;
    }

    led_state = !led_state;
    // printk("LED state: %s\n", led_state ? "ON" : "OFF");
    k_msleep(SLEEP_TIME_MS);
  }

  return 0;
}
