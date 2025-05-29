#ifndef BUTTONISRHANDLER_H
#define BUTTONISRHANDLER_H

#include "freertos/FreeRTOS.h"

typedef void (*button_callback_t)(void);

void create_button_isr_handler(button_callback_t callbackBtn1, button_callback_t callbackBtn2);
void init_button_isr_handler(int priority);

#endif