idf_component_register(SRCS
        "worktimestamper.c"
        "buttonisrhandler/buttonisrhandler.c"
        "oledhandler/oledhandler.c"
        "wifihandler/wifisynchandler.c"
        "timetracker/timetracker.c"
        "systemeventhandler/systemeventhandler.c"
        INCLUDE_DIRS "." "buttonisrhandler" "oledhandler" "wifihandler" "timetracker" "systemeventhandler")
