// Read temperature + humidity from a DHT22 (AM2302) sensor.
//
// Wiring:
//   DHT22 pin 1 (VCC)  -> 3V3
//   DHT22 pin 2 (DATA) -> DHT_GPIO (with a 10k pull-up to 3V3)
//   DHT22 pin 3 (NC)   -> not connected
//   DHT22 pin 4 (GND)  -> GND
//
// The DHT22 uses a single-wire, timing-based protocol. There is no ESP-IDF
// driver for it, so we bit-bang GPIO directly through a typed pin wrapper
// (see gpio_pin.hpp). Each conversion returns 40 bits: 16 humidity, 16
// temperature, 8 checksum.

#include <cstdio>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h" // esp_rom_delay_us
#include "esp_log.h"
}

#include "gpio_pin.hpp"

// The data pin. GPIO32 is on the left side of the DevKit and is output-capable.
// Because DhtPin below is an OpenDrainPin, choosing an input-only GPIO (34-39)
// is a COMPILE error, not a runtime one — try GPIO_NUM_36 to see.
static constexpr gpio_num_t DHT_GPIO = GPIO_NUM_32;
static constexpr int DHT_TIMEOUT_US = 1000; // give up waiting for an edge

static const char *TAG = "dht22";

using DhtPin = pin::OpenDrainPin<DHT_GPIO>;

// Busy-wait until `line` reaches `level`, up to DHT_TIMEOUT_US.
// Returns microseconds waited, or -1 on timeout.
static int wait_for_level(const DhtPin &line, int level)
{
  int us = 0;
  while (line.get() != level)
  {
    if (us++ > DHT_TIMEOUT_US)
      return -1;
    esp_rom_delay_us(1);
  }
  return us;
}

// Perform one read. On success writes Celsius + %RH and returns ESP_OK.
static esp_err_t dht22_read(const DhtPin &line, float *temperature, float *humidity)
{
  uint8_t data[5] = {0};

  // --- Start signal: pull the line low for >1ms, then release. ---
  line.as_output();
  line.low();
  esp_rom_delay_us(1200);
  line.high();
  esp_rom_delay_us(30);

  // Hand the line back to the sensor (external pull-up holds it high).
  line.as_input();

  // --- Sensor response: ~80us low, then ~80us high. ---
  if (wait_for_level(line, 0) < 0)
  {
    ESP_LOGD(TAG, "no response (low)");
    return ESP_ERR_TIMEOUT;
  }
  if (wait_for_level(line, 1) < 0)
  {
    ESP_LOGD(TAG, "no response (high)");
    return ESP_ERR_TIMEOUT;
  }
  if (wait_for_level(line, 0) < 0)
  {
    ESP_LOGD(TAG, "no data start");
    return ESP_ERR_TIMEOUT;
  }

  // --- Read 40 bits. Each bit: ~50us low, then a high pulse whose length
  //     encodes the value (~26us = 0, ~70us = 1). ---
  for (int i = 0; i < 40; i++)
  {
    if (wait_for_level(line, 1) < 0)
    {
      ESP_LOGD(TAG, "bit %d low timeout", i);
      return ESP_ERR_TIMEOUT;
    }
    int high_us = wait_for_level(line, 0);
    if (high_us < 0)
    {
      ESP_LOGD(TAG, "bit %d high timeout", i);
      return ESP_ERR_TIMEOUT;
    }

    data[i / 8] <<= 1;
    if (high_us > 45)
      data[i / 8] |= 1; // long high pulse -> bit is 1
  }

  // --- Verify checksum (low 8 bits of the sum of the first 4 bytes). ---
  uint8_t sum = data[0] + data[1] + data[2] + data[3];
  if (sum != data[4])
  {
    // Rebaixado para debug: com retry no app_main, uma falha isolada de
    // checksum é esperada e recuperada. Só o erro final aparece no log.
    ESP_LOGD(TAG, "checksum mismatch: got 0x%02x want 0x%02x", data[4], sum);
    return ESP_ERR_INVALID_CRC;
  }

  *humidity = ((data[0] << 8) | data[1]) * 0.1f;

  // Temperature: top bit of data[2] is the sign.
  int16_t raw_t = ((data[2] & 0x7F) << 8) | data[3];
  *temperature = raw_t * 0.1f;
  if (data[2] & 0x80)
    *temperature = -*temperature;

  return ESP_OK;
}

extern "C" void app_main(void)
{
  // The data line idles high; enable the internal pull-up as a backup to the
  // external 10k resistor. Construction sets the pull mode.
  DhtPin line;

  // DHT22 needs ~1s after power-up before the first stable reading.
  vTaskDelay(pdMS_TO_TICKS(2000));

  while (true)
  {
    float temperature, humidity;

    // Sem resistor de pull-up externo, o pull-up interno (fraco) faz leituras
    // falharem de vez em quando com checksum ruim. Tenta algumas vezes: quase
    // sempre uma das tentativas passa, então o log fica limpo sem hardware extra.
    esp_err_t err = ESP_FAIL;
    for (int attempt = 0; attempt < 3; attempt++)
    {
      err = dht22_read(line, &temperature, &humidity);
      if (err == ESP_OK)
        break;
      vTaskDelay(pdMS_TO_TICKS(50)); // deixa a linha assentar antes de re-ler
    }

    if (err == ESP_OK)
    {
      ESP_LOGI(TAG, "Temperature: %.1f C   Humidity: %.1f %%",
               temperature, humidity);
    }
    else
    {
      ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(err));
    }

    // DHT22 sampling rate is max once every 2 seconds.
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
