
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
west build -p always -b nrf52840dk/nrf52840 app
```

## Flash program

```
cp build/zephyr/zephyr.uf2 /media/$USER/NICENANO/
```

## Flash bootloader

```
nrfutil device recover
nrfutil device program --firmware ./scripts/nice_nano_bootloader-0.6.0_s140_6.1.1.hex
```

> Install [nrfutil](https://www.nordicsemi.com/Products/Development-tools/nRF-Util/Download)

> Install tools: `nrfutil install device`

> List jlink devices: `nrfutil device list`
