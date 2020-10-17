#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "lpc40xx.h"
#include "queue.h"


static QueueHandle_t switch_queue;
#define PLowCHigh
//#define PHighCLow
//#define PHighCHigh

typedef enum { switch__off, switch__on } switch_e;

static switch_e get_switch_input_from_switch(void) {
    const uint32_t port0_pin30 = (1 << 30);

    switch_e switch_value;
    if (LPC_GPIO0->PIN & port0_pin30) {
        switch_value = switch__on;
    } else {
        switch_value = switch__off;
    }
    return switch_value;
}

// TODO: Create this task at PRIORITY_LOW
void producer(void *p) {
    while (1) {
        // This xQueueSend() will internally switch context to "consumer" task because it is higher priority than this
        // "producer" task Then, when the consumer task sleeps, we will resume out of xQueueSend()and go over to the next
        // line

        // TODO: Get some input value from your board
        const switch_e switch_value = get_switch_input_from_switch();

        // TODO: Print a message before xQueueSend()
        printf("%s(), line %d, tx\n", __FUNCTION__, __LINE__);
// Note: Use printf() and not fprintf(stderr, ...) because stderr is a polling printf
#if 1
        if (!xQueueSend(switch_queue, &switch_value, 500)) {
            // TODO: Print a message after xQueueSend()
            printf("Queue send fail\n");
        } else {
            printf("        line %d, tx\n", __LINE__);
        }
#endif
        vTaskDelay(300);
    }
}

// TODO: Create this task at PRIORITY_HIGH
void consumer(void *p) {
    switch_e switch_value;
    while (1) {
        // TODO: Print a message before xQueueReceive()
        printf("%s(), line %d, rx \n", __FUNCTION__, __LINE__);
        if (xQueueReceive(switch_queue, &switch_value, portMAX_DELAY)) {
#if 0
            if (xQueueReceive(switch_queue, &switch_value, 0)) {
#endif
            // TODO: Print a message after xQueueReceive()
            printf("            line %d, Switch value: %d\n", __LINE__, switch_value);
        } else {
            printf("Queue receive failed\n");
        }
    }
}

static void initialize_gpio(void) {
    const uint32_t func_mask = 0x07;
    const uint32_t enable_gpio_func = 0x00;
    const uint32_t set_as_output = (1 << 30);

    LPC_IOCON->P0_30 &= ~func_mask;
    LPC_IOCON->P0_30 |= enable_gpio_func;
    LPC_GPIO0->DIR |= set_as_output;
}

int main(void) {
    create_blinky_tasks();
    create_uart_task();

    initialize_gpio();

// TODO: Create your tasks
#ifdef PHighCLow
    xTaskCreate(producer, "producer", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(consumer, "consumer", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
#endif

#ifdef PLowCHigh
    xTaskCreate(producer, "producer", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
    xTaskCreate(consumer, "consumer", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_HIGH, NULL);
#endif

#ifdef PHighCHigh
    xTaskCreate(producer, "producer", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(consumer, "consumer", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_HIGH, NULL);
#endif

    // TODO Queue handle is not valid until you create it
    // Choose depth of item being our enum (1 should be okay for this example)
    switch_queue = xQueueCreate(2, sizeof(switch_e));

    puts("Starting RTOS");
    vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

    return 0;
}


