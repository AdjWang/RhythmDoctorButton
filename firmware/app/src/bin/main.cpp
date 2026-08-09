#include <memory>
#include <optional>
#include <string_view>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "battery_adc.h"
#include "button.h"
#include "cmd.h"
#include "keyboard/keyboard.h"
#include "keyboard/usb_device.h"
#include "light.h"
#include "setting.h"

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console),
                                zephyr_cdc_acm_uart),
             "Console device must be a USB CDC ACM UART");

namespace {
static constexpr uint32_t kMainLoopDelayMs = 10;
static constexpr uint32_t kBatLevelReportDurationMs = 1000;
// Force advertising flash even when led is configured off.
static constexpr float kAdvertisingFlashBrightness = 0.5f;

constexpr gpio_dt_spec kMainButton =
    GPIO_DT_SPEC_GET(DT_ALIAS(main_btn), gpios);
constexpr pwm_dt_spec kBkgLed = PWM_DT_SPEC_GET(DT_ALIAS(bkg_led));
constexpr pwm_dt_spec kKeyLed = PWM_DT_SPEC_GET(DT_ALIAS(key_led));
constexpr adc_dt_spec kBatteryAdc =
    ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

std::unique_ptr<rdb::IKeyboard> ble_keyboard;
std::unique_ptr<rdb::IKeyboard> usb_keyboard;
rdb::IKeyboard* keyboard = nullptr;

rdb::Button main_btn(kMainButton);
rdb::BatteryAdc bat_adc(kBatteryAdc);
rdb::Light bkg_led(kBkgLed);
rdb::Light key_led(kKeyLed);
rdb::LightFlasher advertising_flash;
rdb::LightFlasher bkg_flash;
rdb::AppSettings& app_settings = rdb::AppSettings::Instance();
rdb::CmdParser cmd_parser;
const device* console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

uint32_t last_report_time_ms = 0;
bool cmd_console_connected = false;

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

void PrintView(const std::string_view text) {
  printk("%.*s", static_cast<int>(text.size()), text.data());
}

void PrintCmdError(const rdb::Error& error) {
  printk("cmd parse error: code=");
  PrintView(rdb::ToString(error.code));
  printk(" message=");
  PrintView(error.message);
  printk("\n");
}

void PrintSettingsError(const rdb::AppSettings::Error& error) {
  printk("settings error: operation=%s code=%d\n", error.operation,
         error.code);
}

void HandleCommandResult(const rdb::CmdParser::Result& result) {
  if (!result.has_value()) {
    PrintCmdError(result.error());
    return;
  }
  const auto save_result = app_settings.SaveCommand(*result);
  if (!save_result.has_value()) {
    PrintSettingsError(save_result.error());
    return;
  }
  printk("saved command: key=");
  PrintView(result->key);
  printk(" value=");
  PrintView(result->value_text);
  printk("\n");
}

void UpdateCommands() {
  if (!device_is_ready(console)) {
    return;
  }
  uint32_t dtr = 0;
  // The full app prints boot logs through the same CDC UART used for commands.
  // Wait for a real terminal session and drain stale bytes so those logs are not
  // parsed as the first command.
  if (uart_line_ctrl_get(console, UART_LINE_CTRL_DTR, &dtr) != 0 || dtr == 0) {
    if (cmd_console_connected) {
      cmd_parser.Reset();
      cmd_console_connected = false;
    }
    return;
  }
  uint8_t byte = 0;
  if (!cmd_console_connected) {
    cmd_parser.Reset();
    cmd_console_connected = true;
    (void)uart_line_ctrl_set(console, UART_LINE_CTRL_DCD, 1);
    (void)uart_line_ctrl_set(console, UART_LINE_CTRL_DSR, 1);
    while (uart_poll_in(console, &byte) == 0) {
    }
    printk("cmd console ready\n");
    return;
  }
  while (uart_poll_in(console, &byte) == 0) {
    if (byte == '\r' || byte == '\n') {
      printk("\n");
    } else if (byte == '\b' || byte == 0x7f) {
      printk("\b \b");
    } else if (byte >= ' ' && byte <= '~') {
      (void)uart_poll_out(console, byte);
    }
    if (const std::optional<rdb::CmdParser::Result> result =
            cmd_parser.Feed(static_cast<char>(byte))) {
      HandleCommandResult(*result);
    }
  }
}

void OnMainButtonPress() {
  printk("Press button\n");
  if (app_settings.values().enable_led) {
    key_led.SetBrightness(app_settings.values().key_led_brightness);
    bkg_flash.Reset();
  }
  if (keyboard != nullptr && keyboard->is_connected()) {
    keyboard->Press(' ');
  }
}

void OnMainButtonRelease() {
  printk("Release button\n");
  key_led.SetBrightness(0.0f);
  if (keyboard != nullptr && keyboard->is_connected()) {
    keyboard->Release(' ');
  }
}

void EnableAdvertisingFlash() {
  advertising_flash.Reset();
}

void DisableAdvertisingFlash() {
  advertising_flash.Reset();
  key_led.SetBrightness(0.0f);
}

void Setup() {
  main_btn.Begin();
  main_btn.set_on_press(OnMainButtonPress);
  main_btn.set_on_release(OnMainButtonRelease);
  if (const auto result = app_settings.LoadOrWriteDefaults();
      !result.has_value()) {
    PrintSettingsError(result.error());
  }
  app_settings.RegisterCommands(cmd_parser);
  bat_adc.Begin();
  bkg_led.Begin();
  key_led.Begin();
  advertising_flash.SetPattern({
      {1.0f, 100},    // fully on for 100ms
      {0.0f, 1400},   // fully off for 1400ms
  }, /*loop*/ true);
  bkg_flash.SetPattern({
      {1.0f, 40},
      {0.7f, 20},
      {0.3f, 20},
      {0.0f, 10},
  }, /*loop*/ false);
  // Initialize as closed.
  bkg_flash.Reset(-1);
  usb_keyboard = std::make_unique<rdb::UsbKeyboard>(rdb::GetUsbHidDevice());
  usb_keyboard->Begin();
  ble_keyboard = std::make_unique<rdb::BleKeyboard>(
      CONFIG_BT_DEVICE_NAME, CONFIG_BT_DIS_MANUF_NAME_STR);
  ble_keyboard->Begin();
}

void Loop() {
  const uint32_t now_ms = static_cast<uint32_t>(k_uptime_get());
  // Receive setting commands.
  UpdateCommands();
  // Update button state.
  main_btn.Update();
  // Send battery level.
  bat_adc.Update();
  if (now_ms - last_report_time_ms > kBatLevelReportDurationMs) {
    last_report_time_ms = now_ms;
    const uint8_t level = bat_adc.battery_level();
    if (ble_keyboard != nullptr) {
      ble_keyboard->SetBatteryLevel(level);
    }
  }
  // Select keyboard.
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
    // Flash key led to show advertising state.
    advertising_flash.Update(now_ms);
    key_led.SetBrightness(kAdvertisingFlashBrightness *
                          advertising_flash.get_brightness_ratio());
  }
  // Flash background led.
  bkg_flash.Update(now_ms);
  bkg_led.SetBrightness(app_settings.values().bkg_led_brightness *
                        bkg_flash.get_brightness_ratio());
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
