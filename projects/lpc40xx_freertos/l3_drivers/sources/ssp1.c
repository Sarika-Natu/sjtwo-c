#include "ssp1.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

void ssp1__initialize(uint32_t max_clock_hz) {

  const uint32_t frame_format = (0b00 << 4);
  const uint32_t data_size_8 = (0b111 << 0);
  const uint32_t ssp_enable = 0x02;

  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP1);
  LPC_SSP1->CR0 |= (data_size_8 | frame_format);
  LPC_SSP1->CR1 |= ssp_enable;
  LPC_SSP1->CPSR = max_clock_hz;
}

uint8_t ssp1__exchange_byte(uint8_t data_out) {
  const uint32_t ssp1_busy = (1 << 4);
  LPC_SSP1->DR = data_out;
  while (LPC_SSP1->SR & ssp1_busy) {
    ;
  }
  const uint8_t data_in = LPC_SSP1->DR;
  return data_in;
}