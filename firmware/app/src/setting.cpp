#include "setting.h"

#include <errno.h>
#include <cmath>
#include <string_view>
#include <variant>

#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>

namespace rdb {
namespace {

constexpr const char* kSettingsTree = "rdb";
constexpr const char* kEnableLedKey = "enable_led";
constexpr const char* kKeyLedBrightnessKey = "key_led_brightness";
constexpr const char* kBkgLedBrightnessKey = "bkg_led_brightness";
constexpr const char* kFactoryResetKey = "factory_reset";

bool IsBrightnessValid(const float value) {
  return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
}

int ReadSetting(const char* const name, const size_t len,
                settings_read_cb read_cb, void* const cb_arg,
                const char* const expected_name, void* const value,
                const size_t value_size) {
  const char* next = nullptr;
  if (!settings_name_steq(name, expected_name, &next) || next != nullptr) {
    return -ENOENT;
  }
  if (len != value_size) {
    return -EINVAL;
  }
  const ssize_t bytes_read = read_cb(cb_arg, value, value_size);
  if (bytes_read != static_cast<ssize_t>(value_size)) {
    return -EIO;
  }
  return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(rdb_app_settings, kSettingsTree, nullptr,
                               AppSettingsHandleSet, nullptr, nullptr);

}  // namespace

int AppSettingsHandleSet(const char* name, size_t len,
                         settings_read_cb read_cb, void* cb_arg) {
  return AppSettings::Instance().HandleSet(name, len, read_cb, cb_arg);
}

AppSettings& AppSettings::Instance() {
  static AppSettings settings;
  return settings;
}

AppSettings::Result AppSettings::LoadOrWriteDefaults() {
  int ret = settings_subsys_init();
  if (ret != 0) {
    return std::unexpected(Error{.code = ret, .operation = "settings init"});
  }
  ret = settings_load_subtree(kSettingsTree);
  if (ret != 0) {
    return std::unexpected(Error{.code = ret, .operation = "settings load"});
  }
  return SaveMissingDefaults();
}

AppSettings::Result AppSettings::SetEnableLed(const bool value) {
  values_.enable_led = value;
  Result result =
      SaveValue(kEnableLedKey, &values_.enable_led, sizeof(values_.enable_led));
  if (result.has_value()) {
    has_saved_values_ = true;
  }
  return result;
}

AppSettings::Result AppSettings::SetKeyLedBrightness(const float value) {
  if (!IsBrightnessValid(value)) {
    return std::unexpected(
        Error{.code = -ERANGE, .operation = "key_led_brightness range"});
  }
  values_.key_led_brightness = value;
  Result result = SaveValue(kKeyLedBrightnessKey, &values_.key_led_brightness,
                            sizeof(values_.key_led_brightness));
  if (result.has_value()) {
    has_saved_values_ = true;
  }
  return result;
}

AppSettings::Result AppSettings::SetBkgLedBrightness(const float value) {
  if (!IsBrightnessValid(value)) {
    return std::unexpected(
        Error{.code = -ERANGE, .operation = "bkg_led_brightness range"});
  }
  values_.bkg_led_brightness = value;
  Result result = SaveValue(kBkgLedBrightnessKey, &values_.bkg_led_brightness,
                            sizeof(values_.bkg_led_brightness));
  if (result.has_value()) {
    has_saved_values_ = true;
  }
  return result;
}

AppSettings::Result AppSettings::ResetToDefaults() {
  values_ = Values{};
  Result result = SaveValues();
  if (result.has_value()) {
    has_saved_values_ = true;
  }
  return result;
}

AppSettings::Result AppSettings::SaveCommand(
    const CmdParser::Command& command) {
  if (command.key == kEnableLedKey) {
    return SetEnableLed(std::get<bool>(command.value));
  }
  if (command.key == kKeyLedBrightnessKey) {
    return SetKeyLedBrightness(std::get<float>(command.value));
  }
  if (command.key == kBkgLedBrightnessKey) {
    return SetBkgLedBrightness(std::get<float>(command.value));
  }
  if (command.key == kFactoryResetKey) {
    if (!std::get<bool>(command.value)) {
      return std::unexpected(
          Error{.code = -EINVAL, .operation = "factory_reset value"});
    }
    return ResetToDefaults();
  }
  return std::unexpected(Error{.code = -ENOENT, .operation = "settings save"});
}

void AppSettings::RegisterCommands(CmdParser& parser) const {
  parser.RegisterCommand(kEnableLedKey, CmdParser::ValueType::kBool);
  parser.RegisterCommand(kKeyLedBrightnessKey, CmdParser::ValueType::kFloat);
  parser.RegisterCommand(kBkgLedBrightnessKey, CmdParser::ValueType::kFloat);
  parser.RegisterCommand(kFactoryResetKey, CmdParser::ValueType::kBool);
}

int AppSettings::HandleSet(const char* const name, const size_t len,
                           settings_read_cb read_cb, void* const cb_arg) {
  int ret = ReadSetting(name, len, read_cb, cb_arg, kEnableLedKey,
                        &values_.enable_led, sizeof(values_.enable_led));
  if (ret != -ENOENT) {
    if (ret == 0) {
      has_saved_values_ = true;
    }
    return ret;
  }
  ret = ReadSetting(name, len, read_cb, cb_arg, kKeyLedBrightnessKey,
                    &values_.key_led_brightness,
                    sizeof(values_.key_led_brightness));
  if (ret != -ENOENT) {
    if (ret == 0) {
      has_saved_values_ = true;
    }
    return ret;
  }
  ret = ReadSetting(name, len, read_cb, cb_arg, kBkgLedBrightnessKey,
                    &values_.bkg_led_brightness,
                    sizeof(values_.bkg_led_brightness));
  if (ret != -ENOENT) {
    if (ret == 0) {
      has_saved_values_ = true;
    }
    return ret;
  }
  return -ENOENT;
}

AppSettings::Result AppSettings::SaveMissingDefaults() {
  if (has_saved_values_) {
    return {};
  }
  Result result = SaveValues();
  if (result.has_value()) {
    has_saved_values_ = true;
  }
  return result;
}

AppSettings::Result AppSettings::SaveValues() {
  Result result =
      SaveValue(kEnableLedKey, &values_.enable_led, sizeof(values_.enable_led));
  if (!result.has_value()) {
    return result;
  }
  result = SaveValue(kKeyLedBrightnessKey, &values_.key_led_brightness,
                     sizeof(values_.key_led_brightness));
  if (!result.has_value()) {
    return result;
  }
  result = SaveValue(kBkgLedBrightnessKey, &values_.bkg_led_brightness,
                     sizeof(values_.bkg_led_brightness));
  if (!result.has_value()) {
    return result;
  }
  return {};
}

AppSettings::Result AppSettings::SaveValue(const char* const key,
                                           const void* const value,
                                           const size_t size) {
  char full_key[64] = {};
  const int ret = snprintk(full_key, sizeof(full_key), "%s/%s", kSettingsTree,
                           key);
  if (ret < 0 || ret >= static_cast<int>(sizeof(full_key))) {
    return std::unexpected(
        Error{.code = -ENAMETOOLONG, .operation = "settings key"});
  }
  const int save_ret = settings_save_one(full_key, value, size);
  if (save_ret != 0) {
    return std::unexpected(
        Error{.code = save_ret, .operation = "settings save"});
  }
  return {};
}

}  // namespace rdb
