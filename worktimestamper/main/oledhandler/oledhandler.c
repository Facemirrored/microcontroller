#include "oledhandler.h"
#include "font5x7.h"
#include "commands.h"

#include <string.h>

#include "driver/i2c.h"

// https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SCL_IO GPIO_NUM_19
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

static OledMode oled_mode = MODE_LINE_BY_LINE;
static uint8_t current_page = 0;
static uint8_t current_column = 0;
static bool first_text = true;

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

    current_column = 0;
    current_page = 0;
    first_text = true;
}

void oled_init(void) {
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
    clear_display();
}

void set_cursor(const uint8_t column, const uint8_t page) {
    // Column Address
    const uint8_t cmd_col[] = {
        COMMAND_CONTROL_BYTE, 0x21, column, column + 5
    };

    // Page Address
    const uint8_t cmd_page[] = {
        COMMAND_CONTROL_BYTE, 0x22, page, page
    };

    ESP_ERROR_CHECK(
        i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, cmd_col, sizeof(cmd_col), pdMS_TO_TICKS(100)));
    ESP_ERROR_CHECK(
        i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, cmd_page, sizeof(cmd_page), pdMS_TO_TICKS(100)));
}

void send_char(const char character) {
    const uint8_t *glyph = getFontData(character);

    // write glyph with data control byte (0x40) at beginning and space at ending
    const uint8_t i2c_data[7] = {0x40, glyph[0], glyph[1], glyph[2], glyph[3], glyph[4], 0x00};
    i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, i2c_data, sizeof(i2c_data), pdMS_TO_TICKS(1000));
}

void set_mode_and_reset(const OledMode mode) {
    oled_mode = mode;
    clear_display();
    current_column = 0;
    current_page = 0;
}

#define MAX_COLUMNS 21
#define MAX_PAGES   8

void set_cursor_text_col(const uint8_t text_col, const uint8_t page) {
    const uint8_t pixel_col = text_col * 6;
    printf("Setting cursor to pixel_col = %d, page = %d\n", pixel_col, page);
    set_cursor(pixel_col, page);
}

void send_text(const char *text) {
    if (oled_mode == MODE_LINE_BY_LINE) {
        current_column = 0;

        if (!first_text) {
            current_page++;
        }
        if (current_page >= MAX_PAGES) {
            current_page = 0;
        }
    }

    for (int i = 0; text[i] != '\0'; i++) {
        set_cursor_text_col(current_column, current_page);
        send_char(text[i]);

        current_column++;
        if (current_column >= MAX_COLUMNS) {
            current_column = 0;
            current_page++;
            if (current_page >= MAX_PAGES) {
                current_page = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }

    first_text = false;
}

/**
 * Send text at the given position with updating column and page index.
 * @param text text
 * @param column column
 * @param page page
 */
void send_text_at(const char *text, const uint8_t column, const uint8_t page) {
    if (column >= MAX_PAGES) return;

    set_cursor_text_col(column, page);
    send_text(text);
}

/**
 *
 * @param text Send text at the given position without changing the column and page index.
 * @param column column
 * @param page page
 */
void send_text_at_once(const char *text, const uint8_t column, const uint8_t page) {
    const uint8_t temp_col = current_column;
    const uint8_t temp_page = current_page;

    send_text_at(text, column, page);

    current_column = temp_col;
    current_page = temp_page;
}
