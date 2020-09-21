// @file gpio_isr.h
#pragma once

#include "gpio.h"
#include "lpc40xx.h"
//#include <stdio.h>
typedef enum {
  GPIO_INTR__FALLING_EDGE,
  GPIO_INTR__RISING_EDGE,
} gpio_interrupt_e;

void user_isr(void);
void sem_isr(void);

// Function pointer type (demonstrated later in the code sample)
typedef void (*function_pointer_t)(void);

// Allow the user to attach their callbacks
void gpio__attach_interrupt(gpio__port_e port, uint32_t pin, gpio_interrupt_e interrupt_type,
                            function_pointer_t callback);

// Our main() should configure interrupts to invoke this dispatcher where we will invoke user attached callbacks
// You can hijack 'interrupt_vector_table.c' or use API at lpc_peripherals.h
void gpio__interrupt_dispatcher(void);