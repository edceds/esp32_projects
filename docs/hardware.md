# Hardware notes

## Board on hand

| Property | Value |
|----------|-------|
| Chip | ESP32-D0WD-V3 (classic ESP32) |
| Revision | v3.1 |
| Cores | Dual core, 240 MHz |
| Radio | Wi-Fi + Bluetooth |
| Crystal | 40 MHz |
| USB-UART | shows up as `/dev/cu.usbserial-*` (a CP210x/CH340-class bridge) |
| `set-target` | `esp32` |

Detected with:

```sh
esptool.py --port /dev/cu.usbserial-5B153806091 chip_id
```

## Serial port

Current board enumerates as:

```
/dev/cu.usbserial-5B153806091
```

Find it any time with:

```sh
ls /dev/cu.usbserial-*
```

Use `-p` to target it explicitly when auto-detection is ambiguous:

```sh
idf.py -p /dev/cu.usbserial-5B153806091 flash monitor
```

If the port never appears: try a different USB cable (some are charge-only), and
make sure a CP210x or CH340 driver is installed for the USB-UART bridge.

## GPIO / wiring notes

- **Default log/console UART is UART0** — the same pins used for
  flashing/monitor. Avoid repurposing them.
- **Strapping pins** (GPIO0, 2, 5, 12, 15) affect boot mode; be careful driving
  them at reset. GPIO12 in particular can brick boot if held high.
- **Input-only pins:** GPIO34–39 have no output drivers and no internal
  pull-ups/downs — fine for sensors/buttons, not for outputs. Calling
  `gpio_set_level()` on one fails at runtime with
  `gpio_set_level(...): GPIO output gpio_num error`. In C++ you can turn that
  into a compile error — see the typed pin wrapper in
  [../temp_humid_sensor/main/gpio_pin.hpp](../temp_humid_sensor/main/gpio_pin.hpp)
  (`static_assert(GPIO_IS_VALID_OUTPUT_GPIO(PIN), ...)`).
- Prefer plain GPIOs like **GPIO4, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32,
  33** for general use.

### Per-project wiring

Wiring for a specific sensor lives in that project's README, e.g.
[../temp_humid_sensor/README.md](../temp_humid_sensor/README.md) for the DHT22
(data on GPIO32 with a 10 kΩ pull-up).

## Identificando resistores (código de cores)

Cada faixa colorida tem um papel:

- **1ª faixa** = primeiro dígito
- **2ª faixa** = segundo dígito
- **3ª faixa** = multiplicador (quantos zeros adicionar)
- **4ª faixa** (dourado/prata) = tolerância — ignore para o valor

As duas primeiras faixas **formam um número juntas** (não são multiplicadas
entre si). O erro comum é achar que marrom-preto dá "1"; na verdade dá **10**.

### Por que ×1000 vira 10 kΩ (não 1 kΩ)

Em **marrom-preto-laranja**:

```
marrom = 1  ─┐
preto  = 0  ─┴──► os dígitos juntos formam "10"
laranja = ×1000 ──► multiplica (= 3 zeros)

10 × 1000 = 10 000 Ω = 10 kΩ
```

O preto NÃO é o multiplicador aqui — ele é o segundo dígito. Outra forma de ver:
os dois dígitos você escreve ("10"), e o multiplicador conta como "quantos zeros
colocar depois" (laranja = 3 zeros → 10 + 000 = 10000).

### Tabela de cores

| Cor | Dígito | Multiplicador |
|---|---|---|
| Preto | 0 | ×1 |
| Marrom | 1 | ×10 |
| Vermelho | 2 | ×100 |
| Laranja | 3 | **×1000** |
| Amarelo | 4 | ×10 000 |
| Verde | 5 | — |
| Azul | 6 | — |
| Violeta | 7 | — |
| Cinza | 8 | — |
| Branco | 9 | — |
| Dourado | — | tolerância ±5% |
| Prata | — | tolerância ±10% |

### Valores parecidos (cuidado para não confundir)

Os três têm os mesmos dígitos (marrom-preto = "10"); só a 3ª faixa muda:

| Cores | Dígitos | Multiplicador | Valor |
|---|---|---|---|
| marrom-preto-**vermelho** | 10 | ×100 | 1 000 Ω = **1 kΩ** |
| marrom-preto-**laranja** | 10 | ×1000 | 10 000 Ω = **10 kΩ** |
| marrom-preto-**amarelo** | 10 | ×10000 | 100 000 Ω = **100 kΩ** |

**Atalho:** o 10 kΩ de 4 faixas é sempre **marrom-preto-laranja**. O "laranja"
no multiplicador é a assinatura do 10k.

Resistor de 5 faixas (precisão): 10 kΩ = **marrom-preto-preto-vermelho**
(1-0-0 = "100", ×100 → 10 000 Ω).

### Jeito mais confiável: multímetro

Ponha em modo resistência (Ω), encoste uma ponta em cada perna (resistor não tem
polaridade) e leia. 10 kΩ aparece como `10.0 kΩ`, ou `10` na escala de kΩ, ou
`0.010` na escala de MΩ. Em multímetro manual, use a escala de **20k**.

### Para pull-up do DHT22, o valor não é crítico

Qualquer coisa entre **~4,7 kΩ e ~10 kΩ** funciona:

- **4,7 kΩ** = amarelo-violeta-vermelho ✅
- **5,6 kΩ** = verde-azul-vermelho ✅
- **10 kΩ** = marrom-preto-laranja ✅ (ideal)

Sem resistor à mão: o pull-up interno do ESP32 (~45 kΩ, já ativado no código do
`temp_humid_sensor`) pode funcionar com fio curto, mas é menos confiável — se der
`no response`, use o resistor externo.
