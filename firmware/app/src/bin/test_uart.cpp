#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console),
                                zephyr_cdc_acm_uart),
             "Console device must be a USB CDC ACM UART");

int main() {
  const device* const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
  if (!device_is_ready(console)) {
    return 0;
  }

  uint32_t dtr = 0;
  while (dtr == 0) {
    (void)uart_line_ctrl_get(console, UART_LINE_CTRL_DTR, &dtr);
    k_sleep(K_MSEC(100));
  }

  (void)uart_line_ctrl_set(console, UART_LINE_CTRL_DCD, 1);
  (void)uart_line_ctrl_set(console, UART_LINE_CTRL_DSR, 1);
  k_sleep(K_MSEC(100));

  uint32_t count = 0;
  while (true) {
    printk("uart test alive %u\n", count++);
    k_sleep(K_SECONDS(1));
  }
}
