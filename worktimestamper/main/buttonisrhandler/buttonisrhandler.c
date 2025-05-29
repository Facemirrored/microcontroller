#include "buttonisrhandler.h"
#include "systemeventhandler.h"
#include "freertos/task.h"
#include <freertos/projdefs.h>
#include <portmacro.h>
#include <stdint.h>
#include <driver/gpio.h>

#define GPIO_BUTTON_1 GPIO_NUM_5
#define GPIO_BUTTON_2 GPIO_NUM_18

static QueueHandle_t button_isr_queue = NULL;

static void button_task(void *arg) {
    uint32_t io_num; // save the pressed GPIO number

    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        if (xQueueReceive(button_isr_queue, &io_num, portMAX_DELAY)) {
            if (io_num == GPIO_BUTTON_1) {
                set_event_bit(EVENT_BIT_BUTTON_1_PRESSED);
            }

            if (io_num == GPIO_BUTTON_2) {
                set_event_bit(EVENT_BIT_BUTTON_2_PRESSED);
            }

            vTaskDelay(pdMS_TO_TICKS(30)); // wait 50 ms for the debouncing the button press

            // remove all queue entries, which could be added due to the debounced effect
            void *dummy;
            while (xQueueReceive(button_isr_queue, &dummy, 0)) {
                // do nothing
            }
        }
    }
}

static gpio_config_t create_config() {
    const gpio_config_t config = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << GPIO_BUTTON_1) | (1ULL << GPIO_BUTTON_2),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };

    return config;
}

static void IRAM_ATTR button_isr_handler(void *arg) {
    const uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(button_isr_queue, &gpio_num, NULL);
}

void create_button_isr_handler() {
    if (button_isr_queue != NULL) {
        vQueueDelete(button_isr_queue);
        button_isr_queue = NULL;
    }

    const gpio_config_t button_config = create_config();
    gpio_config(&button_config);

    button_isr_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_BUTTON_1, button_isr_handler, (void *) GPIO_BUTTON_1);
    gpio_isr_handler_add(GPIO_BUTTON_2, button_isr_handler, (void *) GPIO_BUTTON_2);
}

void init_button_isr_handler(const int priority) {
    create_button_isr_handler();
    xTaskCreate(button_task, "button_task", 2048, NULL, priority, NULL);
}
