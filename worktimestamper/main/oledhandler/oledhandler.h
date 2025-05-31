#ifndef OLEDHANDLER_H
#define OLEDHANDLER_H

#include <stdint.h>

typedef struct {
    uint8_t column;
    uint8_t row;
    char text[21];
} TextMessage_t;

void init_oled(void);

void set_cursor(uint8_t column, uint8_t row);

void clear_display_and_queue();

void send_text_at(const char *text, uint8_t column, uint8_t row);

void send_high_prio_text_at(const char *text, uint8_t column, uint8_t row);

void send_text_at_row(const char *text, uint8_t row);

void send_high_prio_text_at_row(const char *text, uint8_t row);
#endif
