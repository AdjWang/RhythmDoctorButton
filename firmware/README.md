# Firmware

## Setup environment

Guide: https://docs.zephyrproject.org/4.4.0/develop/getting_started/index.html

```
uv init
uv add west
uv run west update
uv run west packages pip | xargs uv pip install
```

## Build

```
uv run west build -p always -b nrf52840dk/nrf52840 app -- -DAPP_BIN=main
```

Tests:

```
uv run west build -p always -b nrf52840dk/nrf52840 app -- -DAPP_BIN=uart
uv run west build -p always -b nrf52840dk/nrf52840 app -- -DAPP_BIN=cmd
uv run west build -p always -b nrf52840dk/nrf52840 app -- -DAPP_BIN=storage
uv run west build -p always -b nrf52840dk/nrf52840 app -- -DAPP_BIN=pwm_led
uv run west build -p always -b nrf52840dk/nrf52840 app -- -DAPP_BIN=key
uv run west build -p always -b nrf52840dk/nrf52840 app -- -DAPP_BIN=adc
uv run west build -p always -b nrf52840dk/nrf52840 app -- -DAPP_BIN=usb
uv run west build -p always -b nrf52840dk/nrf52840 app -- -DAPP_BIN=ble
```

## Flash program

```
cp build/zephyr/zephyr.uf2 /media/$USER/NICENANO/
```

## Flash bootloader

```
pyocd flash -t nrf52840 -f 1M -e chip --format hex scripts/nice_nano_bootloader-0.11.0_s140_6.1.1.hex
```

## Power consumption

- BLE connected, idle: 0.96 mA
- BLE connected, all leds on: 35.75 mA
- BLE advertising, without led: 1.27 mA

## Persisted settings

Uart on usb cdc is enabled to set arguments through console. Input `key = value` then enter to new line to accept.

Available settings:

- enable_led = true/false
- key_led_brightness = 0.0~1.0
- bkg_led_brightness = 0.0~1.0
- factory_reset = true

Default value:

- enable_led = true
- key_led_brightness = 0.2
- bkg_led_brightness = 0.08


> Adjust led brightness would change default power consumption.

Example outputs:

- Empty line:

    ```
    cmd parse error: code=cmd_empty_line message=empty command at position 0 in ''
    ```

- Set value:

    ```
    enable_led=false
    saved command: key=enable_led value=false
    ```
