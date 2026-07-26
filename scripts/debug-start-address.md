# Debug notes: boot, CDC, and BLE pairing

Date: 2026-07-26

## Boot address and UF2 layout

- The nice!nano bootloader image in this project uses Adafruit nRF52 UF2 with S140 SoftDevice v6.1.1.
- UICR `NRFFW[0]` at `0x10001014` points to the UF2 bootloader start address `0x000f4000`.
- The bootloader expects the user app at `0x26000`, after MBR + S140:
  - `0x00000..0x01000`: MBR
  - `0x01000..0x26000`: S140 SoftDevice
  - `0x26000..0xec000`: Zephyr app
  - `0xec000..0xf4000`: Zephyr settings/NVS
  - `0xf4000..0x100000`: UF2 bootloader
- The current app build verified this:
  - UF2 start address: `0x26000`
  - vector table at `0x26000`
  - reset handler read from flash at `0x26004`

## Why CDC USB disappeared

CDC did not disappear because the UF2 bootloader rejected the image or because the bootloader jumped to the wrong address.

SWD readback showed:

- Flash at `0x26000` contained a valid app vector table.
- UICR `0x10001014` still contained `0x000f4000`, so the bootloader start address was correct.
- The CPU was running inside the Zephyr app flash range.
- After halting with pyOCD, the PC was inside `k_sys_fatal_error_handler`.
- Fatal reason register argument was `2`, which maps to `K_ERR_STACK_CHK_FAIL` in `zephyr/include/zephyr/fatal_types.h`.

So the app was reaching Zephyr, overflowing a stack, and entering the fatal handler before USB CDC had time to enumerate. From Linux this looked like "no ttyACM app device", but the direct cause was early firmware death.

Fix:

- Increased the risky stacks in `app/prj.conf`:
  - `CONFIG_MAIN_STACK_SIZE=4096`
  - `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048`
  - `CONFIG_BT_RX_STACK_SIZE=2048`
- Rebuilt `APP_BIN=ble`.
- Copied `build/zephyr/zephyr.uf2` to the `NICENANO` bootloader filesystem.
- Verified the app returned as USB device `303a:4010 TeamIO RDB CDC Console` on `/dev/ttyACM1`.

## Why BLE refused to connect

The logs showed:

```text
BLE security failed (level 1, err BT_SECURITY_ERR_AUTH_REQUIREMENT/4)
BLE pairing failed ... reason BT_SECURITY_ERR_AUTH_REQUIREMENT/4
BLE disconnected (reason 0x13)
```

The first BLE security fix relaxed the app-side pairing requirements:

- `CONFIG_BT_SMP_SC_PAIR_ONLY=n`
- `CONFIG_BT_SMP_ENFORCE_MITM=n`
- `CONFIG_BT_SMP_MIN_ENC_KEY_SIZE=7`
- `CONFIG_BT_SECURITY_ERR_TO_STR=y`

This allowed legacy/Just Works style pairing instead of requiring a stronger method the HID test device cannot complete.

After manually removing the device on the host and scanning/adding it again, the same error returned. That was the stale-bond case:

- The host forgot its bond.
- The nRF still had the old bond in NVS because `CONFIG_BT_SETTINGS=y` and `CONFIG_SETTINGS_NVS=y`.
- With bonding enabled, Zephyr rejected a new unauthenticated pairing attempt for a peer that already had an unauthenticated bond stored.
- `CONFIG_BT_SMP_ALLOW_UNAUTH_OVERWRITE` was unset, which Zephyr documents as requiring the old bond to be explicitly deleted with `bt_unpair()`.

Fix:

- Enabled `CONFIG_BT_SMP_ALLOW_UNAUTH_OVERWRITE=y` in `app/prj.conf`.
- Added targeted stale-bond cleanup in `app/src/keyboard/ble.cpp`: on `BT_SECURITY_ERR_AUTH_REQUIREMENT`, call `bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn))`.
- Rebuilt and copied the UF2 through the bootloader filesystem.
- Verified CDC returned and serial output showed the BLE app connected and sending reports.

## Current known-good flashing method

Do not use `scripts/flash_app_swd.sh` for the app image in this bootloader flow.

Use UF2:

```sh
uv run west build -p always -b nrf52840dk/nrf52840 app -- -DAPP_BIN=ble
cp build/zephyr/zephyr.uf2 /media/adjwang/NICENANO/zephyr.uf2
sync
```

If the app is running as CDC and the bootloader volume is not mounted, the 1200-baud touch worked:

```sh
stty -F /dev/ttyACM1 1200 hupcl
```

Then copy the UF2 to the `NICENANO` filesystem.
