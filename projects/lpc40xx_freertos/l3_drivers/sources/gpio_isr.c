// @file gpio_isr.c
#include "gpio_isr.h"

#include "lpc40xx.h"
#include <stdio.h>

void port0pin30_isr(void);
void port0pin29_isr(void);
// Note: You may want another separate array for falling vs. rising edge callbacks
static function_pointer_t gpio_callbacks[32] = {port0pin29_isr, port0pin30_isr};

void gpio__attach_interrupt(gpio_port_e port, uint32_t pin, gpio_interrupt_e interrupt_type,
                            function_pointer_t callback) {
  gpio_callbacks[pin] = callback;
  // 1) Store the callback based on the pin at gpio0_callbacks
  // 2) Configure GPIO 0 pin for rising or falling edge
  // fprintf(stderr, "Attach interrupt\n");
  gpio_s port_pin = gpio__construct(port, pin);
  gpio__set_as_input(port_pin);
  if (GPIO_INTR__RISING_EDGE == interrupt_type) {
    // fprintf(stderr, "Rising edge");
    if (PORT0 == port) {
      LPC_GPIOINT->IO0IntEnR |= (1 << pin);
    } else if (PORT2 == port) {
      LPC_GPIOINT->IO2IntEnR |= (1 << pin);
    }

  } else if (GPIO_INTR__FALLING_EDGE == interrupt_type) {
    // fprintf(stderr, "Falling edge");
    if (PORT0 == port) {
      LPC_GPIOINT->IO0IntEnF |= (1 << pin);
    } else if (PORT2 == port) {
      LPC_GPIOINT->IO2IntEnF |= (1 << pin);
    }
  } else {
  }
}

// We wrote some of the implementation for you
void gpio__interrupt_dispatcher(void) {
  u_int8_t port;
  // Check which pin generated the interrupt
  const int pin_that_generated_interrupt = logic_that_you_will_write(&port);
  // fprintf(stderr, "Interrupt pin %d\n", pin_that_generated_interrupt);

  clear_pin_interrupt(port, pin_that_generated_interrupt);
  function_pointer_t attached_user_handler = gpio_callbacks[pin_that_generated_interrupt];
  // Invoke the user registered callback, and then clear the interrupt
  attached_user_handler();
}

void clear_pin_interrupt(u_int8_t port, int pin_that_generated_interrupt) {
  // fprintf(stderr, "Interrupt is cleared pin %d\n", pin_that_generated_interrupt);
  if (PORT0 == port) {
    LPC_GPIOINT->IO0IntClr |= (1 << pin_that_generated_interrupt);
  } else if (PORT2 == port) {
    LPC_GPIOINT->IO2IntClr |= (1 << pin_that_generated_interrupt);
  } else {
  }
}

int logic_that_you_will_write(u_int8_t *port) {
  int interrupt_pin = 0;
  u_int8_t pin_num = 1;
  uint32_t pin;
  for (pin_num = 1; pin_num < 33; pin_num++) {
    pin = (1 << pin_num);
    if ((LPC_GPIOINT->IO0IntStatR & pin) || (LPC_GPIOINT->IO0IntStatF & pin)) {
      *port = PORT0;
      interrupt_pin = pin_num;
      // fprintf(stderr, "Port0Pin_num = %d\n", pin_num);
      pin_num = 33;
    } else if ((LPC_GPIOINT->IO2IntStatR & pin) || (LPC_GPIOINT->IO2IntStatF & pin)) {
      *port = PORT2;
      interrupt_pin = pin_num;
      pin_num = 33;
    } else {
    }
  }
  return interrupt_pin;
}