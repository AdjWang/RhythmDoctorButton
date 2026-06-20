#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "battery_adc.h"

namespace {

constexpr adc_dt_spec kBatteryAdc = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

}  // namespace

int main() {
  rdb::BatteryAdc battery_adc(kBatteryAdc);
  battery_adc.Begin();

  printk("adc test: battery level from P0.04 / AIN2\n");
  while (true) {
    battery_adc.Update();
    printk("battery level %u%%\n", battery_adc.battery_level());
    k_msleep(1000);
  }
}
