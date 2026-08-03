
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

When flashing through SWD, do not program `build/zephyr/zephyr.hex` directly.
For this UF2/SoftDevice layout, use an app-only HEX shifted to `0x26000`:

```
scripts/flash_app_swd.sh
```

The helper defaults to a conservative 100 kHz SWD clock. If CMSIS-DAP reports
repeated `Connection timed out` errors, reset the USB probe with
`usbreset c251:f001` or unplug/replug the DAPLink probe before retrying.

If the SoftDevice/bootloader area was accidentally overwritten, restore it and
then flash the app in one step:

```
RESTORE_BOOTLOADER=1 scripts/flash_app_swd.sh
```

## Flash bootloader

```
pyocd flash -t nrf52840 -f 1M -e chip --format hex scripts/nice_nano_bootloader-0.11.0_s140_6.1.1.hex
```

## Power consomption

- BLE connected, idle: 0.96 mA
- BLE connected, all leds on: 35.75 mA
- BLE advertising, without led: 1.27 mA

