#include "battery_adc.h"

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main() {
  rdb::BatteryAdc battery(ADC_DT_SPEC_GET(DT_PATH(zephyr_user)));
  battery.Begin();

  while (true) {
    battery.Update();
    printk("battery level %u%%\n", battery.battery_level());
    k_sleep(K_SECONDS(1));
  }
}
