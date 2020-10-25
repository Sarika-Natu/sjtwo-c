#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "acceleration.h"
#include "event_groups.h"
#include "ff.h"
#include "queue.h"
#include <string.h>

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

void write_file_using_fatfs_pi(acceleration__axis_data_s *value);
void producer(void *p);
void consumer(void *p);

static QueueHandle_t switch_queue;
static EventGroupHandle_t watchdog_eventgrp;

#define BIT0 0x01
#define BIT1 0x02

void write_file_using_fatfs_pi(acceleration__axis_data_s *value) {
  const char *filename = "sensor.txt";
  FIL file;
  TickType_t ticks = 0;
  UINT bytes_written = 0;
  FRESULT result = f_open(&file, filename, (FA_WRITE | FA_OPEN_APPEND));

  for (char i = 0; i < 10; i++) {

    if (FR_OK == result) {
      char string[64];
      ticks = xTaskGetTickCount();
      sprintf(string, "| ticks:%li | x-axis=%i | y-axis=%i | z-axis=%i |\n", ticks, value->x, value->y, value->z);

      if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
      } else {
        printf("ERROR: Failed to write data to file\n");
      }
    } else {
      printf("ERROR: Failed to open: %s\n", filename);
    }
    value++;
    f_sync(&file);
  }

  f_close(&file);
}

void producer(void *p) {
  acceleration__axis_data_s average = {0, 0, 0};
  acceleration__axis_data_s sensor_reading;
  int i = 0;

  while (1) {
    while (i < 100) {
      const acceleration__axis_data_s sensor_data = acceleration__get_data();
      average.x += sensor_data.x;
      average.y += sensor_data.y;
      average.z += sensor_data.z;
      i++;
    }
    sensor_reading.x = (int)((double)average.x / 100.0);
    average.x = 0;
    sensor_reading.y = (int)((double)average.y / 100.0);
    average.y = 0;
    sensor_reading.z = (int)((double)average.z / 100.0);
    average.z = 0;
    i = 0;

    if (!xQueueSend(switch_queue, &sensor_reading, 500)) {
      printf("Queue send fail\n");
    } else {
    }
    xEventGroupSetBits(watchdog_eventgrp, BIT0);
    vTaskDelay(100);
  }
}

void consumer(void *p) {
  acceleration__axis_data_s sensor_data[10], get_data;
  while (1) {
    if (xQueueReceive(switch_queue, &get_data, portMAX_DELAY)) {
      // printf("Sensor value: | x-axis=%i | y-axis=%i | z-axis=%i |\n", get_data.x, get_data.y, get_data.z);
      for (int i = 0; i < 10; i++) {
        sensor_data[i] = get_data;
      }
      write_file_using_fatfs_pi((acceleration__axis_data_s *)&sensor_data);
    } else {
      printf("Queue receive failed\n");
    }
    xEventGroupSetBits(watchdog_eventgrp, BIT1);
  }
}

void watchdog(void *p) {
  while (1) {
    vTaskDelay(200);
    EventBits_t CheckBits = xEventGroupWaitBits(watchdog_eventgrp, (BIT0 | BIT1), 1, 1, 1000);
    if ((CheckBits & (BIT0 | BIT1)) == (BIT0 | BIT1)) {
      printf("Watchdog task is able to verify the check-in of producer and consumer tasks\n");
    } else if ((CheckBits & BIT0) != BIT0) {
      printf("Producer task failed check-in\n");
    } else if ((CheckBits & BIT1) != BIT1) {
      printf("Consumer task failed check-in\n");
    } else if ((CheckBits & (BIT0 | BIT1)) != (BIT0 | BIT1)) {
      printf("Producer and Consumer task failed check-in. Timeout occured\n");
    } else {
      printf("Timeout occured\n");
    }
  }
}

int main(void) {
  create_blinky_tasks();
  create_uart_task();

  acceleration__init();

  switch_queue = xQueueCreate(10, sizeof(acceleration__axis_data_s));
  watchdog_eventgrp = xEventGroupCreate();

  xTaskCreate(producer, "producer", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(consumer, "consumer", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(watchdog, "watchdog", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_HIGH, NULL);

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
