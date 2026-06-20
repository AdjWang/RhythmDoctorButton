#include "light.h"

#include <cmath>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

int main() {
  rdb::Light bkg_led(PWM_DT_SPEC_GET(DT_ALIAS(bkg_led)), true);
  rdb::Light key_led(PWM_DT_SPEC_GET(DT_ALIAS(key_led)));

  bkg_led.Begin();
  key_led.Begin();

  uint32_t step = 0;
  while (true) {
    const float phase = static_cast<float>(step % 200) / 199.0f;
    const float brightness = 0.5f - 0.5f * std::cos(phase * 3.1415926f);
    bkg_led.SetBrightness(brightness);
    key_led.SetBrightness(1.0f - brightness);
    ++step;
    k_sleep(K_MSEC(20));
  }
}
