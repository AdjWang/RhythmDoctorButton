#include "device/usbd.h"
#include "keyboard/keyboard.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <memory>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "tinyusb.h"

namespace rdb {

namespace {
static const char* TAG = "USB";
static const char kStringLangId[] = {0x09, 0x04};
static char kSerialString[13] = "000000000000";
static const char* kStringDescriptors[] = {
    kStringLangId,  // LANGID
    nullptr,        // manufacturer name
    nullptr,        // device name
    kSerialString,  // serial number string
};

static void PopulateDevInfo(const char* device_name,
                            const char* device_manufacturer) {
  kStringDescriptors[1] = device_manufacturer;
  kStringDescriptors[2] = device_name;
}

static void PopulateUsbSerialString() {
  uint8_t mac[6] = {0};
  if (esp_read_mac(mac, ESP_MAC_EFUSE_FACTORY) != ESP_OK) {
    ESP_LOGW(TAG, "Failed to read efuse MAC for USB serial");
    return;
  }
  snprintf(kSerialString, sizeof(kSerialString), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

enum {
  ITF_NUM_KEYBOARD = 0,
  ITF_NUM_TOTAL = 1,
};

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

static const tusb_desc_device_t kDeviceDescriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x303A,
    .idProduct = 0x4001,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

static const uint8_t kAsciiToKeycode[128][2] = {HID_ASCII_TO_KEYCODE};

}  // namespace

// HID Report Descriptor - declared at namespace scope for access in callbacks
static const uint8_t kHidKeyboardReportDescriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

namespace {

static const uint8_t kConfigurationDescriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_KEYBOARD, 0, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(kHidKeyboardReportDescriptor), 0x81,
                       CFG_TUD_HID_EP_BUFSIZE, 10),
};

}  // namespace

class UsbKeyboardImpl : public IKeyboard {
 public:
  UsbKeyboardImpl(std::string_view device_name,
                  std::string_view device_manufacturer);
  ~UsbKeyboardImpl() override;
  bool is_connected() const override;
  void Begin() override;
  void End() override;
  void Press(uint8_t key) override;
  void Release(uint8_t key) override;
  void ReleaseAll() override;
  void SetBatteryLevel(uint8_t /*lvl*/) override;

 private:
  const std::string device_name_;
  const std::string device_manufacturer_;
  bool installed_ = false;
  std::array<uint8_t, 6> keycodes_{};

  void SendReport();
  void AddKeycode(uint8_t keycode);
  void RemoveKeycode(uint8_t keycode);
};

UsbKeyboardImpl::UsbKeyboardImpl(std::string_view device_name,
                                 std::string_view device_manufacturer)
    : device_name_(device_name), device_manufacturer_(device_manufacturer) {}

UsbKeyboardImpl::~UsbKeyboardImpl() {
  End();
}

bool UsbKeyboardImpl::is_connected() const {
  return installed_ && tud_ready();
}

void UsbKeyboardImpl::Begin() {
  if (installed_) {
    return;
  }
  PopulateDevInfo(device_name_.c_str(), device_manufacturer_.c_str());
  PopulateUsbSerialString();
  ESP_LOGI(TAG, "Descriptor manufacturer=%s", kStringDescriptors[1]);
  ESP_LOGI(TAG, "Descriptor product=%s", kStringDescriptors[2]);
  ESP_LOGI(TAG, "Descriptor serial=%s", kStringDescriptors[3]);
  tinyusb_config_t config = {};
  config.port = TINYUSB_PORT_FULL_SPEED_0;
  config.phy.skip_setup = false;
  config.phy.self_powered = false;
  config.phy.vbus_monitor_io = -1;
  config.task.size = 8192;
  config.task.priority = 5;
  config.task.xCoreID = 0;
  config.descriptor.device = &kDeviceDescriptor;
  config.descriptor.qualifier = nullptr;
  config.descriptor.string = kStringDescriptors;
  config.descriptor.string_count = sizeof(kStringDescriptors) / sizeof(kStringDescriptors[0]);
  config.descriptor.full_speed_config = kConfigurationDescriptor;
  config.descriptor.high_speed_config = nullptr;
  esp_err_t err = tinyusb_driver_install(&config);
  installed_ = (err == ESP_OK);
  if (!installed_) {
    ESP_LOGE(TAG, "TinyUSB driver install failed: %d", err);
  }
}

void UsbKeyboardImpl::End() {
  if (!installed_) {
    return;
  }

  esp_err_t err = tinyusb_driver_uninstall();
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "TinyUSB driver uninstall failed: %d", err);
  }
  installed_ = false;
  keycodes_.fill(0);
}

void UsbKeyboardImpl::AddKeycode(uint8_t keycode) {
  if (keycode == 0) {
    return;
  }
  for (uint8_t current : keycodes_) {
    if (current == keycode) {
      return;
    }
  }
  for (uint8_t &slot : keycodes_) {
    if (slot == 0) {
      slot = keycode;
      return;
    }
  }
}

void UsbKeyboardImpl::RemoveKeycode(uint8_t keycode) {
  for (uint8_t &slot : keycodes_) {
    if (slot == keycode) {
      slot = 0;
      return;
    }
  }
}

void UsbKeyboardImpl::SendReport() {
  if (!installed_ || !tud_hid_ready()) {
    return;
  }
  tud_hid_keyboard_report(0, 0, keycodes_.data());
}

void UsbKeyboardImpl::Press(uint8_t key) {
  if (!installed_) {
    return;
  }

  if (key >= sizeof(kAsciiToKeycode) / sizeof(kAsciiToKeycode[0])) {
    return;
  }

  uint8_t modifier = kAsciiToKeycode[key][0] ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
  uint8_t keycode = kAsciiToKeycode[key][1];
  if (keycode == 0) {
    return;
  }

  AddKeycode(keycode);
  if (modifier != 0) {
    // Keep the report simple, only single modifier supported.
    tud_hid_keyboard_report(0, modifier, keycodes_.data());
  } else {
    SendReport();
  }
}

void UsbKeyboardImpl::Release(uint8_t key) {
  if (!installed_) {
    return;
  }

  if (key >= sizeof(kAsciiToKeycode) / sizeof(kAsciiToKeycode[0])) {
    return;
  }

  uint8_t keycode = kAsciiToKeycode[key][1];
  if (keycode == 0) {
    return;
  }

  RemoveKeycode(keycode);
  SendReport();
}

void UsbKeyboardImpl::ReleaseAll() {
  if (!installed_) {
    return;
  }

  keycodes_.fill(0);
  SendReport();
}

void UsbKeyboardImpl::SetBatteryLevel(uint8_t /*lvl*/) {
  // No battery service implemented for USB HID.
}

UsbKeyboard::UsbKeyboard(std::string_view device_name,
                         std::string_view device_manufacturer)
    : impl_(std::make_unique<UsbKeyboardImpl>(device_name,
                                              device_manufacturer)) {}
UsbKeyboard::~UsbKeyboard() {}
bool UsbKeyboard::is_connected() const { return impl_->is_connected(); }
void UsbKeyboard::Begin() { impl_->Begin(); }
void UsbKeyboard::End() { impl_->End(); }
void UsbKeyboard::Press(uint8_t key) { impl_->Press(key); }
void UsbKeyboard::Release(uint8_t key) { impl_->Release(key); }
void UsbKeyboard::ReleaseAll() { impl_->ReleaseAll(); }
void UsbKeyboard::SetBatteryLevel(uint8_t lvl) { impl_->SetBatteryLevel(lvl); }

}  // namespace rdb

// HID Report Descriptor - declared at global scope for access in C callbacks
static const uint8_t kHidKeyboardReportDescriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// TinyUSB HID Callbacks (must have C linkage)
extern "C" {

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type, uint8_t* buffer,
                                uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  // For keyboard, we don't typically respond to GET_REPORT for output reports
  // Just return 0 as we don't have state to report
  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type, uint8_t const* buffer,
                            uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)bufsize;

  // For keyboard set_report typically contains LED state (Caps Lock, Num Lock, Scroll Lock)
  if (report_type == HID_REPORT_TYPE_OUTPUT) {
    // buffer[0] contains LED state
    // bit 0: Num Lock
    // bit 1: Caps Lock
    // bit 2: Scroll Lock
    if (bufsize > 0) {
      // Here you could handle LED state if needed
      // For now, we just acknowledge receipt
    }
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;

  return kHidKeyboardReportDescriptor;
}

}  // extern "C"
