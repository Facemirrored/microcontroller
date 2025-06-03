#include "timetracker_logic.h"
#include <time.h>

bool handle_stamp(TimeTrackerState *state) {
    time_t now;
    time(&now);

    if (state->session_index >= MAX_SESSIONS) {
        return false;
    }

    WorkTimeSession *session = &state->sessions[state->session_index];

    if (state->is_working) {
        session->end_time = now;
        state->session_index++;
    } else {
        session->start_time = now;
        session->end_time = 0;
    }

    state->is_working = !state->is_working;
    return true;
}

time_t calculate_work_time(const TimeTrackerState *state) {
    time_t total = 0;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        const time_t start = state->sessions[i].start_time;
        const time_t end = state->sessions[i].end_time;

        if (start == 0) continue;

        if (end == 0) {
            time_t now;
            time(&now);
            total += now - start;
        } else {
            total += end - start;
        }
    }

    return total;
}
