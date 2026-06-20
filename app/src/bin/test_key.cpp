#include "button.h"
#include "light.h"

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

int main() {
  rdb::Button main_btn(GPIO_DT_SPEC_GET(DT_ALIAS(main_btn), gpios));
  rdb::Light key_led(PWM_DT_SPEC_GET(DT_ALIAS(key_led)));

  main_btn.Begin();
  key_led.Begin();

  while (true) {
    main_btn.Update();
    key_led.SetBrightness(main_btn.pressed() ? 1.0f : 0.0f);
    k_sleep(K_MSEC(5));
  }
}
