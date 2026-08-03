#include "cmd.h"
#include "setting.h"

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console),
                                zephyr_cdc_acm_uart),
             "Console device must be a USB CDC ACM UART");

namespace {

int32_t ToMilli(const float value) {
  const float scaled = value * 1000.0f;
  if (scaled >= 0.0f) {
    return static_cast<int32_t>(scaled + 0.5f);
  }
  return static_cast<int32_t>(scaled - 0.5f);
}

void PrintView(const std::string_view text) {
  printk("%.*s", static_cast<int>(text.size()), text.data());
}

void PrintCommand(const rdb::CmdParser::Command& command) {
  printk("parsed command: key=");
  PrintView(command.key);
  if (const bool* const enabled = std::get_if<bool>(&command.value)) {
    printk(" value=%s\n", *enabled ? "true" : "false");
    return;
  }
  const float brightness = std::get<float>(command.value);
  printk(" value=");
  PrintView(command.value_text);
  printk(" milli=%d\n", ToMilli(brightness));
}

void PrintError(const rdb::Error& error) {
  printk("parse error: code=");
  PrintView(rdb::ToString(error.code));
  printk(" message=");
  PrintView(error.message);
  printk("\n");
}

void PrintResult(const rdb::CmdParser::Result& result) {
  if (result.has_value()) {
    PrintCommand(*result);
    return;
  }
  PrintError(result.error());
}

void PrintSettingsError(const rdb::AppSettings::Error& error) {
  printk("settings error: operation=%s code=%d\n", error.operation,
         error.code);
}

void WaitForConsole(const device* const console) {
  uint32_t dtr = 0;
  while (dtr == 0) {
    (void)uart_line_ctrl_get(console, UART_LINE_CTRL_DTR, &dtr);
    k_sleep(K_MSEC(100));
  }
  (void)uart_line_ctrl_set(console, UART_LINE_CTRL_DCD, 1);
  (void)uart_line_ctrl_set(console, UART_LINE_CTRL_DSR, 1);
  k_sleep(K_MSEC(100));
}

}  // namespace

int main() {
  const device* const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
  if (!device_is_ready(console)) {
    return 0;
  }
  WaitForConsole(console);
  printk("cmd test ready\n");
  rdb::AppSettings& settings = rdb::AppSettings::Instance();
  if (const auto result = settings.LoadOrWriteDefaults(); !result.has_value()) {
    PrintSettingsError(result.error());
    return 0;
  }
  rdb::CmdParser parser;
  settings.RegisterCommands(parser);
  while (true) {
    uint8_t byte = 0;
    while (uart_poll_in(console, &byte) == 0) {
      if (byte == '\r' || byte == '\n') {
        printk("\n");
      } else if (byte == '\b' || byte == 0x7f) {
        printk("\b \b");
      } else if (byte >= ' ' && byte <= '~') {
        (void)uart_poll_out(console, byte);
      }
      if (const std::optional<rdb::CmdParser::Result> result =
              parser.Feed(static_cast<char>(byte))) {
        PrintResult(*result);
        if (result->has_value()) {
          if (const auto save_result = settings.SaveCommand(**result);
              !save_result.has_value()) {
            PrintSettingsError(save_result.error());
          }
        }
      }
    }
    k_sleep(K_MSEC(10));
  }
}
