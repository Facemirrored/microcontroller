#include <nvs_flash.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>
#include <lwip/apps/sntp.h>

#include "rom/ets_sys.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "credentials.h"
#include "timetracker.h"

// EPS32-API: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/index.html
// ESP32-Datasheet: https://m.media-amazon.com/images/I/A1oyy-n8xfL.pdf
// LCD-Datasheet: https://www.mikrocontroller.net/articles/HD44780

// LCD output data
#define GPIO_LCD_D4 GPIO_NUM_33
#define GPIO_LCD_D5 GPIO_NUM_32
#define GPIO_LCD_D6 GPIO_NUM_22
#define GPIO_LCD_D7 GPIO_NUM_23

// LCD control
#define GPIO_LCD_RS GPIO_NUM_26 // 1 = write data, 0 = write commands
#define GPIO_LCD_E  GPIO_NUM_25 // Enable signal - need a HIGH -> LOW flank (1us) for the LCD to take the written data

// Buttons
#define GPIO_BUTTON_1 GPIO_NUM_14
#define GPIO_BUTTON_2 GPIO_NUM_27

// Build int LED
#define GPIO_LED GPIO_NUM_2

// commands
#define LCD_CLEAR_DISPLAY 0x01
#define LCD_CURSOR_START   0x02
#define LCD_ENTRY_MODE_CURSOR_TO_RIGHT_NO_SCROLL    0x06
#define LCD_4_BIT_2_LINES_5x7_FONT 0x28
#define LCD_DISPLAY_ON_CURSOR_OFF_BLINK_OFF 0x0C
#define LCD_CURSOR_NEXT_LINE 0xC0


/**
 * Negative flank (1us) for the LCD to take the written data which needs to be set to the GPIOs before.
 */
static void executeLCDQuery() {
    gpio_set_level(GPIO_LCD_E, 1);
    ets_delay_us(1);
    gpio_set_level(GPIO_LCD_E, 0);
    ets_delay_us(1);
}

static void sendNibble(const uint8_t nibble) {
    gpio_set_level(GPIO_LCD_D7, (nibble >> 3) & 0x1);
    gpio_set_level(GPIO_LCD_D6, (nibble >> 2) & 0x1);
    gpio_set_level(GPIO_LCD_D5, (nibble >> 1) & 0x1);
    gpio_set_level(GPIO_LCD_D4, (nibble >> 0) & 0x1);

    executeLCDQuery();
}

static void sendByte(const uint8_t byte, const bool is_data) {
    gpio_set_level(GPIO_LCD_RS, is_data); // 0 = command, 1 = data
    sendNibble((byte >> 4) & 0x0F); // send high nibble
    sendNibble(byte & 0x0F); // send low nibble
}

static void sendCommand(const uint8_t byte) {
    sendByte(byte, false);
    ets_delay_us(2000);
}

static void sendChar(const uint8_t byte) {
    sendByte(byte, true);
    ets_delay_us(50);
}

static void set4BitMode() {
    sendNibble(0x3);
    ets_delay_us(5000);

    sendNibble(0x3);
    ets_delay_us(1000);

    sendNibble(0x3);
    ets_delay_us(1000);

    sendNibble(0x2);
    ets_delay_us(1000);
}

static void initDisplay() {
    ets_delay_us(20000); // wait 20 ms after power up

    set4BitMode();

    sendCommand(LCD_4_BIT_2_LINES_5x7_FONT);
    sendCommand(LCD_DISPLAY_ON_CURSOR_OFF_BLINK_OFF);
    sendCommand(LCD_CLEAR_DISPLAY);
    sendCommand(LCD_CURSOR_START);
    sendCommand(LCD_ENTRY_MODE_CURSOR_TO_RIGHT_NO_SCROLL);
}

static bool buttonIsPressed(const int button) {
    return gpio_get_level(button) == 0;
}

static void configureGPIO() {
    // button read mode with pullup resistor
    gpio_set_direction(GPIO_BUTTON_1, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_BUTTON_1, GPIO_PULLUP_ONLY);

    gpio_set_direction(GPIO_BUTTON_2, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_BUTTON_2, GPIO_PULLUP_ONLY);

    // LCD
    gpio_set_direction(GPIO_LCD_RS, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LCD_E, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LCD_D4, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LCD_D5, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LCD_D6, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LCD_D7, GPIO_MODE_OUTPUT);
}

static void controlBuildInLED() {
    if (buttonIsPressed(GPIO_BUTTON_1) || buttonIsPressed(GPIO_BUTTON_2)) {
        gpio_set_level(GPIO_NUM_2, 1);
    } else {
        gpio_set_level(GPIO_NUM_2, 0);
    }
}

static void sendTextAtCurrentPosition(const char *text) {
    while (*text != '\0') {
        sendChar(*text);
        text++;
    }
}

static void sendTextAtStart(const char *text) {
    sendCommand(LCD_CURSOR_START);
    sendTextAtCurrentPosition(text);
}

static void sendTextAtSecondLine(const char *text) {
    sendCommand(LCD_CURSOR_NEXT_LINE);
    sendTextAtCurrentPosition(text);
}

static void sendMultiText(const char *text, const char *secondText) {
    sendTextAtStart(text);
    sendTextAtSecondLine(secondText);
}

static void initWIFI() {
    sendCommand(LCD_CLEAR_DISPLAY);
    sendTextAtStart("Init WIFI...    ");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    sendTextAtSecondLine("        ...done!");
    ets_delay_us(2 * 1000 * 1000); // wait 2 seconds
}

static void initTimeSync() {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1); // Central Europe
}

static void waitForTime() {
    sendCommand(LCD_CLEAR_DISPLAY);
    sendTextAtStart("Sync time...    ");

    time_t now = 0;
    struct tm timeInfo = {0};

    while (timeInfo.tm_year < 2020 - 1900) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeInfo);
    }

    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
    sendCommand(LCD_CLEAR_DISPLAY);
    sendTextAtStart(buffer);
}

void updateDisplayWithTime(int *lastMinute) {
    time_t now;
    struct tm timeInfo;

    time(&now);
    localtime_r(&now, &timeInfo);

    if (*lastMinute != timeInfo.tm_min) {
        *lastMinute = timeInfo.tm_min;

        char buffer[6];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
        sendTextAtStart(buffer);
    }
}

static void disconnectWIFI() {
    esp_wifi_disconnect();
    esp_wifi_stop();
    sendTextAtSecondLine("WIFI stopped    ");
    vTaskDelay(pdMS_TO_TICKS(2000));
    sendTextAtSecondLine("                ");
}

// TODO: better command functions (each simple part as define and then with & combine them)
void app_main(void) {
    ets_delay_us(20000);

    configureGPIO();

    // just to see if buttons are working
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

    initDisplay();
    initWIFI();
    sendMultiText("Connect WIFI... ", "...please wait. ");

    esp_netif_ip_info_t ip_info;
    bool connected = false;
    bool timeSynced = false;
    int lastMinute = -1;

    // ReSharper disable once CppDFAEndlessLoop
    while (true) {
        if (!connected
            && esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info) == ESP_OK
            && ip_info.ip.addr != 0) {
            sendTextAtSecondLine("Connected!      ");
            connected = true;
        }

        if (connected && !timeSynced) {
            initTimeSync();
            timeSynced = true;
            waitForTime();
            disconnectWIFI();
        }

        if (timeSynced) {
            updateDisplayWithTime(&lastMinute);
        }

        controlBuildInLED();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
