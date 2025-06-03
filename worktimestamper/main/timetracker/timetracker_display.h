#ifndef TIMETRACKER_DISPLAY_H
#define TIMETRACKER_DISPLAY_H

#include "timetracker_state.h"

// Show header and net work time
void display_working(const TimeTrackerState *state);

// Show table summary of all sessions
void display_summary(const TimeTrackerState *state);

// Show current time (HH:MM:SS) at top row
void display_clock(const struct tm *time_info);

// show tutorial
void display_tutorial(void);
#endif