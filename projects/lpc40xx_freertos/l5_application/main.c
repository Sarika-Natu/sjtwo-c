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

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

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

static void create_blinky_tasks(void) {
  /**
   * Use '#if (1)' if you wish to observe how two tasks can blink LEDs
   * Use '#if (0)' if you wish to use the 'periodic_scheduler.h' that will spawn 4 periodic tasks, one for each LED
   */
#if (1)
  // These variables should not go out of scope because the 'blink_task' will reference this memory
  static gpio_s led0, led1;

  led0 = board_io__get_led0();
  led1 = board_io__get_led1();

  xTaskCreate(blink_task, "led0", configMINIMAL_STACK_SIZE, (void *)&led0, PRIORITY_LOW, NULL);
  xTaskCreate(blink_task, "led1", configMINIMAL_STACK_SIZE, (void *)&led1, PRIORITY_LOW, NULL);
#else
  const bool run_1000hz = true;
  const size_t stack_size_bytes = 2048 / sizeof(void *); // RTOS stack size is in terms of 32-bits for ARM M4 32-bit CPU
  periodic_scheduler__initialize(stack_size_bytes, !run_1000hz); // Assuming we do not need the high rate 1000Hz task
  UNUSED(blink_task);
#endif
}

static void create_uart_task(void) {
  // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
  // Change '#if (0)' to '#if (1)' and vice versa to try it out
#if (0)
  // printf() takes more stack space, size this tasks' stack higher
  xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#else
  sj2_cli__init();
  UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
#endif
}

static void blink_task(void *params) {
  const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    gpio__toggle(led);
    vTaskDelay(500);
  }
}

// This sends periodic messages over printf() which uses system_calls.c to send them to UART0
static void uart_task(void *params) {
  TickType_t previous_tick = 0;
  TickType_t ticks = 0;

  while (true) {
    // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
    vTaskDelayUntil(&previous_tick, 2000);

    /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
     * sent out before this function returns. See system_calls.c for actual implementation.
     *
     * Use this style print for:
     *  - Interrupts because you cannot use printf() inside an ISR
     *    This is because regular printf() leads down to xQueueSend() that might block
     *    but you cannot block inside an ISR hence the system might crash
     *  - During debugging in case system crashes before all output of printf() is sent
     */
    ticks = xTaskGetTickCount();
    fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
    fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

    /* This deposits data to an outgoing queue and doesn't block the CPU
     * Data will be sent later, but this function would return earlier
     */
    ticks = xTaskGetTickCount();
    printf("This is a more efficient printf ... finished in");
    printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
  }
}
