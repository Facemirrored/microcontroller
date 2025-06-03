idf_component_register(SRCS
        "worktimestamper.c"
        "buttonisrhandler/buttonisrhandler.c"
        "oledhandler/oledhandler.c"
        "wifihandler/wifisynchandler.c"
        "timetracker/timetracker_state.c"
        "timetracker/timetracker_logic.c"
        "timetracker/timetracker_display.c"
        "timetracker/timetracker_controller.c"
        "systemeventhandler/systemeventhandler.c"
        INCLUDE_DIRS "." "buttonisrhandler" "oledhandler" "wifihandler" "systemeventhandler" "timetracker")
