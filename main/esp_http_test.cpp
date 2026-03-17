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
#include "esp_tls.h"
}

static const char* TAG = "CPP_HTTP";

constexpr const char* WIFI_SSID = "HUAWEI-2.4G-Fd8C";
constexpr const char* WIFI_PASS = "a3Ptrp8s";

static bool wifi_ready = false;

/* ================= WIFI EVENT HANDLER ================= */

static void wifi_event_handler(void* arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void* event_data)
{
    if (event_base == IP_EVENT &&
        event_id == IP_EVENT_STA_GOT_IP) {

        ESP_LOGI(TAG, "GOT IP — NETWORK READY");
        wifi_ready = true;
    }

    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_DISCONNECTED) {

        ESP_LOGI(TAG, "WiFi disconnected — retrying...");
        esp_wifi_connect();
        wifi_ready = false;
    }
}

/* ================= WIFI INIT ================= */

void wifi_connect()
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        nullptr,
        nullptr
    );

    esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        nullptr,
        nullptr
    );

    wifi_config_t wifi_config{};
    std::strncpy((char*)wifi_config.sta.ssid,
                 WIFI_SSID,
                 sizeof(wifi_config.sta.ssid));

    std::strncpy((char*)wifi_config.sta.password,
                 WIFI_PASS,
                 sizeof(wifi_config.sta.password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

/* ================= HTTP EVENT HANDLER ================= */

static esp_err_t http_event_handler(esp_http_client_event_t* evt)
{
    switch(evt->event_id) {

        case HTTP_EVENT_ON_DATA:
        {
            std::string data((char*)evt->data, evt->data_len);
            std::cout << data;
            break;
        }

        default:
            break;
    }
    return ESP_OK;
}

/* ================= MAIN ================= */

extern "C" void app_main()
{
    nvs_flash_init();

    ESP_LOGI(TAG, "Connecting WiFi...");
    wifi_connect();

    while (!wifi_ready) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "WiFi ready — sending HTTP GET");

    esp_http_client_config_t config{};
    config.url = "http://server.jadefi.io:4000/";
    config.event_handler = http_event_handler;
    config.timeout_ms = 10000;

    auto client = esp_http_client_init(&config);

    auto err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP Status = %d",
                 esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s",
                 esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}