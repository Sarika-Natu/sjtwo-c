#pragma once

#include <stdint.h>
#include <stdlib.h>

/// Initialize the bus with the given maximum clock rate in Khz
void ssp1__initialize(void);

/**
 * Exchange a single byte over the SPI bus
 * @returns the byte received while sending the byte_to_transmit
 */
uint8_t ssp1__exchange_byte(uint8_t byte_to_transmit);
