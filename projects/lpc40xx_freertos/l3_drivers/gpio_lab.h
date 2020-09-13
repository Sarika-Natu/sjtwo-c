// file gpio_lab.h
#pragma once

#include <stdbool.h>
#include <stdint.h>
/** DECLARED THESE FUNCTIONS AS STATIC IN gpio_lab.h *********
 *
/// Should alter the hardware registers to set the pin as input
void gpio__set_as_input(uint8_t port_num, uint8_t pin_num);

/// Should alter the hardware registers to set the pin as output
void gpio__set_as_output(uint8_t port_num, uint8_t pin_num);

/// Should alter the hardware registers to set the pin as high
void gpio__set_high(uint8_t port_num, uint8_t pin_num);

/// Should alter the hardware registers to set the pin as low
void gpio__set_low(uint8_t port_num, uint8_t pin_num);

//
 * Should alter the hardware registers to set the pin as low
 *
 * @param {bool} high - true => set pin high, false => set pin low
 *
void gpio__set(uint8_t port_num, uint8_t pin_num, bool high);

//
 * Should return the state of the pin (input or output, doesn't matter)
 *
 * @return {bool} level of pin high => true, low => false
 *
bool gpio__get_level(uint8_t port, uint8_t pin_num);**/

bool vReadSw(uint8_t port, uint8_t pin_num);

void vCtrlLed(uint8_t port, uint8_t pin_num, bool high);

void vLedOn(uint8_t port, uint8_t pin_num);

void vLedOff(uint8_t port, uint8_t pin_num);

void vSetasOutput(uint8_t port, uint8_t pin_num);

void vSetasInput(uint8_t port, uint8_t pin_num);