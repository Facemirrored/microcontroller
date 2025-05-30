#include <esp_log.h>

#include "buttonisrhandler.h"
#include "oledhandler.c"
#include "wifisynchandler.h"
#include "systemeventhandler.h"
#include "timetracker.h"

#include <string.h>
#include <lwip/apps/sntp.h>
#include "rom/ets_sys.h"

// EPS32-API: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/index.html
// ESP32-Datasheet: https://m.media-amazon.com/images/I/A1oyy-n8xfL.pdf

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

    init_system_event_group();

    oled_init();

    send_text("-Timestamper startup-");

    init_button_isr_handler(3);
    init_wifi_sync_handler(2);
    init_timetracker_handler(1);
}
