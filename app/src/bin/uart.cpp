#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usbd.h>

namespace {

constexpr uint16_t kUsbVid = 0x303A;
constexpr uint16_t kUsbPid = 0x4010;
constexpr uint8_t kUsbMaxPower = 100 / 2;

USBD_DEVICE_DEFINE(test_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
                   kUsbVid, kUsbPid);
USBD_DESC_LANG_DEFINE(test_lang);
USBD_DESC_MANUFACTURER_DEFINE(test_mfr, "TeamIO");
USBD_DESC_PRODUCT_DEFINE(test_product, "RDB UART Test");
USBD_DESC_CONFIG_DEFINE(test_fs_desc, "FS Configuration");
USBD_CONFIGURATION_DEFINE(test_fs_config, 0, kUsbMaxPower, &test_fs_desc);

bool InitUsbCdc() {
  int ret = usbd_add_descriptor(&test_usbd, &test_lang);
  ret |= usbd_add_descriptor(&test_usbd, &test_mfr);
  ret |= usbd_add_descriptor(&test_usbd, &test_product);
  ret |= usbd_add_configuration(&test_usbd, USBD_SPEED_FS, &test_fs_config);
  ret |= usbd_register_class(&test_usbd, "cdc_acm_0", USBD_SPEED_FS, 1);
  ret |= usbd_init(&test_usbd);
  ret |= usbd_enable(&test_usbd);
  return ret == 0;
}

}  // namespace

int main() {
  if (!InitUsbCdc()) {
    return 0;
  }

  uint32_t counter = 0;
  while (true) {
    printk("uart test alive %u\n", counter++);
    k_msleep(1000);
  }
}
