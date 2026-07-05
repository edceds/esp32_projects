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
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_timer.h"
#include "driver/gpio.h"
}

static const char* TAG = "CPP_HTTP";

#define DEV_MODE 0  // 1 = local PC, 0 = production (jadefi)

constexpr const char* WIFI_SSID = "HUAWEI-2.4G-Fd8C";
constexpr const char* WIFI_PASS = "a3Ptrp8s";

#if DEV_MODE
constexpr const char* POST_URL = "http://192.168.18.222:3100/";
#else
constexpr const char* POST_URL = "http://164.68.102.43:3100/";
#endif

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

    ESP_LOGI(TAG, "WiFi ready");

    /* ================= GPIO 33 STATUS ================= */

    gpio_set_direction(GPIO_NUM_33, GPIO_MODE_INPUT);

    int prev_level = gpio_get_level(GPIO_NUM_33);

    // Get chip info once at boot
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    while (true) {
        int level = gpio_get_level(GPIO_NUM_33);

        if (level != prev_level) {
            const char* state = level ? "high" : "low";
            ESP_LOGI(TAG, "GPIO 33 changed to %s — sending POST", state);

            // Gather dynamic info
            uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000);
            uint32_t free_heap = esp_get_free_heap_size();

            wifi_ap_record_t ap_info;
            int8_t rssi = 0;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                rssi = ap_info.rssi;
            }

            char post_data[256];
            snprintf(post_data, sizeof(post_data),
                     "{"
                     "\"gpio33\":\"%s\","
                     "\"uptime_s\":%lu,"
                     "\"free_heap\":%lu,"
                     "\"wifi_rssi\":%d,"
                     "\"chip_cores\":%d,"
                     "\"chip_revision\":%d"
                     "}",
                     state,
                     (unsigned long)uptime_s,
                     (unsigned long)free_heap,
                     (int)rssi,
                     (int)chip_info.cores,
                     (int)chip_info.revision);

            esp_http_client_config_t post_config{};
            post_config.url = POST_URL;
            post_config.event_handler = http_event_handler;
            post_config.timeout_ms = 10000;

            auto post_client = esp_http_client_init(&post_config);
            esp_http_client_set_method(post_client, HTTP_METHOD_POST);
            esp_http_client_set_header(post_client, "Content-Type", "application/json");
            esp_http_client_set_post_field(post_client, post_data, strlen(post_data));

            auto post_err = esp_http_client_perform(post_client);

            if (post_err == ESP_OK) {
                ESP_LOGI(TAG, "POST Status = %d",
                         esp_http_client_get_status_code(post_client));
            } else {
                ESP_LOGE(TAG, "POST failed: %s", esp_err_to_name(post_err));
            }

            esp_http_client_cleanup(post_client);
        }

        prev_level = level;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}