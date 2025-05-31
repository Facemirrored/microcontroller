#ifndef OLEDHANDLER_H
#define OLEDHANDLER_H

#include <stdint.h>

typedef struct {
    uint8_t column;
    uint8_t row;
    char text[21];
} TextMessage_t;

void init_oled(void);

void send_pixel(uint8_t column, uint8_t page, uint8_t pixel_bit);

void set_cursor(uint8_t column, uint8_t row);

void clear_display();

void send_page_20x8(const char *full_text_page[]);

void send_text_at(const char *text, uint8_t column, uint8_t row);

void send_high_prio_text_at(const char *text, uint8_t column, uint8_t row);

void send_text_at_row(const char *text, uint8_t row);

void send_high_prio_text_at_row(const char *text, uint8_t row);
#endif
