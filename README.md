
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
uv run west build -p always -b promicro_nrf52840/nrf52840/uf2 app -- -DAPP_BIN=main
uv run west build -p always -b promicro_nrf52840/nrf52840/uf2 app -- -DAPP_BIN=uart
uv run west build -p always -b promicro_nrf52840/nrf52840/uf2 app -- -DAPP_BIN=pwm_led
uv run west build -p always -b promicro_nrf52840/nrf52840/uf2 app -- -DAPP_BIN=key
uv run west build -p always -b promicro_nrf52840/nrf52840/uf2 app -- -DAPP_BIN=adc
uv run west build -p always -b promicro_nrf52840/nrf52840/uf2 app -- -DAPP_BIN=usb
uv run west build -p always -b promicro_nrf52840/nrf52840/uf2 app -- -DAPP_BIN=ble
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

<!-- The factory bootloader currently reports:

```
UF2 Bootloader 0.6.0
Model: nice!nano
Board-ID: nRF52840-nicenano
SoftDevice: not found
Date: Jun 19 2021
```

Normal UF2 flashing does not use the bootloader file in `scripts/`. Because
this factory bootloader reports `SoftDevice: not found`, the app is linked with
`CONFIG_FLASH_LOAD_OFFSET=0x1000`.

Only run these commands when intentionally replacing or recovering the
bootloader:

```
nrfutil device recover
nrfutil device program --firmware ./scripts/nice_nano_bootloader-0.6.0_s140_6.1.1.hex
```

> Install [nrfutil](https://www.nordicsemi.com/Products/Development-tools/nRF-Util/Download)

> Install tools: `nrfutil install device`

> List jlink devices: `nrfutil device list` -->

```
pyocd flash -t nrf52840 -f 1M -e chip --format hex scripts/nice_nano_bootloader-0.11.0_s140_6.1.1.hex
```
