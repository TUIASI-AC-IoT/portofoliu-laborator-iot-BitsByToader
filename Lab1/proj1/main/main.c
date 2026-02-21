#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

#define GPIO_OUTPUT_IO      4
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT_IO)

#define BTN_INPUT_IO        2
#define BTN_INPUT_PIN_SEL   (1ULL<<BTN_INPUT_IO)

void button_task(void *params) {
    int btn_state_prev = gpio_get_level(BTN_INPUT_IO);
    
    for(;;) {
        int btn_state = gpio_get_level(BTN_INPUT_IO);
        if (btn_state != btn_state_prev) { // got a change
            // wait for 200ms to settle
            vTaskDelay(pdMS_TO_TICKS(200));

            btn_state = gpio_get_level(BTN_INPUT_IO);
            if (btn_state != btn_state_prev) {
                // stable press, debounced.
                // wait for 500ms for long press
                vTaskDelay(pdMS_TO_TICKS(500));
                btn_state = gpio_get_level(BTN_INPUT_IO);
                if (btn_state != btn_state_prev) {
                    printf("debounced long btn: %d\n", btn_state);
                } else {
                    printf("debounced short btn: %d\n", btn_state);
                }
            }
        }
        
        btn_state_prev = btn_state;
        vTaskDelay(pdMS_TO_TICKS(1)); // 1ms delay
    }
}

void app_main() {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    gpio_config_t btn_conf = {
        BTN_INPUT_PIN_SEL,  //bitmask
        GPIO_MODE_INPUT,    //input
        1,                  //pullup
        0,                  //pulldown
        GPIO_INTR_DISABLE   //irq
    };
    gpio_config(&btn_conf);

    // create task for button mgmt
    TaskHandle_t btn_task_h = NULL;
    xTaskCreate(button_task, "BTN_TASK", 16, NULL, 2, &btn_task_h); // prio 2, 16 stack size

    int cnt = 0;
    while(1) {
        printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_OUTPUT_IO, cnt % 2);
    }
}