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

struct WorkTimeSession {
    time_t start_time;
    time_t end_time;
};

int last_minute = 0;
int last_second = 0;

bool show_stamps = false;
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

void print_time(const struct tm time_info, const bool with_seconds, const uint8_t column, const uint8_t page) {
    char buffer[50];
    if (with_seconds) {
        snprintf(buffer, sizeof(buffer),
                 "time: %02d:%02d:%02d",
                 time_info.tm_hour,
                 time_info.tm_min,
                 time_info.tm_sec);
    } else {
        snprintf(buffer, sizeof(buffer), "time: %02d:%02d", time_info.tm_hour, time_info.tm_min);
    }
    send_text_at(buffer, column, page);
}

void update_timers() {
    const struct tm time_info = get_current_time();

    // update header state
    if (last_minute != time_info.tm_min) {
        last_minute = time_info.tm_min;

        char buffer[21];
        snprintf(buffer, sizeof(buffer), "%02d:%02d        %s",
                 time_info.tm_hour,
                 time_info.tm_min,
                 show_stamps ? "summary" : "working");

        send_text_at(buffer, 0, 0);
    }

    // TODO: nur ein test auf aktuell uhrzeit, noch anpassen auf die aktuelle net time
    // update working time
    if (is_working && last_second != time_info.tm_sec) {
        last_second = time_info.tm_sec;

        char buffer[21];
        snprintf(buffer, sizeof(buffer),
                 "net time:   %02d:%02d:%02d",
                 time_info.tm_hour,
                 time_info.tm_min,
                 time_info.tm_sec);

        send_text_at(buffer, 0, 7);
    }
}

void tutorial() {
    clear_display_and_queue();

    const char *tutorial_page[] = {
        "----time synched----",
        "--main program rdy--",
        " use left button to ",
        "  stamp and right   ",
        "button for switching",
        "       display      ",
        "===> press left <===",
        "===>  to start  <===",
    };
    //send_page_20x8(tutorial_page);

    wait_for_state(EVENT_BIT_BUTTON_1_PRESSED);
    clear_display_and_queue();
    get_current_time();
    vTaskDelay(pdMS_TO_TICKS(500));
}

// TODO: besseres output: 23:32      Summary bzw. 32:32     Working
// TODO: update_display muss wohl wirklich jede 100ms oder so passieren, weil wir die sekunden aktuell anzeigen wollen für netto i guess
void update_display() {
    const struct tm time_info = get_current_time();

    char buffer[21];
    snprintf(buffer, sizeof(buffer), "%02d:%02d        %s",
             time_info.tm_hour,
             time_info.tm_min,
             show_stamps ? "summary" : "working");

    if (show_stamps) {
        const char *show_stamps_page[] = {
            buffer,
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
        };
        //send_page_20x8_no_clear_with_delay(show_stamps_page, 0);
    } else {
        char session[20];
        snprintf(session, sizeof(session), "sessions: %02d      ", current_session_index);
        const char *show_work_page[] = {
            buffer,
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
        };
        //send_page_20x8_no_clear_with_delay(show_work_page, 0);
    }
}

void display_mode_task() {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        wait_for_state(EVENT_BIT_BUTTON_2_PRESSED);
        show_stamps = !show_stamps;
        set_event_bit(EVENT_BIT_UPDATE_DISPLAY);
    }
}

void stamp_task() {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        wait_for_state(EVENT_BIT_BUTTON_1_PRESSED);

        if (is_working) {
            time(&current_sessions[current_session_index].end_time);
            current_session_index++;
            is_working = false;
        } else {
            time(&current_sessions[current_session_index].start_time);
            is_working = true;
        }

        set_event_bit(EVENT_BIT_UPDATE_DISPLAY);
    }
}

void timetracker_task(void *args) {
    wait_for_state(EVENT_BIT_WIFI_HANDLER_DONE);

    tutorial();

    const uint8_t priority = (uint8_t) (uintptr_t) args;
    xTaskCreate(display_mode_task, "display_mode_task", 4096, NULL, priority + 1, NULL);
    xTaskCreate(stamp_task, "stamp_task", 4096, NULL, priority + 2, NULL);

    update_display();

    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        if (wait_for_state_with_ms(EVENT_BIT_UPDATE_DISPLAY, 1000)) {
            update_display();
        } else {
            update_timers();
        }
    }
}

void init_timetracker_handler(const uint8_t priority) {
    xTaskCreate(timetracker_task, "timetracker_task", 4096, (void *) (uintptr_t) priority + 1, priority + 1, NULL);
}
