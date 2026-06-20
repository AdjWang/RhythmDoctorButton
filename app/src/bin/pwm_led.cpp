#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "light.h"

namespace {

constexpr pwm_dt_spec kBkgLed = PWM_DT_SPEC_GET(DT_ALIAS(bkg_led));
constexpr pwm_dt_spec kKeyLed = PWM_DT_SPEC_GET(DT_ALIAS(key_led));

}  // namespace

int main() {
  rdb::Light bkg_led(kBkgLed, true);
  rdb::Light key_led(kKeyLed);
  bkg_led.Begin();
  key_led.Begin();

  printk("pwm led test: fade both LEDs\n");
  while (true) {
    for (int i = 0; i <= 100; i += 5) {
      const float brightness = static_cast<float>(i) / 100.0f;
      bkg_led.SetBrightness(brightness * 0.05f);
      key_led.SetBrightness(brightness);
      k_msleep(50);
    }
    for (int i = 100; i >= 0; i -= 5) {
      const float brightness = static_cast<float>(i) / 100.0f;
      bkg_led.SetBrightness(brightness * 0.05f);
      key_led.SetBrightness(brightness);
      k_msleep(50);
    }
  }
}
