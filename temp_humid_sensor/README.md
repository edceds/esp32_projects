# temp_humid_sensor

Read temperature and humidity from a **DHT22 (AM2302)** sensor on an ESP32 and
print the readings to the serial log every 2 seconds.

The DHT22 uses a single-wire timing protocol with no ESP-IDF driver, so
[main/temp_humid_sensor.cpp](main/temp_humid_sensor.cpp) bit-bangs the GPIO
directly — no external components. The pin is accessed through a typed wrapper
in [main/gpio_pin.hpp](main/gpio_pin.hpp), so picking an invalid pin is a
**compile-time** error (see below).

## Wiring

| DHT22 pin | Connect to |
|-----------|-----------------------------------|
| 1 (VCC)   | 3V3                               |
| 2 (DATA)  | GPIO32 + a 10 kΩ pull-up to 3V3   |
| 3 (NC)    | — (not connected)                 |
| 4 (GND)   | GND                               |

The 10 kΩ pull-up on the data line is required. To use a different pin, change
`DHT_GPIO` at the top of [main/temp_humid_sensor.cpp](main/temp_humid_sensor.cpp).

## Type-safe pins

`DHT_GPIO` is a template parameter of `pin::OpenDrainPin` in
[main/gpio_pin.hpp](main/gpio_pin.hpp), which `static_assert`s that the pin can
drive an output. If you set it to an input-only pin (GPIO34–39) the **build
fails** instead of throwing a runtime error like
`gpio_set_level(240): GPIO output gpio_num error`:

```
error: static assertion failed: OpenDrainPin: this GPIO cannot drive an
output. On the ESP32, GPIO34-39 are input-only ...
```

The wrapper also gives input-only pins no `set()`/`low()`/`high()` method at
all, so you can't accidentally drive a read-only pin — the compiler (and your
editor via clangd) flags it as you type.

## Build, flash, monitor

First activate the ESP-IDF environment in your shell (see
[../docs/setup.md](../docs/setup.md)):

```sh
source ~/.espressif/tools/activate_idf_v5.5.3.sh
```

Then, from this directory:

```sh
idf.py build          # compile
idf.py flash monitor  # flash the board and watch the serial output
```

Target is **esp32** (classic). Exit the monitor with `Ctrl-]`.

## Expected output

```
I (2345) dht22: Temperature: 23.4 C   Humidity: 48.2 %
I (4350) dht22: Temperature: 23.4 C   Humidity: 48.3 %
```

If you see `no response` or `checksum mismatch` warnings, check the wiring and
that the 10 kΩ pull-up is in place.

## Docs

General ESP32/IDF notes (setup, workflow, troubleshooting, hardware) live at the
repo root: [../docs/](../docs/) and [../README.md](../README.md).
