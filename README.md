
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

## Flash

```
cp build/zephyr/zephyr.uf2 /media/$USER/NICENANO/
```
