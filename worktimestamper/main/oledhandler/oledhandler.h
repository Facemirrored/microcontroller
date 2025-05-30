#ifndef OLEDHANDLER_H
#define OLEDHANDLER_H

#include <stdint.h>

typedef enum {
    MODE_CONTINUES_TEXT,
    MODE_LINE_BY_LINE,
} OledMode;

void oled_init(void);

void set_cursor(uint8_t column, uint8_t page);

void clear_display();

void send_char(char character);

void send_text_with_delay(const char *text, int8_t delay_ms);

void send_text(const char *text);

void send_text_at(const char *text, uint8_t column, uint8_t page);

void send_text_at_once(const char *text, uint8_t column, uint8_t page);

void send_page_20x8_no_clear(const char *full_text_page[]);

void send_page_20x8_no_clear_with_delay(const char *full_text_page[], uint8_t delay);

void send_page_20x8(const char *full_text_page[]);

void send_page_20x8_with_delay(const char *full_text_page[], uint8_t delay);

#endif
