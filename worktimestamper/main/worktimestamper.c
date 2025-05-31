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

#define COLUMNS 21
#define ROWS 8

typedef struct {
    uint8_t column;
    uint8_t row;
} Position;

Position positions[COLUMNS * ROWS];

// Fisher-Yates Shuffle (zum Mischen des Arrays)
void shuffle_positions(Position *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Position temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

void random_display_fill_once() {
    // fill all positions
    int idx = 0;
    for (uint8_t r = 0; r < ROWS; r++) {
        for (uint8_t c = 0; c < COLUMNS; c++) {
            positions[idx].column = c;
            positions[idx].row = r;
            idx++;
        }
    }

    // shuffle positions
    shuffle_positions(positions, COLUMNS * ROWS);

    // print
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    const size_t charset_len = sizeof(charset) - 1;

    char buf[2] = {0};

    int speed = 50;
    const int steps = COLUMNS * ROWS;

    for (int i = 0; i < steps; i++) {
        buf[0] = charset[rand() % charset_len];
        send_text_at(buf, positions[i].column, positions[i].row);

        vTaskDelay(pdMS_TO_TICKS(speed));

        speed *= 0.9;
    }

    for (int i = 0; i < steps; i++) {
        send_text_at(" ", positions[i].column, positions[i].row);

        vTaskDelay(pdMS_TO_TICKS(speed));

        speed *= 0.9;
        if (speed < 5) {
            speed = 5;
        }
    }

    clear_display();
    vTaskDelay(pdMS_TO_TICKS(50));
    send_text_at("   START CONTROLLER ", 0, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void app_main(void) {
    const esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI("BOOT", "Reset Reason: %s", reset_reason_str(reason));

    init_system_event_group();
    init_oled();

    random_display_fill_once();

    init_button_isr_handler(3);
    init_wifi_sync_handler(2);
    init_timetracker_handler(1);
}
