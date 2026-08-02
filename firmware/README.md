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

## Power consomption

- BLE connected, idle: 0.96 mA
- BLE connected, all leds on: 35.75 mA
- BLE advertising, without led: 1.27 mA

