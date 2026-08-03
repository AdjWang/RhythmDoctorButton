#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>

#include <cstdint>
#include <cstring>

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console),
                                zephyr_cdc_acm_uart),
             "Console device must be a USB CDC ACM UART");

namespace {

constexpr bool kExpectedEnabled = true;
constexpr float kExpectedGain = 1.25f;
constexpr float kExpectedOffset = -0.5f;

bool g_enabled = false;
float g_gain = 0.0f;
float g_offset = 0.0f;

bool g_loaded_enabled = false;
bool g_loaded_gain = false;
bool g_loaded_offset = false;

uint32_t FloatBits(const float value) {
  uint32_t bits = 0;
  memcpy(&bits, &value, sizeof(bits));
  return bits;
}

bool FloatEqual(const float lhs, const float rhs) {
  return FloatBits(lhs) == FloatBits(rhs);
}

void ResetLoadedValues() {
  g_enabled = false;
  g_gain = 0.0f;
  g_offset = 0.0f;
  g_loaded_enabled = false;
  g_loaded_gain = false;
  g_loaded_offset = false;
}

int ReadSetting(const char* const name, const size_t len,
                settings_read_cb read_cb, void* const cb_arg,
                const char* const expected_name, void* const value,
                const size_t value_size, bool* const loaded) {
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

  *loaded = true;
  return 0;
}

int StorageHandleSet(const char* const name, const size_t len,
                     settings_read_cb read_cb, void* const cb_arg) {
  int ret = ReadSetting(name, len, read_cb, cb_arg, "enabled", &g_enabled,
                        sizeof(g_enabled), &g_loaded_enabled);
  if (ret != -ENOENT) {
    return ret;
  }

  ret = ReadSetting(name, len, read_cb, cb_arg, "gain", &g_gain,
                    sizeof(g_gain), &g_loaded_gain);
  if (ret != -ENOENT) {
    return ret;
  }

  ret = ReadSetting(name, len, read_cb, cb_arg, "offset", &g_offset,
                    sizeof(g_offset), &g_loaded_offset);
  if (ret != -ENOENT) {
    return ret;
  }

  return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(rdb_storage_test, "rdb_storage_test", nullptr,
                               StorageHandleSet, nullptr, nullptr);

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

int SaveExpectedValues() {
  int ret = settings_save_one("rdb_storage_test/enabled", &kExpectedEnabled,
                              sizeof(kExpectedEnabled));
  if (ret != 0) {
    return ret;
  }

  ret = settings_save_one("rdb_storage_test/gain", &kExpectedGain,
                          sizeof(kExpectedGain));
  if (ret != 0) {
    return ret;
  }

  return settings_save_one("rdb_storage_test/offset", &kExpectedOffset,
                           sizeof(kExpectedOffset));
}

bool ValuesMatchExpected() {
  return g_loaded_enabled && g_loaded_gain && g_loaded_offset &&
         g_enabled == kExpectedEnabled && FloatEqual(g_gain, kExpectedGain) &&
         FloatEqual(g_offset, kExpectedOffset);
}

void PrintValues(const char* const label) {
  printk("%s\n", label);
  printk("  enabled: loaded=%d value=%d expected=%d\n", g_loaded_enabled,
         g_enabled, kExpectedEnabled);
  printk("  gain:    loaded=%d bits=0x%08x expected=0x%08x\n", g_loaded_gain,
         FloatBits(g_gain), FloatBits(kExpectedGain));
  printk("  offset:  loaded=%d bits=0x%08x expected=0x%08x\n", g_loaded_offset,
         FloatBits(g_offset), FloatBits(kExpectedOffset));
}

}  // namespace

int main() {
  const device* const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
  if (!device_is_ready(console)) {
    return 0;
  }

  WaitForConsole(console);

  printk("storage test ready\n");
  printk("using settings/NVS subtree: rdb_storage_test\n");
  printk("BLE also uses settings/NVS, so this test stays under its own subtree\n");

  int ret = settings_subsys_init();
  if (ret != 0) {
    printk("settings_subsys_init failed (%d)\n", ret);
    return 0;
  }

  ResetLoadedValues();
  ret = settings_load_subtree("rdb_storage_test");
  if (ret != 0) {
    printk("initial settings_load_subtree failed (%d)\n", ret);
  }
  PrintValues("before write:");

  ret = SaveExpectedValues();
  if (ret != 0) {
    printk("settings_save_one failed (%d)\n", ret);
    return 0;
  }
  printk("write complete\n");

  ResetLoadedValues();
  ret = settings_load_subtree("rdb_storage_test");
  if (ret != 0) {
    printk("verify settings_load_subtree failed (%d)\n", ret);
    return 0;
  }

  PrintValues("after reload:");
  printk("storage test %s\n", ValuesMatchExpected() ? "PASS" : "FAIL");

  while (true) {
    k_sleep(K_SECONDS(1));
  }
}
