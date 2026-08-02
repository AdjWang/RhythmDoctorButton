#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <zephyr/usb/usbd.h>

struct device;

namespace rdb {

const device* GetUsbHidDevice();
usbd_context* SetupUsbDevice();
void EnableUsbDevice(usbd_context* usbd_ctx);
void DisableUsbDevice(usbd_context* usbd_ctx);

}  // namespace rdb

#endif
