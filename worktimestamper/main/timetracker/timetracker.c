#include <freertos/FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <freertos/projdefs.h>
#include <freertos/task.h>

#include "oledhandler.h"
#include "systemeventhandler.h"
#include "timetracker.h"

#include <esp_log.h>
#include <portmacro.h>
#include <stdio.h>
#include <time.h>

#define MAX_SESSIONS 6

enum {
    HEADER_TIME_MODE = 0,
    NET_TIME_ROW = 7,
    SUMMARY_TABLE_HEADER = 1,
} TIMETRACKER_OLED_OUTPUT;

struct WorkTimeSession {
    time_t start_time;
    time_t end_time;
};

int last_minute = 0;
int last_second = 0;

bool is_summary_mode = false;
bool is_working = false;

struct WorkTimeSession current_sessions[MAX_SESSIONS] = {0};
uint8_t current_session_index = 0;

const time_t TEN_HOURS = 10 * 3600;
time_t time_to_stop_working = 0;

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

time_t calc_working_times() {
    time_t work_time = 0;

    ESP_LOGI("CALC", "start with zero (work_time=%lld)", work_time);

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (current_sessions[i].start_time != 0 && current_sessions[i].end_time != 0) {
            ESP_LOGI("CALC", "current_session = %d, end_time = %d, start_time = %d",
                     i, current_sessions[i].end_time, current_sessions[i].start_time);

            work_time += current_sessions[i].end_time - current_sessions[i].start_time;

            ESP_LOGI("CALC", "new work_time = %lld", work_time);
            continue;
        }

        if (current_sessions[i].start_time != 0) {
            time_t now;
            time(&now);

            ESP_LOGI("CALC", "add start_time = %d to work_time = %lld",
                     current_sessions[i].start_time, work_time);

            work_time += now - current_sessions[i].start_time;

            ESP_LOGI("CALC", "new work_time = %lld", work_time);
        }
    }

    const time_t first_start_time = current_sessions[0].start_time;
    if (first_start_time != 0 && work_time != 0) {
        time_to_stop_working = (first_start_time + TEN_HOURS) - (first_start_time + work_time);
    }

    ESP_LOGI("CALC", "calculated work_time: %lld", work_time);
    return work_time;
}

void display_working() {
    char buffer[8];
    strncpy(buffer, is_working ? "working" : "pausing", sizeof(buffer));
    send_text_at(buffer, 14, HEADER_TIME_MODE);

    const time_t work_time = calc_working_times();

    const int hours = work_time / 3600;
    const int minutes = (work_time % 3600) / 60;
    const int seconds = work_time % 60;

    ESP_LOGI("DISPLAY-WORKING", "work_time = %lld, hours = %d, minutes = %d", work_time, hours, minutes);
    char net_work_time_buffer[21];
    const int len = snprintf(net_work_time_buffer, sizeof(net_work_time_buffer), "net work: %02d:%02d:%02d  ",
                             hours, minutes, seconds);
    ESP_LOGI("DISPLAY-WORKING", "net_work_time_buffer = %s", net_work_time_buffer);

    if (len >= sizeof(buffer)) {
        // we need this because the compiler is stupid and doesn't know that snprintf returns the number of bytes written,
        // but we do, so it is okay
        ESP_LOGW("SNPRINTF", "Buffer truncated!");
    }

    send_high_prio_text_at_row(net_work_time_buffer, NET_TIME_ROW);
    send_text_at_row("                    ", SUMMARY_TABLE_HEADER);
    send_text_at_row("                    ", SUMMARY_TABLE_HEADER + 1);
    send_text_at_row("                    ", SUMMARY_TABLE_HEADER + 2);
    send_text_at_row("                    ", SUMMARY_TABLE_HEADER + 3);
    send_text_at_row("                    ", SUMMARY_TABLE_HEADER + 4);
    send_text_at_row("                    ", SUMMARY_TABLE_HEADER + 5);
}

void display_summary() {
    send_text_at("summary", 14, HEADER_TIME_MODE);
    send_text_at_row("start |  end  | net ", SUMMARY_TABLE_HEADER);

    time_t sum_net_time = 0;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        const struct WorkTimeSession *session = &current_sessions[i];

        ESP_LOGI("DISPLAY-SUMMARY", "session->start_time = %lld, session->end_time = %lld",
                 session->start_time, session->end_time);

        if (session->start_time == 0 && session->end_time == 0) {
            continue;
        }

        char buffer[21];

        struct tm start_time_tm;
        localtime_r(&session->start_time, &start_time_tm);

        if (session->end_time == 0) {
            snprintf(buffer, sizeof(buffer), "%02d:%02d | --:-- |--:--", start_time_tm.tm_hour, start_time_tm.tm_min);
            send_text_at_row(buffer, i + 2);
            continue;
        }

        struct tm end_time_tm;
        localtime_r(&session->start_time, &end_time_tm);

        const time_t duration = session->end_time - session->start_time;
        sum_net_time += duration;

        const int dh = duration / 3600;
        const int dm = (duration % 3600) / 60;

        const int len = snprintf(buffer, sizeof(buffer), "%02d:%02d | %02d:%02d |%02d:%02d",
                                 start_time_tm.tm_hour, start_time_tm.tm_min,
                                 end_time_tm.tm_hour, end_time_tm.tm_min,
                                 dm, dh);

        if (len >= sizeof(buffer)) {
            // we need this because the compiler is stupid and doesn't know that snprintf returns the number of bytes written,
            // but we do, so it is okay
            ESP_LOGW("SNPRINTF", "Buffer truncated!");
        }

        send_text_at_row(buffer, i + 2);
    }

    send_text_at_row("                    ", NET_TIME_ROW);
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

        is_summary_mode = !is_summary_mode;

        if (is_summary_mode) {
            display_summary();
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
            current_sessions[current_session_index] = (struct WorkTimeSession){
                .start_time = 0,
                .end_time = 0,
            };
            time(&current_sessions[current_session_index].end_time);
            current_session_index++;
        } else {
            time(&current_sessions[current_session_index].start_time);
        }

        is_working = !is_working;
        display_working();
    }
}

void clock_task() {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        // we don't update display if the tutorial is printed
        if (event_bit_is_set(EVENT_BIT_TUTORIAL_ACTIVE)) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        const struct tm time_info = get_current_time();

        if (last_second != time_info.tm_sec) {
            last_second = time_info.tm_sec;

            char buffer[9];
            snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
                     time_info.tm_hour,
                     time_info.tm_min,
                     time_info.tm_sec);

            send_text_at_row(buffer, HEADER_TIME_MODE);

            if (!is_summary_mode) {
                display_working();
            }
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


    // clock print task (current time and net time etc.)
    xTaskCreate(clock_task, "clock_task", 4096, NULL, priority, NULL);
    // track button 1 press for stamping
    xTaskCreate(track_stamp_task, "track_stamp_task", 4096, NULL, priority + 2, NULL);
    // track button 2 press for switching display
    xTaskCreate(track_display_switch_mode_task, "track_display_switch_mode_task", 4096, NULL, priority + 1, NULL);
}
