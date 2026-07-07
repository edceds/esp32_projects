# temp_humid_sensor

Read temperature and humidity from a **DHT22 (AM2302)** sensor on an ESP32,
print the readings to the serial log, show them on a **16x2 I2C LCD**, and serve
them on a **live web page** over the local network — all every 2 seconds.

The DHT22 uses a single-wire timing protocol with no ESP-IDF driver, so
[main/temp_humid_sensor.cpp](main/temp_humid_sensor.cpp) bit-bangs the GPIO
directly — no external components. The pin is accessed through a typed wrapper
in [main/gpio_pin.hpp](main/gpio_pin.hpp), so picking an invalid pin is a
**compile-time** error (see below).

## Wiring

**DHT22:**

| DHT22 pin | Connect to |
|-----------|-----------------------------------|
| 1 (VCC)   | 3V3                               |
| 2 (DATA)  | GPIO32 + a 10 kΩ pull-up to 3V3   |
| 3 (NC)    | — (not connected)                 |
| 4 (GND)   | GND                               |

**LCD 16x2 (módulo I2C / PCF8574):**

| LCD pin | Connect to |
|---------|-------------------------------------------|
| VCC     | **5V (VIN)** — não 3V3, senão fica apagado |
| GND     | GND                                       |
| SDA     | GPIO26                                     |
| SCL     | GPIO27                                     |

O pull-up de 10 kΩ no DATA do DHT22 é recomendado (sem ele o código usa o
pull-up interno + retry; veja abaixo). Para trocar pinos, edite as constantes
`DHT_GPIO`, `LCD_SDA`, `LCD_SCL` no topo de
[main/temp_humid_sensor.cpp](main/temp_humid_sensor.cpp).

### Endereço I2C do LCD

Esses módulos são quase sempre `0x27` ou `0x3F`. No boot o firmware **escaneia o
barramento e loga o endereço encontrado**:

```
I (xxx) i2c: escaneando barramento (SDA=26 SCL=27)...
I (xxx) i2c:   dispositivo encontrado em 0x27
```

Se o scan achar `0x3F` (ou outro), ajuste `LCD_ADDR` no topo do `.cpp`. Se o LCD
não iniciar, o log avisa e lembra de conferir o VCC em 5V.

## Página web (rede local)

Ao ligar, o ESP32 conecta no WiFi e sobe um servidor HTTP. Abra o **IP dele** no
navegador (celular ou PC **na mesma rede**) e veja temperatura e umidade **ao
vivo** — a página se atualiza sozinha a cada 2s, sem F5. Nada sai pra internet.

Descubra o IP de dois jeitos:

- No **LCD**: logo após conectar, ele mostra `Abra no browser:` e o IP.
- No **log serial**: `I (xxx) wifi: conectado — IP: 192.168.x.x`.

Rotas do servidor:

| Rota | Retorna |
|------|---------|
| `/` | a página HTML (dashboard ao vivo) |
| `/data` | JSON: `{"temperature":19.4,"humidity":75.2,"uptime_s":123,"valid":true}` |

O WiFi (SSID/senha) está em [main/wifi.hpp](main/wifi.hpp); a página e as rotas em
[main/web.hpp](main/web.hpp). O `uptime_s` é o tempo desde o boot — o ESP32 não
sabe a hora real sem NTP (que exigiria... bom, já tem WiFi, dá pra adicionar
depois se quiser data/hora de verdade).

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
