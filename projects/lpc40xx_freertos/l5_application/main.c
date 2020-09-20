#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "gpio_isr.h"
#include "lpc_peripherals.h"
#include "semphr.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

void pin30_isr(void);
void pin29_isr(void);
void sleep_on_sem_task(void *p);

static SemaphoreHandle_t switch_pressed_signal;

void port0pin30_isr(void) {
  // fprintf(stderr, "Interrupt is hit 30\n");
  xSemaphoreGiveFromISR(switch_pressed_signal, NULL);
}
void port0pin29_isr(void) {
  // fprintf(stderr, "Interrupt is hit 29\n");
  xSemaphoreGiveFromISR(switch_pressed_signal, NULL);
}

/****************** PART1:Interrupt with Binary Semaphore ****************************
void gpio_interrupt(void);
static void clear_gpio_interrupt(void);
static void configure_your_gpio_interrupt(void);
static void sleep_on_sem_task(void *p);

static SemaphoreHandle_t switch_pressed_signal;

void gpio_interrupt(void) {
  clear_gpio_interrupt();
  xSemaphoreGiveFromISR(switch_pressed_signal, NULL);

  // fprintf(stderr, "Interrupt is hit\n");
  ****************** PART0:SimpleInterrupt *******************************************
  gpio_s port1_pin8 = gpio__construct(GPIO__PORT_1, 8);
  gpio__set_as_output(port1_pin8);
  // a) Clear Port0/2 interrupt using CLR0 or CLR2 registers
  LPC_GPIOINT->IO0IntClr |= (1 << 29);
  // b) Use fprintf(stderr) or blink and LED here to test your ISR
  // fprintf(stderr, "Interrupt is hit");
  gpio__reset(port1_pin8);
  ****************** PART0:SimpleInterrupt *******************************************
}
static void clear_gpio_interrupt(void) { LPC_GPIOINT->IO0IntClr |= (1 << 29); }
static void configure_your_gpio_interrupt(void) {
  gpio_s port0_pin29 = gpio__construct(GPIO__PORT_0, 29);
  gpio__set_as_input(port0_pin29);
  LPC_GPIOINT->IO0IntEnR |= (1 << 29);
}
****************** PART1:Interrupt with Binary Semaphore ****************************/
void sleep_on_sem_task(void *p) {
  gpio_s port1_pin8 = gpio__construct(GPIO__PORT_1, 8);
  gpio__set_as_output(port1_pin8);
  // fprintf(stderr, "sleep_on_sem_task\n");
  while (true) {
    // Note: There is no vTaskDelay() here, but we use sleep mechanism while waiting for the binary semaphore (signal)
    if (xSemaphoreTake(switch_pressed_signal, portMAX_DELAY)) {
      // fprintf(stderr, "Semaphore taken\n");
      while (true) {
        gpio__set(port1_pin8);
        // fprintf(stderr, "ON\n");
        vTaskDelay(500);

        gpio__reset(port1_pin8);
        // fprintf(stderr, "OFF\n");
        vTaskDelay(500);
      }
    } else {
    }
  }
}

int main(void) {
  create_blinky_tasks();
  create_uart_task();

  switch_pressed_signal = xSemaphoreCreateBinary(); // Create your binary semaphore
  gpio__attach_interrupt(PORT0, 29, GPIO_INTR__RISING_EDGE, port0pin29_isr);
  gpio__attach_interrupt(PORT0, 30, GPIO_INTR__FALLING_EDGE, port0pin30_isr);

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio__interrupt_dispatcher, "gpio_intr");

  /****************** PART1:Interrupt with Binary Semaphore ****************************
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_interrupt, "gpio_intr");
   configure_your_gpio_interrupt(); // TODO: Setup interrupt by re-using code from Part 0
   ****************** PART1:Interrupt with Binary Semaphore ****************************/
  NVIC_EnableIRQ(GPIO_IRQn); // Enable interrupt gate for the GPIO

  xTaskCreate(sleep_on_sem_task, "sem", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  /****************** PART0:SimpleInterrupt *******************************************
  gpio_s port0_pin29 = gpio__construct(GPIO__PORT_0, 29);
  gpio_s port1_pin8 = gpio__construct(GPIO__PORT_1, 8);
  gpio__set_as_output(port1_pin8);
  // Read Table 95 in the LPC user manual and setup an interrupt on a switch connected to Port0 or Port2
  // a) For example, choose SW2 (P0_30) pin on SJ2 board and configure as input
  gpio__set_as_input(port0_pin29);
  //.   Warning: P0.30, and P0.31 require pull-down resistors
  // b) Configure the registers to trigger Port0 interrupt (such as falling edge)
  LPC_GPIOINT->IO0IntEnR |= (1 << 29);
  // Install GPIO interrupt function at the CPU interrupt (exception) vector
  // c) Hijack the interrupt vector at interrupt_vector_table.c and have it call our gpio_interrupt()
  //    Hint: You can declare 'void gpio_interrupt(void)' at interrupt_vector_table.c such that it can see this
  function

  // Most important step: Enable the GPIO interrupt exception using the ARM Cortex M API (this is from lpc40xx.h)
  NVIC_EnableIRQ(GPIO_IRQn);

  // Toggle an LED in a loop to ensure/test that the interrupt is entering ane exiting
  // For example, if the GPIO interrupt gets stuck, this LED will stop blinking
  while (1) {
    delay__ms(100);
    // TODO: Toggle an LED here
    gpio__set(port1_pin8);
  }
  ****************** PART0:SimpleInterrupt End *******************************************/

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
