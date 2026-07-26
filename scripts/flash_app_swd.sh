#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build}"
app_offset="${APP_OFFSET:-0x26000}"
bootloader_hex="${BOOTLOADER_HEX:-scripts/nice_nano_bootloader-0.11.0_s140_6.1.1.hex}"
swd_khz="${SWD_KHZ:-100}"
app_bin="${build_dir}/zephyr/zephyr.bin"

if [[ ! -f "${app_bin}" ]]; then
  echo "Missing ${app_bin}. Build first, for example:"
  echo "  uv run west build -p always -b promicro_nrf52840/nrf52840/uf2 app -- -DAPP_BIN=uart"
  exit 1
fi

if [[ "${RESTORE_BOOTLOADER:-0}" == "1" ]]; then
  pyocd flash -t nrf52840 -f "${swd_khz}k" -e chip --format hex "${bootloader_hex}"
fi

pyocd flash -t nrf52840 -f "${swd_khz}k" -e sector --format bin \
  --base-address "${app_offset}" --no-reset "${app_bin}"

pyocd commander -t nrf52840 -f "${swd_khz}k" \
  -c 'write32 0x4000051c 0x0000006d' \
  -c 'write32 0x40000520 0x00000000' \
  -c 'reset' \
  -c 'quit'
