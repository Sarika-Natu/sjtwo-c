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
#include "queue.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

// This is the queue handle we will need for the xQueue Send/Receive API
static QueueHandle_t adc_to_pwm_task_queue;

#define ADC_MAX_VALUE 4095.0
#define VREF 3.3
#define PWM_FREQ 1
#define DUTY_CYCLE_50 50
#define PERCENTILE 100

typedef struct {
  gpio__port_e port;
  uint8_t pin;
} port_pin_s;

void pin_configure_pwm_channel_as_io_pin() {
  const uint32_t func_mask = 0x07;
  const uint32_t enable_pwm_func = 0x01;
  LPC_IOCON->P2_0 &= ~func_mask;
  LPC_IOCON->P2_0 |= enable_pwm_func;

  LPC_IOCON->P2_1 &= ~func_mask;
  LPC_IOCON->P2_1 |= enable_pwm_func;

  LPC_IOCON->P2_2 &= ~func_mask;
  LPC_IOCON->P2_2 |= enable_pwm_func;

  LPC_IOCON->P2_4 &= ~func_mask;
  LPC_IOCON->P2_4 |= enable_pwm_func;
}
void pwm_task(void *p) {
  int adc_reading;
  uint32_t MR_Readch1, MR_Readch2, MR_Readch3, MR_Readch4;
  uint32_t MR_Read0;
  // Locate a GPIO pin that a PWM channel will control
  // NOTE You can use gpio__construct_with_function() API from gpio.h
  // TODO Write this function yourself

  pin_configure_pwm_channel_as_io_pin();
  pwm1__init_single_edge(PWM_FREQ);
  // We only need to set PWM configuration once, and the HW will drive
  // the GPIO at 1000Hz, and control set its duty cycle to 50%
  pwm1__set_duty_cycle(PWM1__2_0, DUTY_CYCLE_50);
  pwm1__set_duty_cycle(PWM1__2_1, DUTY_CYCLE_50);
  pwm1__set_duty_cycle(PWM1__2_2, DUTY_CYCLE_50);
  pwm1__set_duty_cycle(PWM1__2_4, DUTY_CYCLE_50);

  // Continue to vary the duty cycle in the loop

  while (1) {
    if (xQueueReceive(adc_to_pwm_task_queue, &adc_reading, 100)) {
      uint8_t adc_percentage = (u_int8_t)(((double)adc_reading / ADC_MAX_VALUE) * PERCENTILE);
      pwm1__set_duty_cycle(PWM1__2_0, adc_percentage);
      pwm1__set_duty_cycle(PWM1__2_1, adc_percentage);
      pwm1__set_duty_cycle(PWM1__2_2, adc_percentage);
      pwm1__set_duty_cycle(PWM1__2_4, adc_percentage);
      MR_Read0 = LPC_PWM1->MR0;
      fprintf(stderr, "The value at MR0 is %ld\n", MR_Read0);
      MR_Readch1 = LPC_PWM1->MR1;
      fprintf(stderr, "The value at MR1 is %ld\n", MR_Readch1);
      MR_Readch2 = LPC_PWM1->MR2;
      fprintf(stderr, "The value at MR2 is %ld\n", MR_Readch2);
      MR_Readch3 = LPC_PWM1->MR3;
      fprintf(stderr, "The value at MR3 is %ld\n", MR_Readch3);
      MR_Readch4 = LPC_PWM1->MR5;
      fprintf(stderr, "The value at MR5 is %ld\n\n", MR_Readch4);
    }
#ifdef PART0
    uint8_t percent = 0;
    pwm1__set_duty_cycle(PWM1__2_1, percent);
    // fprintf(stderr, "%d\n", percent);
    if (++percent > PERCENTILE) {
      percent = 0;
    }
    vTaskDelay(100);
#endif
  }
}

void pin_configure_adc_channel_as_io_pin(void) {
  const uint32_t func_mask = 0x07;
  const uint32_t enable_adc_func = 0x01;
  const uint32_t enable_analog_mode = (1u << 7);

  LPC_IOCON->P0_25 &= ~func_mask;
  LPC_IOCON->P0_25 |= enable_adc_func;     // setting FUNC bits of IOCON
  LPC_IOCON->P0_25 &= ~enable_analog_mode; // setting pin to analog mode
}
void adc_task(void *p) {
  int adc_reading;
  double adc_voltage = 0;
  adc__initialize();
  int64_t average = 0;
  int i = 0;

  // TODO This is the function you need to add to adc.h
  // You can configure burst mode for just the channel you are using
  adc__enable_burst_mode(ADC__CHANNEL_2);

  // Configure a pin, such as P1.31 with FUNC 011 to route this pin as ADC channel 5
  // You can use gpio__construct_with_function() API from gpio.h
  pin_configure_adc_channel_as_io_pin(); // TODO You need to write this function

  while (1) {
    // Get the ADC reading using a new routine you created to read an ADC burst reading
    // TODO: You need to write the implementation of this function
    while (i < 30) {
      const uint16_t adc_value = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
      // const uint16_t adc_value = adc__get_adc_value(ADC__CHANNEL_5);
      average += adc_value;
      i++;
    }

    adc_reading = (int)((double)average / 30.0);
    average = 0;
    i = 0;
    adc_voltage = ((double)adc_reading / ADC_MAX_VALUE) * VREF;
    fprintf(stderr, "The voltage read using the potentiometer is : %f volts\n", adc_voltage);
    // fprintf(stderr, "the ADC reading is: %d\n", adc_reading);

    xQueueSend(adc_to_pwm_task_queue, &adc_reading, 0);

    vTaskDelay(150);
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
  // Queue will only hold 1 integer
  adc_to_pwm_task_queue = xQueueCreate(1, sizeof(int));
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
