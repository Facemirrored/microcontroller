#include <oledhandler.h>
#include "systemeventhandler.h"
#include "timetracker_logic.h"
#include "timetracker_display.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>

static void clock_task(void *arg);

static void button1_task(void *arg);

static void button2_task(void *arg);

void timetracker_start(const uint8_t base_priority) {
    static TimeTrackerState tracker_state = {0};
    init_timetracker_state(&tracker_state);

    // Wait for the system to be ready (e.g., Wi-Fi sync complete)
    wait_for_state(EVENT_BIT_WIFI_HANDLER_DONE);
    vTaskDelay(pdMS_TO_TICKS(1000));

    clear_display();
    vTaskDelay(pdMS_TO_TICKS(100));

    display_tutorial();

    // Start core tasks
    xTaskCreate(clock_task, "clock_task", 4096, &tracker_state, base_priority, NULL);
    xTaskCreate(button1_task, "button1_task", 4096, &tracker_state, base_priority + 1, NULL);
    xTaskCreate(button2_task, "button2_task", 4096, &tracker_state, base_priority + 2, NULL);
}

static void clock_task(void *arg) {
    const TimeTrackerState *state = arg;
    int last_second = -1;

    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        if (event_bit_is_set(EVENT_BIT_TUTORIAL_ACTIVE)) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        time_t now;
        struct tm time_info;
        time(&now);
        localtime_r(&now, &time_info);

        if (time_info.tm_sec != last_second) {
            last_second = time_info.tm_sec;
            display_clock(&time_info);

            if (!state->is_summary_mode) {
                display_working(state);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void button1_task(void *arg) {
    TimeTrackerState *state = arg;

    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        wait_for_state(EVENT_BIT_BUTTON_1_PRESSED);

        if (!state->is_summary_mode && handle_stamp(state)) {
            display_working(state);
        }
    }
}

static void button2_task(void *arg) {
    TimeTrackerState *state = arg;

    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        wait_for_state(EVENT_BIT_BUTTON_2_PRESSED);

        if (event_bit_is_set(EVENT_BIT_TUTORIAL_ACTIVE)) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        state->is_summary_mode = !state->is_summary_mode;

        if (state->is_summary_mode) {
            display_summary(state);
        } else {
            display_working(state);
        }
    }
}
