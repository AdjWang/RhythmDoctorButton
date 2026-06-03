// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/drivers/pwm.h>
// #include <zephyr/drivers/adc.h>
// #include <zephyr/bluetooth/bluetooth.h>
// #include <zephyr/bluetooth/hids.h>

// /* Device specifications from devicetree */
// static const struct gpio_dt_spec key_button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
// static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

// /* ADC setup for battery reading */
// #define ADC_NODE DT_IO_CHANNELS(adc0)
// static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(ADC_NODE, 0);

// /* Battery voltage reading */
// static uint32_t read_battery_mv(void)
// {
//     int err;
//     uint16_t sample_buffer;
//     struct adc_sequence sequence = {
//         .buffer = &sample_buffer,
//         .buffer_size = sizeof(sample_buffer),
//         .calibrate = true,
//     };

//     err = adc_channel_setup_dt(&adc_channel);
//     if (err) return 0;

//     (void)adc_sequence_init_dt(&adc_channel, &sequence);
//     err = adc_read(adc_channel.dev, &sequence);
//     if (err) return 0;

//     int32_t voltage_mv;
//     voltage_mv = sample_buffer;
//     adc_raw_to_millivolts_dt(&adc_channel, &voltage_mv);
    
//     /* Calculate battery percentage (assuming 4.2V full, 3.0V empty) */
//     int32_t battery_pct = (voltage_mv - 3000) * 100 / (4200 - 3000);
//     return CLAMP(battery_pct, 0, 100);
// }

// /* PWM control for LED (brightness) */
// static void set_led_brightness(uint8_t duty_percent)
// {
//     uint32_t pulse_width = (pwm_led.period * duty_percent) / 100;
//     pwm_set_pulse_dt(&pwm_led, pulse_width);
// }

// /* Key press event handling */
// static void button_pressed_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
// {
//     /* Send BLE HID key report */
//     /* Send USB HID key report if connected */
//     /* Blink LED to indicate press */
// }

// /* Main entry point */
// void main(void)
// {
//     int err;

//     /* Initialize button */
//     gpio_pin_configure_dt(&key_button, GPIO_INPUT);
    
//     /* Initialize PWM LED */
//     pwm_set_dt(&pwm_led, PWM_MSEC(20), PWM_USEC(1500));
    
//     /* Initialize BLE and advertise as HID keyboard */
//     err = bt_enable(NULL);
    
//     /* Optional: Enable USB HID stack */
    
//     /* Main loop with power-aware design */
//     while (1) {
//         /* Read battery level every minute */
//         static uint32_t last_battery_read = 0;
//         if (k_uptime_get() - last_battery_read > 60000) {
//             uint32_t battery_level = read_battery_mv();
//             last_battery_read = k_uptime_get();
//         }
        
//         /* Sleep most of the time (Zephyr power management handles deep sleep) */
//         k_sleep(K_MSEC(10));
//     }
// }

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

class Dummy {};

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 500

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void) {
  printk("Hello from nRF52840!\n");

  int ret;
  bool led_state = true;

  if (!gpio_is_ready_dt(&led)) {
    return 0;
  }

  ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    return 0;
  }

  while (1) {
    ret = gpio_pin_toggle_dt(&led);
    if (ret < 0) {
      return 0;
    }

    led_state = !led_state;
    printf("LED state: %s\n", led_state ? "ON" : "OFF");
    k_msleep(SLEEP_TIME_MS);
  }
  return 0;
}
