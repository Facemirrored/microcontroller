#include "timetracker_state.h"
#include <string.h>

void init_timetracker_state(TimeTrackerState *state) {
    state->is_working = false;
    state->is_summary_mode = false;
    state->session_index = 0;
    memset(state->sessions, 0, sizeof(state->sessions));
}
