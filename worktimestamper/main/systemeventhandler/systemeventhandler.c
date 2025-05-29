#include "systemeventhandler.h"

EventGroupHandle_t system_event_group;

void init_system_event_group() {
    system_event_group = xEventGroupCreate();
}

EventBits_t wait_for_state(const SystemEventBit system_event_bit) {
    return xEventGroupWaitBits(system_event_group, system_event_bit, pdFALSE, pdTRUE, portMAX_DELAY);
}

void set_event_bit(const SystemEventBit system_event_bit) {
    xEventGroupSetBits(system_event_group, system_event_bit);
}

void clear_event_bit(const SystemEventBit system_event_bit) {
    xEventGroupClearBits(system_event_group, system_event_bit);
}