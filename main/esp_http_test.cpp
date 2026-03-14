#include <cstring>
#include <string>
#include <iostream>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_http_client.h"
}

static const char* TAG = "CPP_HTTP";

constexpr const char* WIFI_SSID = "YOUR_WIFI";
constexpr const char* WIFI_PASS = "YOUR_PASS";

void wifi_connect()
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config{};
    std::strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    std::strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

esp_err_t http_event_handler(esp_http_client_event_t* evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        std::string data((char*)evt->data, evt->data_len);
        std::cout << data;
    }
    return ESP_OK;
}

extern "C" void app_main()
{
    nvs_flash_init();

    ESP_LOGI(TAG, "Connecting WiFi...");
    wifi_connect();

    vTaskDelay(pdMS_TO_TICKS(5000));

    esp_http_client_config_t config{};
    config.url = "http://server.jadefi.io:4000/";
    config.event_handler = http_event_handler;

    auto client = esp_http_client_init(&config);

    ESP_LOGI(TAG, "Sending HTTP GET...");
    auto err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Status = %d",
                 esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "HTTP request failed");
    }

    esp_http_client_cleanup(client);
}