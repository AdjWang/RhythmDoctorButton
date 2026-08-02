#include "usb_device.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usbd.h>

namespace {

constexpr uint16_t kUsbVid = 0x303A;
constexpr uint16_t kUsbPid = 0x4001;
constexpr uint8_t kUsbMaxPower = 100 / 2;

USBD_DEVICE_DEFINE(rdb_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), kUsbVid,
                   kUsbPid);
USBD_DESC_LANG_DEFINE(rdb_lang);
USBD_DESC_MANUFACTURER_DEFINE(rdb_mfr, "TeamIO");
USBD_DESC_PRODUCT_DEFINE(rdb_product, "RhythmDoctorButton");
USBD_DESC_CONFIG_DEFINE(rdb_fs_desc, "FS Configuration");
USBD_CONFIGURATION_DEFINE(rdb_fs_config, USB_SCD_REMOTE_WAKEUP, kUsbMaxPower,
                          &rdb_fs_desc);

bool g_usbd_enabled = false;

void UsbMessageCallback(usbd_context* const usbd_ctx,
                        const usbd_msg* const msg) {
  if (usbd_can_detect_vbus(usbd_ctx)) {
    // Disable usb to reduce current if usb is not plugged in.
    if (msg->type == USBD_MSG_VBUS_READY) {
      rdb::EnableUsbDevice(usbd_ctx);
    }
    if (msg->type == USBD_MSG_VBUS_REMOVED) {
      rdb::DisableUsbDevice(usbd_ctx);
    }
  }
}

}  // namespace

namespace rdb {

const device* GetUsbHidDevice() {
  return DEVICE_DT_GET_ONE(zephyr_hid_device);
}

void EnableUsbDevice(usbd_context* const usbd_ctx) {
  if (g_usbd_enabled) {
    return;
  }

  const int ret = usbd_enable(usbd_ctx);
  if (ret != 0) {
    printk("Failed to enable USB device (%d)\n", ret);
    return;
  }

  g_usbd_enabled = true;
}

void DisableUsbDevice(usbd_context* const usbd_ctx) {
  if (!g_usbd_enabled) {
    return;
  }

  const int ret = usbd_disable(usbd_ctx);
  if (ret != 0) {
    printk("Failed to disable USB device (%d)\n", ret);
    return;
  }

  g_usbd_enabled = false;
}

usbd_context* SetupUsbDevice() {
  int ret = usbd_add_descriptor(&rdb_usbd, &rdb_lang);
  if (ret != 0) {
    printk("Failed to add USB language descriptor (%d)\n", ret);
    return nullptr;
  }
  ret = usbd_add_descriptor(&rdb_usbd, &rdb_mfr);
  if (ret != 0) {
    printk("Failed to add USB manufacturer descriptor (%d)\n", ret);
    return nullptr;
  }
  ret = usbd_add_descriptor(&rdb_usbd, &rdb_product);
  if (ret != 0) {
    printk("Failed to add USB product descriptor (%d)\n", ret);
    return nullptr;
  }
  ret = usbd_add_configuration(&rdb_usbd, USBD_SPEED_FS, &rdb_fs_config);
  if (ret != 0) {
    printk("Failed to add USB configuration (%d)\n", ret);
    return nullptr;
  }
  ret = usbd_register_all_classes(&rdb_usbd, USBD_SPEED_FS, 1, nullptr);
  if (ret != 0) {
    printk("Failed to register USB classes (%d)\n", ret);
    return nullptr;
  }
  ret = usbd_device_set_code_triple(&rdb_usbd, USBD_SPEED_FS,
                                    USB_BCC_MISCELLANEOUS, 0x02, 0x01);
  if (ret != 0) {
    printk("Failed to set USB composite device code (%d)\n", ret);
    return nullptr;
  }
  ret = usbd_msg_register_cb(&rdb_usbd, UsbMessageCallback);
  if (ret != 0) {
    printk("Failed to register USB message callback (%d)\n", ret);
    return nullptr;
  }
  ret = usbd_init(&rdb_usbd);
  if (ret != 0) {
    printk("Failed to initialize USB device (%d)\n", ret);
    return nullptr;
  }
  return &rdb_usbd;
}

}  // namespace rdb
