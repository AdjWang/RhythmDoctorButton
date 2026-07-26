# Debug Report 2: `test_uart.cpp`, SoftDevice Corruption, and CMSIS-DAP Recovery

Date: 2026-07-26

## Project

Project path:

```text
/home/adjwang/projects/RhythmDoctorButton-nrf52840
```

Target board:

```text
promicro_nrf52840/nrf52840/uf2
```

Physical board and bootloader layout are the same as the previous
`nRF52840_Examples/HelloWorld` debugging session:

- nice!nano / Adafruit nRF52 bootloader
- S140 SoftDevice v6.1.1
- Zephyr application starts at `0x26000`
- UF2 bootloader starts at `0xf4000`

## Original Symptom

`test_uart.cpp` was built and flashed, but the expected Zephyr CDC ACM serial
device did not show as `/dev/ttyACM1`.

Only the DAPLink/CMSIS-DAP adapter was visible:

```text
/dev/ttyACM0
usb-jixin.pro_CMSIS-DAP_LU_LU_2022_8888-if00 -> ../../ttyACM0
```

Expected app USB device:

```text
303a:4010 TeamIO RDB CDC Console
/dev/ttyACM1
```

## Source Fix in `test_uart.cpp`

`app/src/bin/test_uart.cpp` was updated to make the UART test easier to debug
on this same board.

Changes:

- clear `RESETREAS` early so the nice!nano bootloader's reset/double-reset
  logic is not confused by stale reset reasons
- pulse and toggle P0.02 as a diagnostic LED
- use explicit `const struct device *` for the Zephyr console device
- keep waiting for host DTR before printing

Important code added:

```cpp
#include <hal/nrf_power.h>

void ClearResetReason() {
  const uint32_t reset_reason = nrf_power_resetreas_get(NRF_POWER);
  nrf_power_resetreas_clear(NRF_POWER, reset_reason);
}
```

P0.02 diagnostic LED:

```cpp
constexpr gpio_dt_spec kLed = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    .pin = 2,
    .dt_flags = GPIO_ACTIVE_HIGH,
};
```

The test prints once the host opens the CDC ACM port and DTR becomes high:

```text
uart test alive N
```

## Build Verification

Built the UART target with:

```sh
uv run west build -p always -b promicro_nrf52840/nrf52840/uf2 app -- -DAPP_BIN=uart
```

The build succeeded and generated:

```text
Converted to uf2, output size: 324096, start address: 0x26000
```

Generated config confirmed:

```text
CONFIG_FLASH_LOAD_OFFSET=0x26000
CONFIG_USE_DT_CODE_PARTITION=y
CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL=y
CONFIG_USB_DEVICE_STACK_NEXT=y
CONFIG_CDC_ACM_SERIAL_ENABLE_AT_BOOT=y
CONFIG_CDC_ACM_SERIAL_VID=0x303A
CONFIG_CDC_ACM_SERIAL_PID=0x4010
```

## Critical Flashing Mistake Found

The main issue was flashing this file over SWD:

```text
build/zephyr/zephyr.hex
```

In this project, that HEX file contains low-address records. Programming it
over SWD overwrote/corrupted the MBR/SoftDevice area.

Evidence from inspecting the generated HEX address ranges:

```text
0x00006000-0x00010000
0x00000000-0x00010000
0x00000000-0x0000af84
0x0000af90-0x0000d8c8
```

Those ranges are not safe for the nice!nano/S140 bootloader layout because the
SoftDevice metadata lives near `0x3004`.

## Corruption Evidence on the Board

After `/dev/ttyACM1` disappeared again, flash was read through pyOCD:

```sh
pyocd commander -t nrf52840 \
  -c 'read32 0x00003004' \
  -c 'read32 0x00003008' \
  -c 'read32 0x10001014' \
  -c 'quit'
```

Observed bad values:

```text
00003004:  bf00bd08
00003008:  20001a34
10001014:  000f4000
```

Expected SoftDevice metadata:

```text
0x00003004: 51b1e5db
0x00003008: 00026000
```

Interpretation:

- UICR still pointed to the bootloader at `0xf4000`
- SoftDevice metadata was corrupted
- the bootloader/SoftDevice layout could no longer reliably start the app
- therefore the Zephyr CDC ACM app did not enumerate as `/dev/ttyACM1`

## CMSIS-DAP Failure

While trying to recover with OpenOCD, the DAPLink/CMSIS-DAP clone also became
unstable.

OpenOCD failed before reaching the nRF52840 target:

```text
Error: error writing data: Connection timed out
Error: CMSIS-DAP command CMD_INFO failed.
```

This is a probe USB/debug-link failure, not an application firmware failure.

`lsusb` still showed the probe:

```text
c251:f001 Keil Software, Inc. CMSIS-DAP_LU
```

But OpenOCD could not claim or communicate with it.

The probe was reset from Linux with:

```sh
usbreset c251:f001
```

After that, pyOCD could see the probe again:

```text
jixin.pro CMSIS-DAP_LU   LU_2022_8888
```

OpenOCD remained unreliable with this probe, so recovery was switched to pyOCD.

## Working Recovery Commands

The board was recovered with pyOCD at a conservative 100 kHz SWD clock.

Restore nice!nano bootloader plus S140 SoftDevice:

```sh
pyocd flash -t nrf52840 -f 100k -e chip --format hex \
  scripts/nice_nano_bootloader-0.11.0_s140_6.1.1.hex
```

Flash only the Zephyr application binary to the correct app start address:

```sh
pyocd flash -t nrf52840 -f 100k -e sector --format bin \
  --base-address 0x26000 \
  --no-reset \
  build/zephyr/zephyr.bin
```

Set the nice!nano bootloader one-shot skip flag and reset into the app:

```sh
pyocd commander -t nrf52840 -f 100k \
  -c 'write32 0x4000051c 0x0000006d' \
  -c 'write32 0x40000520 0x00000000' \
  -c 'reset' \
  -c 'quit'
```

The `0x6d` value is the Adafruit/nice!nano bootloader's `DFU_MAGIC_SKIP`
one-shot flag. It tells the bootloader to skip DFU mode and start the app.

## Final Working Result

After recovery, USB showed the expected app device:

```text
Bus 001 Device 022: ID 303a:4010 TeamIO RDB CDC Console
/dev/serial/by-id/usb-TeamIO_RDB_CDC_Console_9D2798A31B1E8F69-if00 -> ../../ttyACM1
/dev/ttyACM1
```

Opening `/dev/ttyACM1` confirmed the app was running:

```text
*** Booting Zephyr OS build v4.4.0-4154-geb3a4f3bf113 ***

uart test alive 0
uart test alive 1
uart test alive 2
uart test alive 3
```

## Helper Script Added

Added:

```text
scripts/flash_app_swd.sh
```

The script now uses pyOCD instead of OpenOCD by default:

```sh
pyocd flash -t nrf52840 -f "${swd_khz}k" -e sector --format bin \
  --base-address "${app_offset}" --no-reset "${app_bin}"
```

Defaults:

```text
APP_OFFSET=0x26000
SWD_KHZ=100
```

Normal app flash:

```sh
scripts/flash_app_swd.sh
```

Full bootloader/SoftDevice restore plus app flash:

```sh
RESTORE_BOOTLOADER=1 scripts/flash_app_swd.sh
```

If the CMSIS-DAP probe times out:

```sh
usbreset c251:f001
```

or physically unplug/replug the DAPLink probe.

## README Updates

`README.md` was updated to use the correct board target:

```sh
uv run west build -p always -b promicro_nrf52840/nrf52840/uf2 app -- -DAPP_BIN=uart
```

It also now warns:

- do not SWD-flash `build/zephyr/zephyr.hex`
- use UF2 copy or `scripts/flash_app_swd.sh`
- use `RESTORE_BOOTLOADER=1 scripts/flash_app_swd.sh` if the SoftDevice area
  was corrupted

## Conclusion

`/dev/ttyACM1` disappeared because the SoftDevice metadata at `0x3004` and
`0x3008` was corrupted by flashing the wrong HEX image over SWD.

The UART test application itself works when:

- the nice!nano/S140 bootloader is intact
- the app is placed at `0x26000`
- the app clears `RESETREAS`
- the bootloader is given the one-shot `DFU_MAGIC_SKIP` reset when needed

For SWD flashing on this board, the reliable path is:

```sh
RESTORE_BOOTLOADER=1 scripts/flash_app_swd.sh
```

