#ifndef TIMETRACKER_LOGIC_H
#define TIMETRACKER_LOGIC_H

#include "timetracker_state.h"
#include <stdbool.h>

// Called when user presses "stamp" button
bool handle_stamp(TimeTrackerState *state);

// Calculates total worked time in seconds
time_t calculate_work_time(const TimeTrackerState *state);

#endif
