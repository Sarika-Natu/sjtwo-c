#include "ssp2_lab.h"
#include "lpc40xx.h"
#include <stdint.h>

#include <stdio.h>

static void set_ssp_iocon(void);
void ssp2__init(uint32_t max_clock_mhz);
uint8_t ssp2__exchg_byte(uint8_t data_out);

static void set_ssp_iocon(void) {
  const uint32_t func_mask = 0x07;
  const uint32_t enable_spi_func = 0x04;
  const uint32_t cs_pin = (1 << 10);

  // SSP2_SCK pin
  LPC_IOCON->P1_0 &= func_mask;
  LPC_IOCON->P1_0 |= enable_spi_func;
  // SSP2_MOSI pin
  LPC_IOCON->P1_1 &= func_mask;
  LPC_IOCON->P1_1 |= enable_spi_func;
  // SSP2_MISO pin
  LPC_IOCON->P1_4 &= func_mask;
  LPC_IOCON->P1_4 |= enable_spi_func;
  // SSP2_CS GPIO pin as output
  LPC_GPIO1->DIR |= cs_pin;
}

void ssp2__init(uint32_t max_clock_mhz) {
  const uint32_t enable_ssp2 = (1 << 20);

  const uint32_t data_size_8 = (0b111 << 0);
  const uint32_t ssp_enable = 0x02;
  const uint32_t prescalar_value = 0x04;
  const uint32_t prescalar_mask = 0xFF;
  const uint32_t frame_format = (0b00 << 4);

  // Refer to LPC User manual and setup the register bits correctly
  // a) Power on Peripheral - SSP2
  LPC_SC->PCONP |= enable_ssp2;

  set_ssp_iocon();

  // b) Setup control registers CR0 and CR1
  LPC_SSP2->CR0 |= (data_size_8 | frame_format);
  LPC_SSP2->CR1 |= ssp_enable;

  // c) Setup prescalar register to be <= max_clock_mhz
  // LPC_SSP2->CPSR &= prescalar_mask;
  // LPC_SSP2->CPSR |= prescalar_value;
  LPC_SSP2->CPSR = max_clock_mhz;
}

uint8_t ssp2__exchg_byte(uint8_t data_out) {
  const uint32_t ssp2_busy = (1 << 4);
  LPC_SSP2->DR = data_out;
  // Configure the Data register(DR) to send and receive data by checking the SPI peripheral status register
  while (LPC_SSP2->SR & ssp2_busy) {
    ;
  }
  const uint8_t data_in = LPC_SSP2->DR;
  return data_in;
}