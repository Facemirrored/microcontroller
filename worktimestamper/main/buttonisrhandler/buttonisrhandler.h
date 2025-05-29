#ifndef BUTTONISRHANDLER_H
#define BUTTONISRHANDLER_H

#include "freertos/FreeRTOS.h"

typedef void (*button_callback_t)(void);

void init_button_isr_handler(int priority);

#endif