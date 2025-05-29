#include <esp_log.h>

#include "buttonisrhandler/buttonisrhandler.h"
#include "oledhandler/oledhandler.c"
#include "wifihandler/wifisynchandler.h"
#include <nvs_flash.h>
#include <string.h>

#include <driver/gpio.h>
#include <lwip/apps/sntp.h>

#include "rom/ets_sys.h"
#include "esp_event.h"

// EPS32-API: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/index.html
// ESP32-Datasheet: https://m.media-amazon.com/images/I/A1oyy-n8xfL.pdf

// Build int LED
#define GPIO_LED GPIO_NUM_2

struct tm get_current_time() {
    time_t now;
    struct tm time_info;

    time(&now);
    localtime_r(&now, &time_info);

    return time_info;
}

void print_time(const struct tm time_info) {
    clear_display();

    char buffer[50];
    snprintf(buffer, sizeof(buffer), "Clock: %02d:%02d", time_info.tm_hour, time_info.tm_min);
    send_text(buffer, 0, 0);
}

void updateDisplayWithTime(int *lastMinute) {
    const struct tm time_info = get_current_time();

    if (*lastMinute != time_info.tm_min) {
        *lastMinute = time_info.tm_min;

        print_time(time_info);
    }
}

void on_button_1_pressed() {
    gpio_set_level(GPIO_LED, 1);
}

void on_button_2_pressed() {
    gpio_set_level(GPIO_LED, 0);
}

const char *reset_reason_str(const esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON: return "Power-On";
        case ESP_RST_SW: return "Software Reset";
        case ESP_RST_PANIC: return "Panic";
        case ESP_RST_INT_WDT: return "Interrupt Watchdog";
        case ESP_RST_TASK_WDT: return "Task Watchdog";
        case ESP_RST_DEEPSLEEP: return "Deep Sleep Wake";
        default: return "Other";
    }
}

void app_main(void) {
    const esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI("BOOT", "Reset Reason: %s", reset_reason_str(reason));

    oled_init();
    send_text("Timestamper startup...", 0, 0);

    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

    create_button_isr_handler(on_button_1_pressed, on_button_2_pressed);
    init_button_isr_handler(2);
    init_wifi_sync_handler(1);
    vTaskDelay(pdTICKS_TO_MS(2000));

    print_time(get_current_time());


    int lastMinute = -1;

    // ReSharper disable once CppDFAEndlessLoop
    while (true) {
        updateDisplayWithTime(&lastMinute);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
