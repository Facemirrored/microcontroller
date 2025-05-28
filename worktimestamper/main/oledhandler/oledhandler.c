#include "oledhandler.h"
#include "font5x7.h"

#include <string.h>

#include "driver/i2c.h"

// https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SCL_IO GPIO_NUM_19
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

#define SSD1306_ADDR 0x3C

// control byte - needed before setting a command
#define COMMAND_CONTROL_BYTE 0x00

// 0xAE: Display OFF
#define DISPLAY_OFF_COMMAND 0xAE

// 0xD5: Set Display Clock Divide Ratio - default is enough
#define DISPLAY_CLOCK_DIVIDE_RATIO_COMMAND 0xD5
#define DISPLAY_CLOCK_DIVIDE_RATIO_ARG_DEFAULT 0x80

// 0xA8: Set Multiplex Ratio - for 128x64 display
#define SET_MULTIPLEX_RATIO_COMMAND 0xA8
#define MULTIPLEX_RATIO_128X64 0x3F

// 0xD3: Set Display Offset - no offset
#define SET_DISPLAY_OFFSET_COMMAND 0xD3
#define DISPLAY_OFFSET_NONE 0x00

// 0x40: Set Display Start Line (0)
#define SET_START_LINE_COMMAND 0x40

// 0x8D: Charge Pump Setting - enable internal pump
#define CHARGE_PUMP_SETTING_COMMAND 0x8D
#define CHARGE_PUMP_ENABLE 0x14

// 0x20: Set Memory Addressing Mode - horizontal
#define MEMORY_ADDR_MODE_COMMAND 0x20
#define MEMORY_ADDR_MODE_HORIZONTAL 0x00

// 0xA1: Segment Remap - mirror horizontally
#define SEGMENT_REMAP_COMMAND 0xA1

// 0xC8: COM Output Scan Direction - mirror vertically (COM63→COM0)
#define COM_OUTPUT_SCAN_DIR_COMMAND 0xC8

// 0xDA: Set COM Pins Hardware Config - suitable for 128x64
#define COM_PINS_CONFIG_COMMAND 0xDA
#define COM_PINS_CONFIG_128X64 0x12

// 0x81: Set Contrast Control - brightness
#define SET_CONTRAST_COMMAND 0x81
#define DEFAULT_CONTRAST 0xCF

// 0xD9: Set Pre-charge Period - default value
#define SET_PRECHARGE_PERIOD_COMMAND 0xD9
#define DEFAULT_PRECHARGE_PERIOD 0xF1

// 0xDB: Set VCOMH Deselect Level - pixel OFF voltage level
#define SET_VCOMH_DESELECT_LEVEL_COMMAND 0xDB
#define DEFAULT_VCOMH_LEVEL 0x40

// 0xA4: Entire Display ON (resume RAM content)
#define ENTIRE_DISPLAY_ON_RESUME_COMMAND 0xA4

// 0xA6: Set Normal/Inverse Display - normal (non-inverted)
#define NORMAL_DISPLAY_COMMAND 0xA6

// 0xAF: Display ON
#define DISPLAY_ON_COMMAND 0xAF

static const uint8_t init_sequence[] = {
    COMMAND_CONTROL_BYTE, DISPLAY_OFF_COMMAND,
    COMMAND_CONTROL_BYTE, DISPLAY_CLOCK_DIVIDE_RATIO_COMMAND, DISPLAY_CLOCK_DIVIDE_RATIO_ARG_DEFAULT,
    COMMAND_CONTROL_BYTE, SET_MULTIPLEX_RATIO_COMMAND, MULTIPLEX_RATIO_128X64,
    COMMAND_CONTROL_BYTE, SET_DISPLAY_OFFSET_COMMAND, DISPLAY_OFFSET_NONE,
    COMMAND_CONTROL_BYTE, SET_START_LINE_COMMAND,
    COMMAND_CONTROL_BYTE, CHARGE_PUMP_SETTING_COMMAND, CHARGE_PUMP_ENABLE,
    COMMAND_CONTROL_BYTE, MEMORY_ADDR_MODE_COMMAND, MEMORY_ADDR_MODE_HORIZONTAL,
    COMMAND_CONTROL_BYTE, SEGMENT_REMAP_COMMAND,
    COMMAND_CONTROL_BYTE, COM_OUTPUT_SCAN_DIR_COMMAND,
    COMMAND_CONTROL_BYTE, COM_PINS_CONFIG_COMMAND, COM_PINS_CONFIG_128X64,
    COMMAND_CONTROL_BYTE, SET_CONTRAST_COMMAND, DEFAULT_CONTRAST,
    COMMAND_CONTROL_BYTE, SET_PRECHARGE_PERIOD_COMMAND, DEFAULT_PRECHARGE_PERIOD,
    COMMAND_CONTROL_BYTE, SET_VCOMH_DESELECT_LEVEL_COMMAND, DEFAULT_VCOMH_LEVEL,
    COMMAND_CONTROL_BYTE, ENTIRE_DISPLAY_ON_RESUME_COMMAND,
    COMMAND_CONTROL_BYTE, NORMAL_DISPLAY_COMMAND,
    COMMAND_CONTROL_BYTE, DISPLAY_ON_COMMAND,
};

static esp_err_t ssd1306_send_init_sequence(void) {
    return i2c_master_write_to_device(I2C_MASTER_NUM,
                                      SSD1306_ADDR,
                                      init_sequence,
                                      sizeof(init_sequence),
                                      pdMS_TO_TICKS(1000)
    );
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
}

/**
 * Set the cursor to display Position.
 * @param column column from 0 to 127
 * @param page page (one page = 8 pixels)
 */
void set_cursor(const uint8_t column, const uint8_t page) {
    const uint8_t cmds[] = {
        0x00, 0x21, column, 127,
        0x00, 0x22, page, 7
    };
    i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, cmds, sizeof(cmds), pdMS_TO_TICKS(1000));
}

static void clear_display() {
    const uint8_t clear_data[1024] = {0};

    uint8_t data_buf[1 + 1024];
    data_buf[0] = COMMAND_CONTROL_BYTE;
    memcpy(data_buf + 1, clear_data, 1024);

    // 0x21 column address -> from column 0 (0x00) to column 127 (0x7F)
    i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, (uint8_t[]){COMMAND_CONTROL_BYTE, 0x21, 0x00, 0x7F}, 4,
                               pdMS_TO_TICKS(100));
    // 0x22 page address (one page = 8 pixels) -> from page 0 (0x00) to page 7 (0x07)
    i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, (uint8_t[]){COMMAND_CONTROL_BYTE, 0x22, 0x00, 0x07}, 4,
                               pdMS_TO_TICKS(100));
    i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, data_buf, sizeof(data_buf), pdMS_TO_TICKS(1000));
}

void send_char(const char character) {
    const uint8_t *glyph = getFontData(character);

    // write glyph with data control byte (0x40) at beginning and space at ending
    const uint8_t i2c_data[7] = {0x40, glyph[0], glyph[1], glyph[2], glyph[3], glyph[4], 0x00};
    i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDR, i2c_data, sizeof(i2c_data), pdMS_TO_TICKS(1000));
}

void send_text(const char *text) {
    clear_display();
    set_cursor(0, 0);

    for (int i = 0; text[i] != '\0'; i++) {
        send_char(text[i]);
    }
}
