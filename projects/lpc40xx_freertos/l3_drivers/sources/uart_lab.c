
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "lpc40xx.h"
#include "task.h"
#include "uart_lab.h"
#include <stdio.h>

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  // Refer to LPC User manual and setup the register bits correctly
  // The first page of the UART chapter has good instructions
  const uint32_t enable_uart2 = (1 << 24);
  const uint32_t enable_uart3 = (1 << 25);

  const uint32_t func_mask = 0x07;
  const uint32_t enable_uart2_func = 0x01;
  const uint32_t enable_uart3_func = 0x02;

  const uint32_t dlab_bit = (1 << 7);
  const uint32_t word_len_sel_mask = (0b11 << 0);
  const uint32_t word_len_sel_8bit = (0b11 << 0);
  const uint8_t one_byte = 8;
  const uint8_t byte_mask = 0xFF;
  uint16_t dlm_dll = 0;

  if (UART_2 == uart) {
    // a) Power on Peripheral
    LPC_SC->PCONP |= enable_uart2;

    // b) Setup DLL, DLM, FDR, LCR registers
    LPC_UART2->LCR |= dlab_bit;

    // baud_rate = Pclk/16/(DLM_DLL)
    dlm_dll = (peripheral_clock / baud_rate / 16);
    LPC_UART2->DLL = (byte_mask & dlm_dll);
    LPC_UART2->DLM = (byte_mask & (dlm_dll >> one_byte));

    LPC_UART2->LCR &= ~dlab_bit;

    LPC_UART2->LCR &= ~word_len_sel_mask;
    LPC_UART2->LCR |= word_len_sel_8bit;

  } else if (UART_3 == uart) {
    LPC_SC->PCONP |= enable_uart3;
    LPC_UART3->LCR |= dlab_bit;

    dlm_dll = (peripheral_clock / baud_rate / 16);
    LPC_UART3->DLL = (byte_mask & dlm_dll);
    LPC_UART3->DLM = (byte_mask & (dlm_dll >> one_byte));

    LPC_UART3->LCR &= ~dlab_bit;

    LPC_UART3->LCR &= ~word_len_sel_mask;
    LPC_UART3->LCR |= word_len_sel_8bit;

    LPC_IOCON->P4_28 &= func_mask;
    LPC_IOCON->P4_28 |= enable_uart3_func;
    LPC_IOCON->P4_29 &= func_mask;
    LPC_IOCON->P4_29 |= enable_uart3_func;
  } else {
  }
}

// Read the byte from RBR and actually save it to the pointer
bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  bool ret_val = false;
  const uint32_t receive_data_ready = (1 << 0);

  if (UART_2 == uart) {
    // a) Check LSR for Receive Data Ready
    while (!(LPC_UART2->LSR & receive_data_ready)) {
      ;
    }
    // b) Copy data from RBR register to input_byte
    *input_byte = LPC_UART2->RBR;
    ret_val = true;
  } else if (UART_3 == uart) {
    while (!(LPC_UART3->LSR & receive_data_ready)) {
      ;
    }
    *input_byte = LPC_UART3->RBR;
    ret_val = true;

  } else {
  }
  return ret_val;
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  bool ret_val = false;
  const uint32_t transmit_holding_register_empty = (1 << 5);
  if (UART_2 == uart) {
    // a) Check LSR for Transmit Hold Register Empty
    while (!(LPC_UART2->LSR & transmit_holding_register_empty)) {
    }
    // b) Copy output_byte to THR register
    LPC_UART2->THR = output_byte;
    ret_val = true;

  } else if (UART_3 == uart) {
    while (!(LPC_UART3->LSR & transmit_holding_register_empty)) {
      ;
    }
    LPC_UART3->THR = output_byte;
    while (!(LPC_UART3->LSR & transmit_holding_register_empty)) {
      ;
    }
    ret_val = true;
    // fprintf(stderr, "Character transmitted\n");
  } else {
  }
  return ret_val;
}