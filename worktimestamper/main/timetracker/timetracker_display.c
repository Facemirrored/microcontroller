#include "timetracker_display.h"

#include <esp_log.h>

#include "timetracker_logic.h"
#include "oledhandler.h"

#include <stdio.h>
#include <string.h>
#include <systemeventhandler.h>

#define HEADER_ROW 0
#define LATEST_CHECKOUT_ROW 6
#define NET_WORK_TIME_OW 7
#define SUMMARY_HEADER_ROW 1

#define TIME_STRING_SIZE (sizeof("00:00:00"))
#define NET_WORK_STRING_SIZE (sizeof("net work: 00:00:00"))
#define EMPTY_TIME_STRING_SIZE (sizeof("--:-- | --:-- |--:--"))

static_assert(TIME_STRING_SIZE == 9, "Buffer size must be 19 bytes");
static_assert(NET_WORK_STRING_SIZE == 19, "Buffer size must be 19 bytes");
static_assert(EMPTY_TIME_STRING_SIZE == 21, "Buffer size must be 19 bytes");

void display_clock(const struct tm *time_info) {
    char time_buf[TIME_STRING_SIZE];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d",
             time_info->tm_hour, time_info->tm_min, time_info->tm_sec);

    send_text_at_row(time_buf, HEADER_ROW);
}

void display_working(const TimeTrackerState *state) {
    const char *mode = state->is_working ? "working" : "pausing";
    send_text_at(mode, 14, HEADER_ROW);

    const time_t work_time = calculate_work_time(state);
    const int h = (int) work_time / 3600;
    const int m = (int) (work_time % 3600) / 60;
    const int s = (int) work_time % 60;

    char temp[64];
    const int len = snprintf(temp, sizeof(temp), "net work: %02d:%02d:%02d", h, m, s);

    char buffer[NET_WORK_STRING_SIZE];
    if (len >= sizeof(buffer)) {
        memcpy(buffer, temp, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
    } else {
        strcpy(buffer, temp);
    }

    send_text_at_row(buffer, NET_WORK_TIME_OW);

    // Clear summary rows (only needed in this view)
    for (int i = 0; i < 5; i++) {
        send_text_at_row("                    ", SUMMARY_HEADER_ROW + i);
    }
}

void display_summary(const TimeTrackerState *state) {
    send_text_at("summary", 14, HEADER_ROW);
    send_text_at_row("start |  end  | net ", SUMMARY_HEADER_ROW);

    for (int i = 0; i < MAX_SESSIONS; i++) {
        const WorkTimeSession *s = &state->sessions[i];
        if (s->start_time == 0) continue;

        char buffer[EMPTY_TIME_STRING_SIZE];

        struct tm start_tm;
        localtime_r(&s->start_time, &start_tm);

        if (s->end_time == 0) {
            snprintf(buffer, sizeof(buffer), "%02d:%02d | --:-- |--:--",
                     start_tm.tm_hour, start_tm.tm_min);
        } else {
            struct tm end_tm;
            localtime_r(&s->end_time, &end_tm);
            const time_t dur = s->end_time - s->start_time;
            const int dh = (int) (dur / 3600);
            const int dm = (int) ((dur % 3600) / 60);

            char temp[64];
            const int len = snprintf(temp, sizeof(temp),
                                     "%02d:%02d | %02d:%02d |%02d:%02d",
                                     start_tm.tm_hour,
                                     start_tm.tm_min,
                                     end_tm.tm_hour,
                                     end_tm.tm_min,
                                     dh,
                                     dm);

            if (len >= sizeof(buffer)) {
                memcpy(buffer, temp, sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';
            } else {
                strcpy(buffer, temp);
            }
        }

        send_text_at_row(buffer, i + 2);
    }

    // Clear net row
    send_text_at_row("                    ", NET_WORK_TIME_OW);
}

void display_tutorial(void) {
    set_event_bit(EVENT_BIT_TUTORIAL_ACTIVE);

    const char *page[] = {
        "----time synched----",
        "--main program rdy--",
        "                    ",
        " left btn:   stamp  ",
        " right btn:  switch ",
        "                    ",
        "===> press left <===",
        "===>  to start  <===",
    };

    send_page_20x8(page);

    wait_for_state(EVENT_BIT_BUTTON_1_PRESSED);

    clear_event_bit(EVENT_BIT_TUTORIAL_ACTIVE);
    clear_display();
    vTaskDelay(pdMS_TO_TICKS(300));
}
