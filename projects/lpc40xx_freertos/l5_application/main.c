#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "gpio_lab.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#define FLASHY_DELAY 200

typedef struct {
  /* First get gpio0 driver to work only, and if you finish it
   * you can do the extra credit to also make it work for other Ports
   */
  uint8_t port;
  uint8_t pin;
} port_pin_s;

static SemaphoreHandle_t switch_press_indication;
static SemaphoreHandle_t flashy_easter_egg;

// static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);
static void vInit(void);
static void vBlinkLED(void *pvParameters);
static void vControlLED(void *task_parameter);
static void vReadSwitch(void *task_parameter);
static void vControlLED_Flashy(void *task_parameter);
static void vReadSwitch_Flashy(void *task_parameter);

static void vInit(void) {
  // iocon is not set for other ports as GPIO(000) is default IOCON for all ports
  u_int32_t *port_iocon_P1_8 = (uint32_t *)(0x4002C0A0);
  *port_iocon_P1_8 = UINT32_C(0);

  vSetasOutput(PORT1, 8);
  vSetasOutput(PORT1, 18);
  vSetasOutput(PORT1, 24);
  vSetasOutput(PORT1, 26);
  vSetasOutput(PORT2, 3);
  vSetasInput(PORT0, 30);
  vSetasInput(PORT0, 29);
}
static void vBlinkLED(void *task_parameter) {
  // Type-cast the parameter that was passed from xTaskCreate()
  const port_pin_s *port_led = (port_pin_s *)(task_parameter);
  while (true) {

    vLedOn(port_led->port, port_led->pin);
    vTaskDelay(500);

    vLedOff(port_led->port, port_led->pin);
    vTaskDelay(500);
  }
}
static void vControlLED(void *task_parameter) {
  while (true) {
    const port_pin_s *port_led = (port_pin_s *)(task_parameter);
    // Note: There is no vTaskDelay() here, but we use sleep mechanism while waiting for the binary semaphore (signal)
    if (xSemaphoreTake(switch_press_indication, 1000)) {
      vCtrlLed(port_led->port, port_led->pin, true);
      vTaskDelay(100);
      vCtrlLed(port_led->port, port_led->pin, false);
      vTaskDelay(100);
    } else {
      // puts("Timeout: No switch press indication for 1000ms");
    }
  }
}
static void vReadSwitch(void *task_parameter) {
  port_pin_s *port_sw = (port_pin_s *)task_parameter;

  while (true) {
    // TODO: If switch pressed, set the binary semaphore
    if (vReadSw(port_sw[0].port, port_sw[0].pin)) {
      // puts("Switch pressed\n");
    } else {
      xSemaphoreGive(switch_press_indication);
      // puts("Switch not pressed\n");
    }
    // Task should always sleep otherwise they will use 100% CPU
    // This task sleep also helps avoid spurious semaphore give during switch debounce
    vTaskDelay(500);
  }
}

static void vReadSwitch_Flashy(void *task_parameter) {
  port_pin_s *port_sw = (port_pin_s *)task_parameter;
  while (true) {
    // TODO: If switch pressed, set the binary semaphore
    if (vReadSw(port_sw[1].port, port_sw[1].pin)) {
      xSemaphoreGive(flashy_easter_egg);
      // puts("Switch pressed - flashy\n");
    } else {
      // puts("Switch not pressed - flashy\n");
    }
    // Task should always sleep otherwise they will use 100% CPU
    // This task sleep also helps avoid spurious semaphore give during switch debounce
    vTaskDelay(500);
  }
}

static void vControlLED_Flashy(void *task_parameter) {
  while (true) {
    port_pin_s *port_leds = (port_pin_s *)task_parameter;
    // Note: There is no vTaskDelay() here, but we use sleep mechanism while waiting for the binary semaphore (signal)
    if (xSemaphoreTake(flashy_easter_egg, 1000)) {
      for (u_int8_t index = 0; index < 4; index++) {
        vCtrlLed(port_leds[index].port, port_leds[index].pin, true);
        vTaskDelay(FLASHY_DELAY);
        vCtrlLed(port_leds[index].port, port_leds[index].pin, false);
        vTaskDelay(FLASHY_DELAY);
        // puts("loop 1");
      }
      for (u_int8_t index = 4; index > 0; index--) {
        vCtrlLed(port_leds[index].port, port_leds[index].pin, true);
        vTaskDelay(FLASHY_DELAY);
        vCtrlLed(port_leds[index].port, port_leds[index].pin, false);
        vTaskDelay(FLASHY_DELAY);
        // puts("loop 2");
      }
    } else {
      // puts("Timeout: No switch press indication for 1000ms");
    }
  }
}

int main(void) {
  // create_blinky_tasks();
  create_uart_task();
  vInit();
  switch_press_indication = xSemaphoreCreateBinary();
  flashy_easter_egg = xSemaphoreCreateBinary();
  static port_pin_s port_switches[2] = {{PORT0, 30}, {PORT0, 29}};
  static port_pin_s port1_led8 = {PORT1, 8};
  static port_pin_s flashy_led[4] = {{PORT2, 3}, {PORT1, 26}, {PORT1, 24}, {PORT1, 18}};

  xTaskCreate(vBlinkLED, "vBlinkLED", (300U) / sizeof(void *), (void *)&port1_led8, PRIORITY_LOW, NULL);
  xTaskCreate(vControlLED, "vControlLED", (300U) / sizeof(void *), (void *)&port1_led8, PRIORITY_HIGH, NULL);
  xTaskCreate(vReadSwitch, "vReadSwitch", (300U) / sizeof(void *), (void *)&port_switches, PRIORITY_LOW, NULL);
  xTaskCreate(vControlLED_Flashy, "vControlLED_Flashy", (300U) / sizeof(void *), (void *)&flashy_led, PRIORITY_LOW,
              NULL);
  xTaskCreate(vReadSwitch_Flashy, "vReadSwitch_Flashy", (300U) / sizeof(void *), (void *)&port_switches, PRIORITY_LOW,
              NULL);

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
