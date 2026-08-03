#pragma once

#include "cmd.h"

#include <expected>

#include <zephyr/settings/settings.h>

namespace rdb {

int AppSettingsHandleSet(const char* name, size_t len,
                         settings_read_cb read_cb, void* cb_arg);

class AppSettings {
 public:
  struct Values {
    bool enable_led = true;
    float key_led_brightness = 0.2f;
    float bkg_led_brightness = 0.08f;
  };

  struct Error {
    int code = 0;
    const char* operation = "";
  };

  using Result = std::expected<void, Error>;

  static AppSettings& Instance();

  Result LoadOrWriteDefaults();
  Result SetEnableLed(bool value);
  Result SetKeyLedBrightness(float value);
  Result SetBkgLedBrightness(float value);
  Result ResetToDefaults();
  Result SaveCommand(const CmdParser::Command& command);
  void RegisterCommands(CmdParser& parser) const;

  const Values& values() const { return values_; }

 private:
  friend int AppSettingsHandleSet(const char* name, size_t len,
                                  settings_read_cb read_cb, void* cb_arg);

  AppSettings() = default;

  int HandleSet(const char* name, size_t len, settings_read_cb read_cb,
                void* cb_arg);
  Result SaveMissingDefaults();
  Result SaveValues();
  Result SaveValue(const char* key, const void* value, size_t size);

  Values values_;
  bool has_saved_values_ = false;
};

}  // namespace rdb
