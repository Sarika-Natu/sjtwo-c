git #include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

static void task_one(void *task_parameter);
static void task_two(void *task_parameter);

int main(void) {

    puts("Starting RTOS");
    xTaskCreate(task_one, "task1", configMINIMAL_STACK_SIZE, NULL, PRIORITY_HIGH, NULL);
    xTaskCreate(task_two, "task2", configMINIMAL_STACK_SIZE, NULL, PRIORITY_LOW, NULL);
    vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

    return 0;
}

static void task_one(void *task_parameter) {
    while (true) {
        // Read existing main.c regarding when we should use fprintf(stderr...) in place of printf()
        // For this lab, we will use fprintf(stderr, ...)
        fprintf(stderr, "AAAAAAAAAAAA");

        // Sleep for 100ms
        vTaskDelay(100);
    }
}

static void task_two(void *task_parameter) {
    while (true) {
        fprintf(stderr, "bbbbbbbbbbbb");
        vTaskDelay(100);
    }
}