#include <freertos/FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <freertos/projdefs.h>
#include <freertos/task.h>

#include "oledhandler.h"
#include "systemeventhandler.h"
#include "timetracker.h"

#include <portmacro.h>
#include <stdio.h>
#include <time.h>
#include <driver/gpio.h>

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
    snprintf(buffer, sizeof(buffer), "clock: %02d:%02d", time_info.tm_hour, time_info.tm_min);
    send_text(buffer);
}

void updateDisplayWithTime(uint8_t *lastMinute) {
    const struct tm time_info = get_current_time();

    if (*lastMinute != time_info.tm_min) {
        *lastMinute = time_info.tm_min;

        print_time(time_info);
    }
}

// Build in LED
#define GPIO_LED GPIO_NUM_2

void timetracker_task(void *args) {
    wait_for_state(EVENT_BIT_WIFI_HANDLER_DONE);

    clear_display();
    send_text("-time synched");
    send_text("-main program rdy");
    vTaskDelay(pdMS_TO_TICKS(1000));
    uint8_t lastMinute = 0;
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        updateDisplayWithTime(&lastMinute);
        if (event_bit_is_set(EVENT_BIT_BUTTON_1_PRESSED)) {
            gpio_set_level(GPIO_LED, 1);
        }
        if (event_bit_is_set(EVENT_BIT_BUTTON_2_PRESSED)) {
            gpio_set_level(GPIO_LED, 0);
        }
    }
}

void init_timetracker_handler(const uint8_t priority) {
    xTaskCreate(timetracker_task, "timetracker_task", 4096, NULL, priority, NULL);
}
