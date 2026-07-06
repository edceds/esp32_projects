// Driver para LCD 16x2 (HD44780) via módulo I2C PCF8574 (a plaquinha preta
// soldada atrás do display). Usa a API i2c_master nova do ESP-IDF v5.5.
//
// O PCF8574 é um expansor de 8 bits. A fiação padrão desses módulos mapeia:
//   P0=RS  P1=RW  P2=EN  P3=backlight  P4-P7 = D4-D7 (barramento em 4 bits)
//
// Alimentação: o LCD quer 5V (VCC no VIN/5V do devkit, não no 3V3), senão o
// display fica apagado/fraco. SDA/SCL podem ir direto no ESP32 (3,3V) porque o
// PCF8574 é open-dreno.

#pragma once

extern "C"
{
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h" // esp_rom_delay_us
}

#include <cstdint>
#include <cstring>

class Lcd1602
{
  // Bits de controle no PCF8574 (fiação padrão dos módulos).
  static constexpr uint8_t RS = 0x01; // 0 = comando, 1 = dado
  static constexpr uint8_t EN = 0x04; // pulso de "enable"
  static constexpr uint8_t BL = 0x08; // backlight ligado

  i2c_master_dev_handle_t dev_;
  uint8_t backlight_ = BL;

  // Escreve 1 byte cru no PCF8574 (que vira os níveis dos pinos do LCD).
  void write_pcf(uint8_t v) const
  {
    i2c_master_transmit(dev_, &v, 1, 100 /*ms timeout*/);
  }

  // Manda um nibble (4 bits altos) com o pulso de enable exigido pelo HD44780.
  void write_nibble(uint8_t nibble, uint8_t mode) const
  {
    uint8_t d = (nibble & 0xF0) | mode | backlight_;
    write_pcf(d | EN); // enable alto
    esp_rom_delay_us(1);
    write_pcf(d & ~EN); // enable baixo -> LCD captura o nibble
    esp_rom_delay_us(50);
  }

  // Um byte = dois nibbles (alto depois baixo). mode=0 comando, mode=RS dado.
  void write_byte(uint8_t b, uint8_t mode) const
  {
    write_nibble(b & 0xF0, mode);
    write_nibble((b << 4) & 0xF0, mode);
  }

  void command(uint8_t c) const { write_byte(c, 0); }

public:
  // Anexa o LCD a um barramento I2C já criado. addr costuma ser 0x27 ou 0x3F.
  esp_err_t begin(i2c_master_bus_handle_t bus, uint8_t addr = 0x27)
  {
    i2c_device_config_t cfg = {};
    cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    cfg.device_address = addr;
    cfg.scl_speed_hz = 100000; // 100 kHz é de sobra pro HD44780

    esp_err_t err = i2c_master_bus_add_device(bus, &cfg, &dev_);
    if (err != ESP_OK)
      return err;

    // Sequência de inicialização do HD44780 em modo 4 bits (do datasheet).
    vTaskDelay(pdMS_TO_TICKS(50)); // espera o LCD ligar
    write_nibble(0x30, 0);
    esp_rom_delay_us(4500);
    write_nibble(0x30, 0);
    esp_rom_delay_us(4500);
    write_nibble(0x30, 0);
    esp_rom_delay_us(150);
    write_nibble(0x20, 0); // entra no modo 4 bits

    command(0x28); // function set: 4 bits, 2 linhas, fonte 5x8
    command(0x0C); // display ligado, cursor off, blink off
    command(0x01); // limpa o display
    vTaskDelay(pdMS_TO_TICKS(2));
    command(0x06); // modo de entrada: incrementa, sem shift
    return ESP_OK;
  }

  void clear() const
  {
    command(0x01);
    vTaskDelay(pdMS_TO_TICKS(2)); // clear é lento (~1.5ms)
  }

  // Posiciona o cursor. col 0-15, row 0-1.
  void set_cursor(uint8_t col, uint8_t row) const
  {
    static const uint8_t row_offset[2] = {0x00, 0x40};
    command(0x80 | (col + row_offset[row & 1]));
  }

  void print(const char *s) const
  {
    for (; *s; ++s)
      write_byte((uint8_t)*s, RS);
  }

  // Escreve uma linha inteira, preenchendo com espaços até 16 chars pra apagar
  // o que sobrou de um texto anterior mais longo.
  void print_line(uint8_t row, const char *s) const
  {
    char buf[17];
    int n = 0;
    for (; n < 16 && s[n]; ++n)
      buf[n] = s[n];
    for (; n < 16; ++n)
      buf[n] = ' ';
    buf[16] = '\0';
    set_cursor(0, row);
    print(buf);
  }

  void backlight(bool on)
  {
    backlight_ = on ? BL : 0;
    write_pcf(backlight_);
  }
};
