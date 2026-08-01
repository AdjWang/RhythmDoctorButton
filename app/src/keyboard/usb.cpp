#include "keyboard/keyboard.h"
#include "usb_device.h"

#include <array>
#include <cstdint>
#include <memory>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/usb/class/usbd_hid.h>

namespace {

constexpr uint8_t kKeyboardReportSize = 8;

enum ReportIndex {
  kModifierIndex = 0,
  kReservedIndex = 1,
  kKeycodeStartIndex = 2,
};

struct KeyMapping {
  uint8_t modifier;
  uint8_t keycode;
};

const uint8_t kHidKeyboardReportDescriptor[] = HID_KEYBOARD_REPORT_DESC();
bool g_hid_ready = false;
uint32_t g_idle_duration = 0;

KeyMapping MapAsciiToKey(uint8_t key) {
  if (key >= 'a' && key <= 'z') {
    return {HID_KBD_MODIFIER_NONE,
            static_cast<uint8_t>(HID_KEY_A + (key - 'a'))};
  }
  if (key >= 'A' && key <= 'Z') {
    return {HID_KBD_MODIFIER_LEFT_SHIFT,
            static_cast<uint8_t>(HID_KEY_A + (key - 'A'))};
  }
  if (key >= '1' && key <= '9') {
    return {HID_KBD_MODIFIER_NONE,
            static_cast<uint8_t>(HID_KEY_1 + (key - '1'))};
  }
  if (key == '0') {
    return {HID_KBD_MODIFIER_NONE, HID_KEY_0};
  }
  if (key == ' ') {
    return {HID_KBD_MODIFIER_NONE, HID_KEY_SPACE};
  }
  if (key == '\n' || key == '\r') {
    return {HID_KBD_MODIFIER_NONE, HID_KEY_ENTER};
  }

  return {HID_KBD_MODIFIER_NONE, 0};
}

void HidInterfaceReady(const device* dev, bool ready) {
  printk("USB HID %s is %s\n", dev->name, ready ? "ready" : "not ready");
  g_hid_ready = ready;
}

int HidGetReport(const device* dev, uint8_t type, uint8_t id, uint16_t len,
                 uint8_t* const buf) {
  ARG_UNUSED(dev);
  ARG_UNUSED(type);
  ARG_UNUSED(id);
  ARG_UNUSED(len);
  ARG_UNUSED(buf);
  return 0;
}

int HidSetReport(const device* dev, uint8_t type, uint8_t id, uint16_t len,
                 const uint8_t* const buf) {
  ARG_UNUSED(dev);
  ARG_UNUSED(id);
  ARG_UNUSED(len);
  ARG_UNUSED(buf);
  return type == HID_REPORT_TYPE_OUTPUT ? 0 : -ENOTSUP;
}

void HidSetIdle(const device* dev, uint8_t id, uint32_t duration) {
  ARG_UNUSED(dev);
  ARG_UNUSED(id);
  g_idle_duration = duration;
}

uint32_t HidGetIdle(const device* dev, uint8_t id) {
  ARG_UNUSED(dev);
  ARG_UNUSED(id);
  return g_idle_duration;
}

void HidSetProtocol(const device* dev, uint8_t proto) {
  ARG_UNUSED(dev);
  ARG_UNUSED(proto);
}

void HidOutputReport(const device* dev, uint16_t len, const uint8_t* const buf) {
  (void)HidSetReport(dev, HID_REPORT_TYPE_OUTPUT, 0, len, buf);
}

const hid_device_ops kHidOps = {
    .iface_ready = HidInterfaceReady,
    .get_report = HidGetReport,
    .set_report = HidSetReport,
    .set_idle = HidSetIdle,
    .get_idle = HidGetIdle,
    .set_protocol = HidSetProtocol,
    .output_report = HidOutputReport,
};

}  // namespace

namespace rdb {

class UsbKeyboardImpl : public IKeyboard {
 public:
  explicit UsbKeyboardImpl(const device* hid_dev);
  ~UsbKeyboardImpl() override = default;

  bool is_connected() const override;
  void Begin() override;
  void End() override;
  void Press(uint8_t key) override;
  void Release(uint8_t key) override;
  void ReleaseAll() override;
  void SetBatteryLevel(uint8_t level) override;

 private:
  bool initialized_ = false;
  const device* hid_dev_ = nullptr;
  usbd_context* usbd_ctx_ = nullptr;
  std::array<uint8_t, kKeyboardReportSize> report_{};

  void SendReport();
  void AddKeycode(uint8_t keycode);
  void RemoveKeycode(uint8_t keycode);
};

UsbKeyboardImpl::UsbKeyboardImpl(const device* hid_dev) : hid_dev_(hid_dev) {}

bool UsbKeyboardImpl::is_connected() const { return initialized_ && g_hid_ready; }

void UsbKeyboardImpl::Begin() {
  if (initialized_) {
    return;
  }

  if (hid_dev_ == nullptr) {
    printk("USB HID device is not configured\n");
    return;
  }

  if (!device_is_ready(hid_dev_)) {
    printk("USB HID device is not ready\n");
    return;
  }

  int ret = hid_device_register(hid_dev_, kHidKeyboardReportDescriptor,
                                sizeof(kHidKeyboardReportDescriptor), &kHidOps);
  if (ret != 0) {
    printk("Failed to register USB HID device (%d)\n", ret);
    return;
  }

  usbd_ctx_ = rdb::SetupUsbDevice();
  if (usbd_ctx_ == nullptr) {
    printk("Failed to set up USB device\n");
    return;
  }

  // Disable usb to reduce current if usb is not plugged in.
  if (!usbd_can_detect_vbus(usbd_ctx_)) {
    rdb::EnableUsbDevice(usbd_ctx_);
  }

  initialized_ = true;
}

void UsbKeyboardImpl::End() {
  if (!initialized_) {
    return;
  }

  if (usbd_ctx_ != nullptr) {
    rdb::DisableUsbDevice(usbd_ctx_);
  }
  initialized_ = false;
  g_hid_ready = false;
  report_.fill(0);
}

void UsbKeyboardImpl::AddKeycode(uint8_t keycode) {
  if (keycode == 0) {
    return;
  }
  for (const uint8_t current : report_) {
    if (current == keycode) {
      return;
    }
  }
  for (size_t i = kKeycodeStartIndex; i < report_.size(); ++i) {
    if (report_[i] == 0) {
      report_[i] = keycode;
      return;
    }
  }
}

void UsbKeyboardImpl::RemoveKeycode(uint8_t keycode) {
  for (size_t i = kKeycodeStartIndex; i < report_.size(); ++i) {
    if (report_[i] == keycode) {
      report_[i] = 0;
      return;
    }
  }
}

void UsbKeyboardImpl::SendReport() {
  if (!is_connected()) {
    return;
  }

  const int ret =
      hid_device_submit_report(hid_dev_, report_.size(), report_.data());
  if (ret != 0) {
    printk("Failed to submit USB HID report (%d)\n", ret);
  }
}

void UsbKeyboardImpl::Press(uint8_t key) {
  const KeyMapping mapping = MapAsciiToKey(key);
  if (mapping.keycode == 0) {
    return;
  }

  report_[kModifierIndex] = mapping.modifier;
  report_[kReservedIndex] = 0;
  AddKeycode(mapping.keycode);
  SendReport();
}

void UsbKeyboardImpl::Release(uint8_t key) {
  const KeyMapping mapping = MapAsciiToKey(key);
  if (mapping.keycode == 0) {
    return;
  }

  report_[kModifierIndex] = HID_KBD_MODIFIER_NONE;
  RemoveKeycode(mapping.keycode);
  SendReport();
}

void UsbKeyboardImpl::ReleaseAll() {
  report_.fill(0);
  SendReport();
}

void UsbKeyboardImpl::SetBatteryLevel(uint8_t level) { ARG_UNUSED(level); }

UsbKeyboard::UsbKeyboard(const device* hid_dev)
    : impl_(std::make_unique<UsbKeyboardImpl>(hid_dev)) {}

UsbKeyboard::~UsbKeyboard() = default;

bool UsbKeyboard::is_connected() const { return impl_->is_connected(); }
void UsbKeyboard::Begin() { impl_->Begin(); }
void UsbKeyboard::End() { impl_->End(); }
void UsbKeyboard::Press(uint8_t key) { impl_->Press(key); }
void UsbKeyboard::Release(uint8_t key) { impl_->Release(key); }
void UsbKeyboard::ReleaseAll() { impl_->ReleaseAll(); }
void UsbKeyboard::SetBatteryLevel(uint8_t level) {
  impl_->SetBatteryLevel(level);
}

}  // namespace rdb
