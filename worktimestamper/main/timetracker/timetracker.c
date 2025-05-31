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

#define MAX_SESSIONS 6

enum {
    HEADER_TIME_MODE = 0,
} TIMETRACKER_OLED_OUTPUT;

struct WorkTimeSession {
    time_t start_time;
    time_t end_time;
};

int last_minute = 0;
int last_second = 0;

bool is_summary_mode = false;
bool is_working = false;
struct WorkTimeSession current_sessions[MAX_SESSIONS];
uint8_t current_session_index = 0;

struct tm get_current_time() {
    time_t now;
    struct tm time_info;

    time(&now);
    localtime_r(&now, &time_info);

    return time_info;
}

void tutorial() {
    set_event_bit(EVENT_BIT_TUTORIAL_ACTIVE);

    const char *tutorial_page[] = {
        "----time synched----",
        "--main program rdy--",
        "                    ",
        " left btn: stamp    ",
        " right btn: switch  ",
        "                    ",
        "===> press left <===",
        "===>  to start  <===",
    };
    send_page_20x8(tutorial_page);

    wait_for_state(EVENT_BIT_BUTTON_1_PRESSED);

    clear_display();
    clear_event_bit(EVENT_BIT_TUTORIAL_ACTIVE);
    vTaskDelay(pdMS_TO_TICKS(500));
}

void display_working() {
    // TODO ausgabe working

    // nur als show case
    const struct tm time_info = get_current_time();
    char buffer[21];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
             time_info.tm_hour,
             time_info.tm_min,
             time_info.tm_sec);

    send_text_at_row("                    ", 6);
    send_text_at_row(buffer, 5);
}

void display_summary() {
    // TODO: display summary

    // nur als show case
    const struct tm time_info = get_current_time();
    char buffer[21];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
             time_info.tm_hour,
             time_info.tm_min,
             time_info.tm_sec);

    send_text_at_row("                    ", 5);
    send_text_at_row(buffer, 6);
}

void track_display_switch_mode_task() {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        // we don't want to react to btn 2 if the tutorial is printed
        if (event_bit_is_set(EVENT_BIT_TUTORIAL_ACTIVE)) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        wait_for_state(EVENT_BIT_BUTTON_2_PRESSED);

        // TODO: wir sollten hier direkt ausgeben
        is_summary_mode = !is_summary_mode;

        if (is_summary_mode) {
            display_summary();
        } else {
            display_working();
        }
    }
}

void track_stamp_task() {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        wait_for_state(EVENT_BIT_BUTTON_1_PRESSED);

        // button 1 has no function in summary mode
        if (is_summary_mode) {
            continue;
        }

        if (is_working) {
            time(&current_sessions[current_session_index].end_time);
            current_session_index++;
            is_working = false;
        } else {
            time(&current_sessions[current_session_index].start_time);
            is_working = true;
        }

        display_working();
    }
}

void timetracker_task() {
    // TODO: react to
    // - show_stamps -> print summary display (dynamic calculation of in and out tamps (work times diff)
    // - !show_stamps -> print working display (dynamic show last pause, net work, next must have pause
    vTaskDelay(pdMS_TO_TICKS(1000));
    vTaskDelete(NULL);
}

void clock_task() {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        // we don't update display if tutorial is printed
        if (event_bit_is_set(EVENT_BIT_TUTORIAL_ACTIVE)) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        const struct tm time_info = get_current_time();

        if (last_second != time_info.tm_sec) {
            last_second = time_info.tm_sec;

            // TODO: wir sollten nur zeit ausgeben (nicht den status rechts überschreiben)
            const char *mode = is_summary_mode ? "summary" : "working";

            char buffer[21];
            snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d%*s",
                     time_info.tm_hour,
                     time_info.tm_min,
                     time_info.tm_sec,
                     20 - 8, // 20 chars total - 8 chars clock
                     mode); // right padding

            send_text_at_row(buffer, HEADER_TIME_MODE);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void init_timetracker_handler(const uint8_t priority) {
    wait_for_state(EVENT_BIT_WIFI_HANDLER_DONE);
    vTaskDelay(pdMS_TO_TICKS(1000));
    clear_display();
    vTaskDelay(pdMS_TO_TICKS(100));
    tutorial();

    // clock print task
    xTaskCreate(clock_task, "clock_task", 4096, NULL, priority, NULL);
    // track button 1 press for stamping
    xTaskCreate(track_stamp_task, "track_stamp_task", 4096, NULL, priority + 2, NULL);
    // track button 2 press for switching display
    xTaskCreate(track_display_switch_mode_task, "track_display_switch_mode_task", 4096, NULL, priority + 1, NULL);
    // control task for calculating and reacting to states
    xTaskCreate(timetracker_task, "timetracker_task", 4096, (void *) (uintptr_t) priority + 1, priority + 1, NULL);
}
