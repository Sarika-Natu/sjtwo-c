#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "uart_lab.h"

#define PART3

#if 0
#define UART_print(...) fprintf(stderr, __VA_ARGS__)
#else
#define UART_print(...) printf(__VA_ARGS__)
#endif

#ifdef PART3
// This task is done for you, but you should understand what this code is doing
void board_1_sender_task(void *p) {
    char number_as_string[16] = {0};

    while (true) {
        const int number = rand();
        sprintf(number_as_string, "%i", number);

        // Send one char at a time to the other board including terminating NULL char
        for (int i = 0; i <= strlen(number_as_string); i++) {
            uart_lab__polled_put(UART_2, number_as_string[i]);
            UART_print("Sent: %c\n", number_as_string[i]);
        }

        UART_print("Sent: %i over UART to the other board\n", number);
        vTaskDelay(3000);
    }
}

void board_2_receiver_task(void *p) {
    char number_as_string[16] = {0};
    int counter = 0;

    while (true) {
        char byte = 0;
        uart_lab__get_char_from_queue(&byte, portMAX_DELAY);
        UART_print("Received: %c\n", byte);

        // This is the last char, so print the number
        if ('\0' == byte) {
            number_as_string[counter] = '\0';
            counter = 0;
            UART_print("Received this number from the other board: %s\n", number_as_string);
        }
            // We have not yet received the NULL '\0' char, so buffer the data
        else {
            // TODO: Store data to number_as_string[] array one char at a time
            // Hint: Use counter as an index, and increment it as long as we do not reach max value of 16
            for (counter = 0; counter < 15; counter++) {
                number_as_string[counter] = byte;
            }
        }
    }
}
#endif

#ifdef PART2
void uart_read_intr(void *p) {
  while (1) {
    char input_byte = 'B';

    // TODO: Use uart_lab__polled_get() function and printf the received value
    bool result = uart_lab__get_char_from_queue(&input_byte, 100);
#ifdef PART1
    // TODO: Use uart_lab__polled_get() function and printf the received value
    bool result = uart_lab__polled_get(UART_2, &input_byte);
#endif
    if (result) {
      UART_print(stderr, "\nCharacter read on UART3 is %c\n", input_byte);
    } else {
      UART_print(stderr, "Character not received on UART3\n");
    }

    vTaskDelay(500);
  }
}
#endif

#ifdef PART1
void uart_read_task(void *p) {
  while (1) {
    char input_byte = 'B';

    // TODO: Use uart_lab__polled_get() function and printf the received value
    bool result = uart_lab__polled_get(UART_2, &input_byte);

    if (result) {
      UART_print(stderr, "\nCharacter read on UART3 is %c\n", input_byte);
    } else {
      UART_print(stderr, "Character not received on UART3\n");
    }

    vTaskDelay(500);
  }
}
#endif

#ifdef PART2
void uart_write_task(void *p) {
  while (1) {

    char output_byte = 'S';
    // TODO: Use uart_lab__polled_put() function and send a value

    bool result = uart_lab__polled_put(UART_2, output_byte);
    if (result) {
      // UART_print(stderr, "\nCharacter sent on UART3 is %c\n", output_byte);
    } else {
      UART_print(stderr, "Character not sent on UART3\n");
    }

    vTaskDelay(500);
  }
}
#endif

int main(void) {
    const uint32_t peripheral_clock = 96 * 1000 * 1000;
    const uint32_t baud_rate = 38400;

    // TODO : Use uart_lab__init() function and initialize UART2 or UART3(your choice)
    // TODO: Pin Configure IO pins to perform UART2/UART3 function

    UART_print(stderr, "Initialize UART\n");
    uart_lab__init(UART_2, peripheral_clock, baud_rate);
#ifdef PART3
    uart__enable_receive_interrupt(UART_2);
    NVIC_EnableIRQ(UART2_IRQn);
#endif

#ifdef PART3
    xTaskCreate(board_2_receiver_task, "receiver_task", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
    xTaskCreate(board_1_sender_task, "sender_task", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
    UART_print(stderr, "UART LAB Part3\n");
#endif

#ifdef PART2
    xTaskCreate(uart_read_intr, "uart_read_intr", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
  xTaskCreate(uart_write_task, "uart_write", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
  UART_print(stderr, "UART LAB Part2\n");
#endif
#ifdef PART1
    xTaskCreate(uart_read_task, "uart_read", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
  xTaskCreate(uart_write_task, "uart_write", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
  UART_print(stderr, "UART LAB Part1\n");
#endif
    puts("Starting RTOS");
    vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

    return 0;
}