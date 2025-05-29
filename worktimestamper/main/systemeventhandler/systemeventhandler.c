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

bool event_bit_is_set(const SystemEventBit system_event_bit) {
    const EventBits_t bits = xEventGroupGetBits(system_event_group);
    if ((bits & system_event_bit) != 0) {
        clear_event_bit(system_event_bit);
        return true;
    }

    return false;
}
