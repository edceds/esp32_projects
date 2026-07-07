// Conexão WiFi em modo estação (STA). Adaptado do esp_http_test.
// Conecta na rede e reconecta sozinho se cair. wifi_got_ip() diz quando o IP
// já chegou; wifi_ip_str() devolve o IP como texto (pra logar/mostrar).

#pragma once

extern "C"
{
#include <cstring>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
}

namespace wifi
{

constexpr const char *SSID = "HUAWEI-2.4G-Fd8C";
constexpr const char *PASS = "a3Ptrp8s";

inline bool s_got_ip = false;
inline char s_ip[16] = "0.0.0.0";

inline void event_handler(void *, esp_event_base_t base, int32_t id, void *data)
{
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START)
  {
    esp_wifi_connect();
  }
  else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
  {
    ESP_LOGW("wifi", "desconectado — reconectando...");
    s_got_ip = false;
    esp_wifi_connect();
  }
  else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
  {
    auto *ev = (ip_event_got_ip_t *)data;
    esp_ip4addr_ntoa(&ev->ip_info.ip, s_ip, sizeof(s_ip));
    ESP_LOGI("wifi", "conectado — IP: %s", s_ip);
    s_got_ip = true;
  }
}

inline void connect()
{
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &event_handler, nullptr, nullptr);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                      &event_handler, nullptr, nullptr);

  wifi_config_t wc = {};
  std::strncpy((char *)wc.sta.ssid, SSID, sizeof(wc.sta.ssid));
  std::strncpy((char *)wc.sta.password, PASS, sizeof(wc.sta.password));

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &wc);
  esp_wifi_start();
}

inline bool got_ip() { return s_got_ip; }
inline const char *ip_str() { return s_ip; }

} // namespace wifi
