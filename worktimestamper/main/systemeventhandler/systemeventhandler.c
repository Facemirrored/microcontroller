#include "systemeventhandler.h"

EventGroupHandle_t system_event_group;

void init_system_event_group() {
    system_event_group = xEventGroupCreate();
}

bool wait_for_state(const SystemEventBit system_event_bit) {
    while (system_event_group == NULL) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    const EventBits_t bits = xEventGroupWaitBits(system_event_group, system_event_bit, pdTRUE, pdTRUE, portMAX_DELAY);

    return (bits & system_event_bit) != 0;
}

bool wait_for_state_with_ms(const SystemEventBit system_event_bit, const int ms) {
    while (system_event_group == NULL) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    const EventBits_t bits = xEventGroupWaitBits(
        system_event_group,
        system_event_bit,
        pdTRUE,
        pdTRUE,
        pdMS_TO_TICKS(ms));

    return (bits & system_event_bit) != 0;
}

void set_event_bit(const SystemEventBit system_event_bit) {
    xEventGroupSetBits(system_event_group, system_event_bit);
}

void set_event_bit_isr(const SystemEventBit system_event_bit) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(system_event_group, system_event_bit, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void clear_event_bit(const SystemEventBit system_event_bit) {
    xEventGroupClearBits(system_event_group, system_event_bit);
}

bool event_bit_is_set(const SystemEventBit system_event_bit) {
    const EventBits_t bits = xEventGroupGetBits(system_event_group);
    return (bits & system_event_bit) != 0;
}
