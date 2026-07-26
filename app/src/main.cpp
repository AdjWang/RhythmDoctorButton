#include <memory>
#include <string_view>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "battery_adc.h"
#include "button.h"
#include "keyboard/keyboard.h"
#include "keyboard/usb_device.h"
#include "light.h"

static constexpr std::string_view kDeviceName = "RhythmDoctorButton";
static constexpr std::string_view kDeviceManufacturer = "TeamIO";
static constexpr float kBkgLedFlashBrightness = 0.01f;
static constexpr float kKeyLedFlashBrightness = 0.3f;
static constexpr uint32_t kMainLoopDelayMs = 10;
static constexpr uint32_t kBatLevelReportDurationMs = 1000;

namespace {

constexpr gpio_dt_spec kBootButton =
    GPIO_DT_SPEC_GET(DT_ALIAS(boot_btn), gpios);
constexpr gpio_dt_spec kMainButton =
    GPIO_DT_SPEC_GET(DT_ALIAS(main_btn), gpios);
constexpr pwm_dt_spec kBkgLed = PWM_DT_SPEC_GET(DT_ALIAS(bkg_led));
constexpr pwm_dt_spec kKeyLed = PWM_DT_SPEC_GET(DT_ALIAS(key_led));
constexpr adc_dt_spec kBatteryAdc =
    ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

std::unique_ptr<rdb::IKeyboard> ble_keyboard;
std::unique_ptr<rdb::IKeyboard> usb_keyboard;
rdb::IKeyboard* keyboard = nullptr;

rdb::Button boot_btn(kBootButton);
rdb::Button main_btn(kMainButton);
rdb::BatteryAdc bat_adc(kBatteryAdc);
rdb::Light bkg_led(kBkgLed);
rdb::Light key_led(kKeyLed);
rdb::LightFlasher advertising_flash;

uint32_t last_report_time_ms = 0;

void InspectKeyboardMode() {
  if (usb_keyboard != nullptr && keyboard == usb_keyboard.get()) {
    printk("Using USB HID keyboard\n");
  } else if (ble_keyboard != nullptr && keyboard == ble_keyboard.get()) {
    printk("Using Bluetooth keyboard\n");
  } else if (keyboard == nullptr) {
    printk("Keyboard unplugged\n");
  } else {
    printk("Invalid keyboard mode\n");
  }
}

void OnMainButtonPress() {
  printk("Press button\n");
  bkg_led.SetBrightness(kBkgLedFlashBrightness);
  key_led.SetBrightness(kKeyLedFlashBrightness);
  if (keyboard != nullptr && keyboard->is_connected()) {
    keyboard->Press(' ');
  }
}

void OnMainButtonRelease() {
  printk("Release button\n");
  bkg_led.SetBrightness(0.0f);
  key_led.SetBrightness(0.0f);
  if (keyboard != nullptr && keyboard->is_connected()) {
    keyboard->Release(' ');
  }
}

void EnableAdvertisingFlash() { advertising_flash.Reset(); }

void DisableAdvertisingFlash() {
  advertising_flash.Reset();
  bkg_led.SetBrightness(0.0f);
  key_led.SetBrightness(0.0f);
}

void Setup() {
  boot_btn.Begin();
  boot_btn.set_on_press(OnMainButtonPress);
  boot_btn.set_on_release(OnMainButtonRelease);

  main_btn.Begin();
  main_btn.set_on_press(OnMainButtonPress);
  main_btn.set_on_release(OnMainButtonRelease);

  bat_adc.Begin();

  bkg_led.Begin();
  key_led.Begin();
  advertising_flash.SetPattern({
      {true, 100},
      {false, 1400},
  });

  ble_keyboard =
      std::make_unique<rdb::BleKeyboard>(kDeviceName, kDeviceManufacturer);
  ble_keyboard->Begin();

  usb_keyboard =
      std::make_unique<rdb::UsbKeyboard>(rdb::GetUsbHidDevice(), kDeviceName,
                                         kDeviceManufacturer);
  usb_keyboard->Begin();
}

void Loop() {
  const uint32_t now_ms = static_cast<uint32_t>(k_uptime_get());

  boot_btn.Update();
  main_btn.Update();
  bat_adc.Update();

  if (now_ms - last_report_time_ms > kBatLevelReportDurationMs) {
    last_report_time_ms = now_ms;
    const uint8_t level = bat_adc.battery_level();
    if (ble_keyboard != nullptr) {
      ble_keyboard->SetBatteryLevel(level);
    }
  }

  if (usb_keyboard != nullptr && usb_keyboard->is_connected()) {
    if (keyboard != usb_keyboard.get()) {
      keyboard = usb_keyboard.get();
      InspectKeyboardMode();
      DisableAdvertisingFlash();
    }
  } else if (ble_keyboard != nullptr && ble_keyboard->is_connected()) {
    if (keyboard != ble_keyboard.get()) {
      keyboard = ble_keyboard.get();
      keyboard->ReleaseAll();
      InspectKeyboardMode();
      DisableAdvertisingFlash();
    }
  } else {
    if (keyboard != nullptr) {
      keyboard = nullptr;
      InspectKeyboardMode();
      EnableAdvertisingFlash();
    }

    advertising_flash.Update(now_ms);
    key_led.SetBrightness(advertising_flash.is_on() ? kKeyLedFlashBrightness
                                                    : 0.0f);
  }
}

}  // namespace

int main() {
  printk("RhythmDoctorButton nRF52840 starting\n");
  Setup();

  while (true) {
    Loop();
    k_msleep(kMainLoopDelayMs);
  }
}
