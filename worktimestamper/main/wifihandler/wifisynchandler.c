#include "systemeventhandler.h"
#include "wifisynchandler.h"
#include "credentials.h"
#include "oledhandler.h"

#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_wifi_default.h>
#include <nvs_flash.h>
#include <portmacro.h>
#include <stdint.h>
#include <driver/gpio.h>
#include <lwip/apps/sntp.h>
#include <rom/ets_sys.h>

#define WIFI_CONNECTED_BIT BIT0

static bool is_connected;
static bool should_reconnect = true;

void wifi_log_status(const char *prefix, const bool connected, const int retry_count) {
    char status_msg[32];
    snprintf(status_msg, sizeof(status_msg), "%s %s %d",
             prefix, connected ? "connected" : "disconnected", retry_count);
    send_text(status_msg);
}

static void wifi_event_handler(
    void *arg,
    const esp_event_base_t event_base,
    const int32_t event_id,
    void *event_data
) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW("WIFI", "Disconnected, should_reconnect=%d", should_reconnect);
                if (should_reconnect) {
                    esp_wifi_connect();
                }
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI("WIFI", "Got IP");
        set_event_bit(EVENT_BIT_WIFI_CONNECTED);
    }
}

bool setup_wifi(int *retry_out) {
    esp_log_level_set("wifi", ESP_LOG_INFO);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // register the event handler for listening to IP_EVENT_STA_GOT_IP event from WI-FI stack
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    int retry = 0;
    while (retry < 5) {
        if (wait_for_state(EVENT_BIT_WIFI_CONNECTED)) {
            is_connected = true;
            break;
        }

        retry++;
    }

    if (retry_out) *retry_out = retry;
    return is_connected;
}

void disconnect_wifi() {
    should_reconnect = false;

    esp_wifi_disconnect();
    esp_wifi_stop();

    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler);
}

static void initTimeSync() {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1); // Central Europe
}

static void waitForTime() {
    time_t now = 0;
    struct tm timeInfo = {0};

    while (timeInfo.tm_year < 2020 - 1900) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeInfo);
    }

    char buffer[50];
    snprintf(buffer, sizeof(buffer), " clock: %02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
    send_text(buffer);
}

void wifi_sync_task(void *args) {
    send_text(" start WIFI task");
    if (!is_connected) {
        int retries = 0;
        const bool success = setup_wifi(&retries);
        wifi_log_status(" WIFI", success, retries);
    }

    send_text(" init time sync");
    initTimeSync();
    waitForTime();
    disconnect_wifi();
    send_text(" WIFI disconnected");
    send_text(" Controller ready");
    set_event_bit(EVENT_BIT_WIFI_HANDLER_DONE);
    vTaskDelete(NULL);
}

void init_wifi_sync_handler(const int priority) {
    xTaskCreate(wifi_sync_task, "wifi_sync_task", 8192, NULL, priority, NULL);
}
