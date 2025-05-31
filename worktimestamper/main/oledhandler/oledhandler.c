#include "oledhandler.h"
#include "font5x7.h"
#include "commands.h"

#include "esp_log.h"
#include <string.h>
#include "driver/i2c.h"

// https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SCL_IO GPIO_NUM_19
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

#define MSG_QUEUE_LEN 20
#define DISPLAY_CACHE_SIZE 20
#define MAX_HIGH_PRIO_PER_CYCLE 3

QueueHandle_t high_priority_queue;
QueueHandle_t normal_priority_queue;

static esp_err_t ssd1306_send_init_sequence(void) {
    return i2c_master_write_to_device(I2C_MASTER_NUM,
                                      SSD1306_ADDR,
                                      init_sequence,
                                      sizeof(init_sequence),
                                      pdMS_TO_TICKS(1000)
    );
}

void clear_display() {
    const uint8_t clear_data[1024] = {0};

    uint8_t data_buf[1 + 1024];
    data_buf[0] = 0x40;
    memcpy(data_buf + 1, clear_data, 1024);

    // 0x21 column address -> from column 0 (0x00) to column 127 (0x7F)
    i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, (uint8_t[]){COMMAND_CONTROL_BYTE, 0x21, 0x00, 0x7F}, 4,
                               pdMS_TO_TICKS(100));
    // 0x22 page address (one page = 8 pixels) -> from page 0 (0x00) to page 7 (0x07)
    i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, (uint8_t[]){COMMAND_CONTROL_BYTE, 0x22, 0x00, 0x07}, 4,
                               pdMS_TO_TICKS(100));
    i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, data_buf, sizeof(data_buf), pdMS_TO_TICKS(1000));
}

void set_cursor(const uint8_t column, const uint8_t row) {
    const uint8_t pixel_start = column * 6;

    // Column Address
    const uint8_t cmd_col[] = {
        COMMAND_CONTROL_BYTE, 0x21, pixel_start, pixel_start + 5
    };

    // Page Address
    const uint8_t cmd_page[] = {
        COMMAND_CONTROL_BYTE, 0x22, row, row
    };

    ESP_ERROR_CHECK(
        i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, cmd_col, sizeof(cmd_col), pdMS_TO_TICKS(100)));
    ESP_ERROR_CHECK(
        i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, cmd_page, sizeof(cmd_page), pdMS_TO_TICKS(100)));
}

void send_char(const char character) {
    ESP_LOGI("PRINT CHAR", "%c", character);

    const uint8_t *glyph = getFontData(character);

    // write glyph with data control byte (0x40) at the beginning and space at the ending
    const uint8_t i2c_data[7] = {0x40, glyph[0], glyph[1], glyph[2], glyph[3], glyph[4], 0x00};
    i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, i2c_data, sizeof(i2c_data), pdMS_TO_TICKS(1000));
}

TextMessage_t create_display_message(const char *text, const uint8_t column, const uint8_t row) {
    TextMessage_t msg = {
        .column = column,
        .row = row,
    };

    strncpy(msg.text, text, sizeof(msg.text) - 1);
    msg.text[sizeof(msg.text) - 1] = '\0';

    ESP_LOGI("NEW DISPLAY TEXT", "%s", msg.text);

    return msg;
}

void send_text_at(const char *text, const uint8_t column, const uint8_t row) {
    const TextMessage_t message = create_display_message(text, column, row);
    xQueueSend(normal_priority_queue, &message, 0);
}

void send_text_at_row(const char *text, const uint8_t row) {
    send_text_at(text, 0, row);
}

void send_high_prio_text_at(const char *text, const uint8_t column, const uint8_t row) {
    const TextMessage_t message = create_display_message(text, column, row);
    xQueueSend(high_priority_queue, &message, 0);
}

void send_high_prio_text_at_row(const char *text, const uint8_t row) {
    send_high_prio_text_at(text, 0, row);
}

void process_display_message(const TextMessage_t *message) {
    set_cursor(message->column, message->row);

    ESP_LOGI("PRINT MESSAGE", "%s", message->text);

    for (int i = 0; message->text[i] != '\0'; i++) {
        set_cursor(message->column + i, message->row);
        send_char(message->text[i]);
    }
}

void display_task() {
    TextMessage_t message;
    uint8_t priority_counter = 0;

    // ReSharper disable once CppDFAEndlessLoop
    for (;;) {
        // first, we always will send the high-priority messages (by 3 times)
        while (priority_counter < MAX_HIGH_PRIO_PER_CYCLE
               && xQueueReceive(high_priority_queue, &message, 0) == pdPASS) {
            process_display_message(&message);
            priority_counter++;
        }

        // then we will send normal-priority messages (reset counter for prio another queue again)
        if (xQueueReceive(normal_priority_queue, &message, 0) == pdPASS) {
            process_display_message(&message);
            priority_counter = 0;
        }
    }
}

void send_page_20x8(const char *full_text_page[]) {
    if (full_text_page == NULL) return;

    char line[21];

    for (int i = 0; i < 8; i++) {
        if (full_text_page[i] == NULL) return;

        strncpy(line, full_text_page[i], 20);
        line[20] = '\0';
        send_text_at(line, 0, i);
    }
}

void init_oled(void) {
    const i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));

    ESP_ERROR_CHECK(i2c_driver_install(
        I2C_MASTER_NUM,
        conf.mode,
        I2C_MASTER_RX_BUF_DISABLE,
        I2C_MASTER_TX_BUF_DISABLE,
        0
    ));

    ESP_ERROR_CHECK(ssd1306_send_init_sequence());

    high_priority_queue = xQueueCreate(MSG_QUEUE_LEN, sizeof(TextMessage_t));
    normal_priority_queue = xQueueCreate(MSG_QUEUE_LEN, sizeof(TextMessage_t));

    clear_display();

    xTaskCreate(display_task, "display_task", 4096, NULL, 1, NULL);
}
