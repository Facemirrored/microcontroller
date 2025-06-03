#ifndef TIMETRACKER_STATE_H
#define TIMETRACKER_STATE_H

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_SESSIONS 6

typedef struct {
    time_t start_time;
    time_t end_time;
} WorkTimeSession;

typedef struct {
    bool is_working;
    bool is_summary_mode;
    uint8_t session_index;
    WorkTimeSession sessions[MAX_SESSIONS];
} TimeTrackerState;

// init new timetracker state
void init_timetracker_state(TimeTrackerState *state);

#endif
