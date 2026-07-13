#include "keyboard/keyboard.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/class/hid.h>

extern "C" {
#include "keyboard/ble_hid.h"
}

namespace rdb {

namespace {

hids_report_kb_t make_report(uint8_t key) {
  hids_report_kb_t report = {0};
  if (key == ' ') {
    report.key_pressed[0] = HID_KEY_SPACE;
  } else {
    report.key_pressed[0] = key;
  }
  return report;
}

static bool g_bt_ready = false;

static void restart_advertising(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(adv_restart_work, restart_advertising);

static const struct bt_data ad[] = {
  BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
  BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
                BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sd[] = {
  BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
          sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static bool should_retry_advertising(int err) {
  return err == -EAGAIN || err == -ENOMEM;
}

static void schedule_advertising_retry(int err) {
  printk("BLE advertising retry scheduled (err %d)\n", err);
  k_work_reschedule(&adv_restart_work, K_MSEC(250));
}

static void start_advertising(void) {
  int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd,
                            ARRAY_SIZE(sd));
  if (err == 0) {
    printk("BLE advertising started\n");
  } else if (should_retry_advertising(err)) {
    schedule_advertising_retry(err);
  } else {
    printk("BLE advertising failed (err %d)\n", err);
  }
}

static void restart_advertising(struct k_work *work) {
  ARG_UNUSED(work);
  // Stop old advertising (if any) before starting new advertising, ignore err.
  bt_le_adv_stop();
  int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd,
                            ARRAY_SIZE(sd));
  if (err == 0) {
    printk("BLE advertising restarted\n");
    return;
  }

  if (should_retry_advertising(err)) {
    schedule_advertising_retry(err);
    return;
  }

  printk("BLE advertising restart failed (err %d)\n", err);
}

static void connected(struct bt_conn *conn, uint8_t err) {
  if (err != 0) {
    printk("BLE connect failed (err 0x%02x)\n", err);
    return;
  }

  printk("BLE connected\n");
  hids_connected(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
  ARG_UNUSED(conn);
  printk("BLE disconnected (reason 0x%02x)\n", reason);
  hids_disconnected();

  k_work_cancel_delayable(&adv_restart_work);
  k_work_schedule(&adv_restart_work, K_MSEC(250));
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err) {
  ARG_UNUSED(conn);
  if (err != BT_SECURITY_ERR_SUCCESS) {
    printk("BLE security failed (level %u, err %u)\n", level, err);
    return;
  }

  printk("BLE security changed (level %u)\n", level);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
  .connected = connected,
  .disconnected = disconnected,
  .security_changed = security_changed,
};

static void bt_ready(int err) {
  if (err != 0) {
    printk("BLE init failed (err %d)\n", err);
    return;
  }

  if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
    err = settings_load();
    if (err != 0) {
      printk("BLE settings load failed (err %d)\n", err);
      return;
    }
  }

  g_bt_ready = true;
  printk("BLE initialized\n");

  start_advertising();
}

}  // namespace

class BleKeyboardImpl : public IKeyboard {
 public:
  BleKeyboardImpl(std::string_view device_name,
                  std::string_view device_manufacturer);
  ~BleKeyboardImpl() override = default;
  bool is_connected() const override;
  void Begin() override;
  void End() override;
  void Press(uint8_t key) override;
  void Release(uint8_t key) override;
  void ReleaseAll() override;
  void SetBatteryLevel(uint8_t level) override;

 private:
  std::string_view device_name_;
  std::string_view device_manufacturer_;
  uint8_t battery_level_ = 100;
};

BleKeyboardImpl::BleKeyboardImpl(std::string_view device_name,
                                 std::string_view device_manufacturer)
    : device_name_(device_name), device_manufacturer_(device_manufacturer) {}

bool BleKeyboardImpl::is_connected() const {
  return hids_report_writable();
}

void BleKeyboardImpl::Begin() {
  if (!g_bt_ready) {
    int err = bt_enable(bt_ready);
    if (err != 0) {
      printk("BLE enable failed (err %d)\n", err);
    }
  }
}

void BleKeyboardImpl::End() {
  (void)bt_le_adv_stop();
  g_bt_ready = false;
}

void BleKeyboardImpl::Press(uint8_t key) {
  if (!hids_report_writable()) {
    return;
  }

  const auto report = make_report(key);
  hids_kb_notify_input(&report, sizeof(report));
}

void BleKeyboardImpl::Release(uint8_t key) {
  (void)key;
  if (!hids_report_writable()) {
    return;
  }

  const auto report = make_report(0);
  hids_kb_notify_input(&report, sizeof(report));
}

void BleKeyboardImpl::ReleaseAll() {
  if (!hids_report_writable()) {
    return;
  }

  const auto report = make_report(0);
  hids_kb_notify_input(&report, sizeof(report));
}

void BleKeyboardImpl::SetBatteryLevel(uint8_t level) {
  battery_level_ = level;
  (void)bt_bas_set_battery_level(level);
}

BleKeyboard::BleKeyboard(std::string_view device_name,
                         std::string_view device_manufacturer)
    : impl_(std::make_unique<BleKeyboardImpl>(device_name, device_manufacturer)) {}

BleKeyboard::~BleKeyboard() = default;

bool BleKeyboard::is_connected() const { return impl_->is_connected(); }
void BleKeyboard::Begin() { impl_->Begin(); }
void BleKeyboard::End() { impl_->End(); }
void BleKeyboard::Press(uint8_t key) { impl_->Press(key); }
void BleKeyboard::Release(uint8_t key) { impl_->Release(key); }
void BleKeyboard::ReleaseAll() { impl_->ReleaseAll(); }
void BleKeyboard::SetBatteryLevel(uint8_t level) { impl_->SetBatteryLevel(level); }

}  // namespace rdb
