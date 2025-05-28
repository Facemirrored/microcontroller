#ifndef TIMETRACKER_H
#define TIMETRACKER_H

typedef struct Worktime Worktime {
    time_t start_time = 0;
    time_t end_time = 0;
} Worktime;

inline bool worktime_finished(const Worktime *worktime) {
    if (!worktime) return false;
    return worktime->end_time != 0;
}

inline void worktime_start(Worktime *worktime, const time_t start_time) {
    if (!worktime) return;
    worktime->start_time = start_time;
}

inline void worktime_end(Worktime *worktime, const time_t end_time) {
    if (!worktime) return;
    worktime->end_time = end_time;
}

typedef struct Timetracker Timetracker {
    Worktime work_sessions[3];

    int active_session = 0;
    bool working = false;
} Timetracker;

inline Timetracker *timetracker_create() {
    return calloc(1, sizeof(Timetracker));
};

inline void timetracker_destroy(Timetracker *tracker) {
    free(tracker);
};

inline Worktime* get_active_session(Timetracker *tracker) {
    return &tracker->work_sessions[tracker->active_session];
}

inline void start_working(Timetracker *tracker) {
    if (tracker->active_session >= 2) {
        return;
    }
    Worktime *active_session = get_active_session(tracker);
    time(&active_session->start_time);
}

inline void stop_working(Timetracker *tracker) {
    Worktime *active_session = get_active_session(tracker);
    time(&active_session->end_time);
    tracker->active_session++;
}

#endif
