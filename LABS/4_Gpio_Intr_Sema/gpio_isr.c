// @file gpio_isr.c
#include "gpio_isr.h"

// Note: You may want another separate array for falling vs. rising edge callbacks
static function_pointer_t gpio_callbacks[2] = {sem_isr, user_isr};
static void clear_pin_interrupt(gpio__port_e port, int interrupt_pin);
static int read_pin_interrupt(gpio__port_e *port);
void gpio__attach_interrupt(gpio__port_e port, uint32_t pin, gpio_interrupt_e interrupt_type,
                            function_pointer_t callback);
void gpio__interrupt_dispatcher(void);

/*************************************************************************************
 * This function clears the interrupt flag for the requested port-pin
 */
static void clear_pin_interrupt(gpio__port_e port, int interrupt_pin) {
    fprintf(stderr, "Interrupt is cleared pin %d\n", interrupt_pin);
    if (GPIO__PORT_0 == port) {
        LPC_GPIOINT->IO0IntClr |= (1 << interrupt_pin);
    } else if (GPIO__PORT_2 == port) {
        LPC_GPIOINT->IO2IntClr |= (1 << interrupt_pin);
    } else {
    }
}

/*************************************************************************************
 * This function identifies the interrupt flag and return the port and pin number
 */
static int read_pin_interrupt(gpio__port_e *port) {
    int interrupt_pin = 0;
    uint8_t pin_num = 1;
    uint32_t pin_comp;
    for (pin_num = 1; pin_num < 33; pin_num++) {
        pin_comp = (1 << pin_num);
        if ((LPC_GPIOINT->IO0IntStatR & pin_comp) || (LPC_GPIOINT->IO0IntStatF & pin_comp)) {
            *port = GPIO__PORT_0;
            interrupt_pin = pin_num;
            // fprintf(stderr, "Port0Pin_num = %d\n", pin_num);
            pin_num = 33;
        } else if ((LPC_GPIOINT->IO2IntStatR & pin_comp) || (LPC_GPIOINT->IO2IntStatF & pin_comp)) {
            *port = GPIO__PORT_2;
            interrupt_pin = pin_num;
            pin_num = 33;
        } else {
        }
    }
    return interrupt_pin;
}

/*************************************************************************************
 * This function enables the interrupt and assigns the callback function
 *
 */
void gpio__attach_interrupt(gpio__port_e port, uint32_t pin, gpio_interrupt_e interrupt_type,
                            function_pointer_t callback) {
    gpio_callbacks[pin] = callback;
    // 1) Store the callback based on the pin at gpio0_callbacks
    // 2) Configure GPIO 0 pin for rising or falling edge
    // fprintf(stderr, "Attach interrupt\n");
    gpio_s port_pin = gpio__construct(port, pin);
    gpio__set_as_input(port_pin);
    if (GPIO_INTR__RISING_EDGE == interrupt_type) {
        // fprintf(stderr, "Rising edge");
        if (GPIO__PORT_0 == port) {
            LPC_GPIOINT->IO0IntEnR |= (1 << pin);
        } else if (GPIO__PORT_2 == port) {
            LPC_GPIOINT->IO2IntEnR |= (1 << pin);
        }

    } else if (GPIO_INTR__FALLING_EDGE == interrupt_type) {
        // fprintf(stderr, "Falling edge");
        if (GPIO__PORT_0 == port) {
            LPC_GPIOINT->IO0IntEnF |= (1 << pin);
        } else if (GPIO__PORT_2 == port) {
            LPC_GPIOINT->IO2IntEnF |= (1 << pin);
        }
    } else {
    }
}

/*************************************************************************************
 * This function identifies the interrupt and calls the callback function
 * after clearing the interrupt flag
 */
void gpio__interrupt_dispatcher(void) {
    gpio__port_e port;
    // Check which pin generated the interrupt
    const int interrupt_pin = read_pin_interrupt(&port);
    // fprintf(stderr, "Interrupt pin %d\n", interrupt_pin);

    clear_pin_interrupt(port, interrupt_pin);
    function_pointer_t attached_user_handler = gpio_callbacks[interrupt_pin];
    // Invoke the user registered callback, and then clear the interrupt
    attached_user_handler();
}