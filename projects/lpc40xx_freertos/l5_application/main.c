#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "adc.h"
#include "lpc40xx.h"
#include "pwm1.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

typedef struct {
  gpio__port_e port;
  uint8_t pin;
} port_pin_s;

void pin_configure_pwm_channel_as_io_pin() {
  static port_pin_s pwm_ch = {GPIO__PORT_2, 1};
  gpio_s pwm_channel = gpio__construct(pwm_ch.port, pwm_ch.pin);
  gpio__set_as_output(pwm_channel);
  LPC_IOCON->P2_1 &= ~0b111;
  LPC_IOCON->P2_1 |= 0b001;
}
void pwm_task(void *p) {

  // Locate a GPIO pin that a PWM channel will control
  // NOTE You can use gpio__construct_with_function() API from gpio.h
  // TODO Write this function yourself

  pin_configure_pwm_channel_as_io_pin();
  pwm1__init_single_edge(5);
  // We only need to set PWM configuration once, and the HW will drive
  // the GPIO at 1000Hz, and control set its duty cycle to 50%
  pwm1__set_duty_cycle(PWM1__2_1, 50);

  // Continue to vary the duty cycle in the loop
  uint8_t percent = 0;
  while (1) {
    pwm1__set_duty_cycle(PWM1__2_1, percent);
    // fprintf(stderr, "%d\n", percent);
    if (++percent > 100) {
      percent = 0;
    }

    vTaskDelay(100);
  }
}

void pin_configure_adc_channel_as_io_pin(void) {
  static port_pin_s adc_ch = {GPIO__PORT_1, 31};
  gpio_s adc_channel = gpio__construct(adc_ch.port, adc_ch.pin);
  LPC_IOCON->P0_25 &= ~0b111;
  LPC_IOCON->P0_25 |= 0b001;      // setting FUNC bits of IOCON
  LPC_IOCON->P0_25 &= ~(1u << 7); // setting pin to analog mode
}
void adc_task(void *p) {
  adc__initialize();

  // TODO This is the function you need to add to adc.h
  // You can configure burst mode for just the channel you are using
  adc__enable_burst_mode(ADC__CHANNEL_2);

  // Configure a pin, such as P1.31 with FUNC 011 to route this pin as ADC channel 5
  // You can use gpio__construct_with_function() API from gpio.h
  pin_configure_adc_channel_as_io_pin(); // TODO You need to write this function

  while (1) {
    // Get the ADC reading using a new routine you created to read an ADC burst reading
    // TODO: You need to write the implementation of this function
    const uint16_t adc_value = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
    // const uint16_t adc_value = adc__get_adc_value(ADC__CHANNEL_5);
    fprintf(stderr, "\nIn main: %d", adc_value);
    vTaskDelay(100);
  }
}

int main(void) {
  // create_blinky_tasks();
  create_uart_task();
  static port_pin_s flashy_led[4] = {{GPIO__PORT_2, 3}, {GPIO__PORT_1, 26}, {GPIO__PORT_1, 24}, {GPIO__PORT_1, 18}};

  for (u_int8_t index = 0; index < 4; index++) {
    gpio_s port_pin = gpio__construct(flashy_led[index].port, flashy_led[index].pin);
    gpio__set_as_output(port_pin);
    gpio__set(port_pin);
  }
  xTaskCreate(pwm_task, "pwm_task", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
  xTaskCreate(adc_task, "adc_task", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
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
