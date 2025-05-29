#ifndef SYSTEMEVENTHANDLER_H
#define SYSTEMEVENTHANDLER_H

#include <freertos/FreeRTOS.h>
#include "freertos/event_groups.h"

typedef enum {
    EVENT_BIT_WIFI_CONNECTED = BIT0,
    EVENT_BIT_WIFI_HANDLER_DONE = BIT1,
    EVENT_BIT_BUTTON_1_PRESSED = BIT2,
    EVENT_BIT_BUTTON_2_PRESSED = BIT3,
} SystemEventBit;

extern EventGroupHandle_t system_event_group;

EventBits_t wait_for_state(const SystemEventBit system_event_bit);
void init_system_event_group();
void set_event_bit(const SystemEventBit system_event_bit);
void clear_event_bit(const SystemEventBit system_event_bit);
bool event_bit_is_set(const SystemEventBit system_event_bit);

#endif
