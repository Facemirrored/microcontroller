#ifndef OLEDHANDLER_H
#define OLEDHANDLER_H

void oled_init(void);

void set_cursor(uint8_t column, uint8_t page);

static void clear_display();

void send_char(char character);

void send_text(const char *text);

#endif
