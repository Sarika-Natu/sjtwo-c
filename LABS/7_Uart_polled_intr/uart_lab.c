#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "queue.h"
#include "task.h"
#include "uart_lab.h"

// Private queue handle of our uart_lab.c
static QueueHandle_t your_uart_rx_queue;
static void power_on_peripheral(uart_number_e uart);
static void set_iocon_for_uart(uart_number_e uart);
static void your_receive_interrupt(void);

static void power_on_peripheral(uart_number_e uart) {
    const uint32_t enable_uart2 = (1 << 24);
    const uint32_t enable_uart3 = (1 << 25);

    switch (uart) {
        case UART_2:
            LPC_SC->PCONP |= enable_uart2;
            break;

        case UART_3:
            LPC_SC->PCONP |= enable_uart3;
            break;

        default:
            break;
    }
}

static void set_iocon_for_uart(uart_number_e uart) {

    const uint32_t func_mask = 0x07;
    const uint32_t enable_uart2_func = 0x02;
    const uint32_t enable_uart3_func = 0x02;

    switch (uart) {
        case UART_2:
            LPC_IOCON->P2_8 &= func_mask;
            LPC_IOCON->P2_8 |= enable_uart2_func;
            LPC_IOCON->P2_9 &= func_mask;
            LPC_IOCON->P2_9 |= enable_uart2_func;
            break;

        case UART_3:
            LPC_IOCON->P4_28 &= func_mask;
            LPC_IOCON->P4_28 |= enable_uart3_func;
            LPC_IOCON->P4_29 &= func_mask;
            LPC_IOCON->P4_29 |= enable_uart3_func;
            break;

        default:
            break;
    }
}

// Private function of our uart_lab.c
static void your_receive_interrupt(void) {
    uint32_t interrupt_identification = (0b010 << 1);
    const uint32_t receive_data_ready = (1 << 0);
#if 1
    // TODO: Read the IIR register to figure out why you got interrupted
    if (LPC_UART2->IIR == interrupt_identification) {

        // TODO: Based on IIR status, read the LSR register to confirm if there is data to be read
        while (!(LPC_UART2->LSR & receive_data_ready)) {
            ;
        }

        // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
        const char byte = LPC_UART2->RBR;
        xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
        uint32_t clear_iir = LPC_UART2->IIR;
        // fprintf(stderr, "Recieve interrupt\n");
    }
#endif
#if 0
    if (LPC_UART3->IIR == interrupt_identification) {

    // TODO: Based on IIR status, read the LSR register to confirm if there is data to be read
    while (!(LPC_UART3->LSR & receive_data_ready)) {
      ;
    }

    // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
    const char byte = LPC_UART3->RBR;
    xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
    uint32_t clear_iir = LPC_UART3->IIR;
    // fprintf(stderr, "Recieve interrupt\n");
  }
#endif
}

// Public function to enable UART interrupt
// TODO Declare this at the header file
void uart__enable_receive_interrupt(uart_number_e uart_number) {
    uint32_t rbr_intr_enable = (1 << 0);
    switch (uart_number) {
        case UART_2:

            // TODO: Use lpc_peripherals.h to attach your interrupt
            lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, your_receive_interrupt, "uart_intr");

            // TODO: Enable UART receive interrupt by reading the LPC User manual
            // Hint: Read about the IER register
            LPC_UART2->IER |= rbr_intr_enable;

            // TODO: Create your RX queue
            your_uart_rx_queue = xQueueCreate(10, sizeof(uint8_t));
            break;

        case UART_3:
            // TODO: Use lpc_peripherals.h to attach your interrupt
            lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, your_receive_interrupt, "uart_intr");

            // TODO: Enable UART receive interrupt by reading the LPC User manual
            // Hint: Read about the IER register
            LPC_UART3->IER |= rbr_intr_enable;

            // TODO: Create your RX queue
            your_uart_rx_queue = xQueueCreate(10, sizeof(uint8_t));
            // fprintf(stderr, "Character transmitted\n");
            break;

        default:
            break;
    }
}

// Public function to get a char from the queue (this function should work without modification)
// TODO: Declare this at the header file
bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
    return xQueueReceive(your_uart_rx_queue, input_byte, timeout);
}

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
    // Refer to LPC User manual and setup the register bits correctly
    // The first page of the UART chapter has good instructions

    const uint32_t dlab_bit = (1 << 7);
    const uint32_t word_len_sel_mask = (0b11 << 0);
    const uint32_t word_len_sel_8bit = (0b11 << 0);
    const uint8_t one_byte = 8;
    const uint8_t byte_mask = 0xFF;
    uint16_t dlm_dll = 0;

    power_on_peripheral(uart);
    switch (uart) {
        case UART_2:
            // b) Setup DLL, DLM, FDR, LCR registers
            LPC_UART2->LCR |= dlab_bit;

            // baud_rate = Pclk/16/(DLM_DLL)
            dlm_dll = (peripheral_clock / baud_rate / 16);
            LPC_UART2->DLL = (byte_mask & dlm_dll);
            LPC_UART2->DLM = (byte_mask & (dlm_dll >> one_byte));

            LPC_UART2->LCR &= ~dlab_bit;

            LPC_UART2->LCR &= ~word_len_sel_mask;
            LPC_UART2->LCR |= word_len_sel_8bit;
            break;

        case UART_3:
            LPC_UART3->LCR |= dlab_bit;

            dlm_dll = (peripheral_clock / baud_rate / 16);
            LPC_UART3->DLL = (byte_mask & dlm_dll);
            LPC_UART3->DLM = (byte_mask & (dlm_dll >> one_byte));

            LPC_UART3->LCR &= ~dlab_bit;

            LPC_UART3->LCR &= ~word_len_sel_mask;
            LPC_UART3->LCR |= word_len_sel_8bit;
            break;

        default:
            break;
    }
    set_iocon_for_uart(uart);
}

// Read the byte from RBR and actually save it to the pointer
bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
    bool ret_val = false;
    const uint32_t receive_data_ready = (1 << 0);

    switch (uart) {
        case UART_2:
            // a) Check LSR for Receive Data Ready
            while (!(LPC_UART2->LSR & receive_data_ready)) {
                ;
            }
            // b) Copy data from RBR register to input_byte
            *input_byte = LPC_UART2->RBR;
            ret_val = true;
            break;

        case UART_3:
            while (!(LPC_UART3->LSR & receive_data_ready)) {
                ;
            }
            *input_byte = LPC_UART3->RBR;
            ret_val = true;
            break;

        default:
            ret_val = false;
            break;
    }
    return ret_val;
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
    bool ret_val = false;
    const uint32_t transmit_holding_register_empty = (1 << 5);

    switch (uart) {
        case UART_2:
            // a) Check LSR for Transmit Hold Register Empty
            while (!(LPC_UART2->LSR & transmit_holding_register_empty)) {
            }
            // b) Copy output_byte to THR register
            LPC_UART2->THR = output_byte;
            ret_val = true;
            break;

        case UART_3:
            while (!(LPC_UART3->LSR & transmit_holding_register_empty)) {
                ;
            }
            LPC_UART3->THR = output_byte;
            while (!(LPC_UART3->LSR & transmit_holding_register_empty)) {
                ;
            }
            ret_val = true;
            // fprintf(stderr, "Character transmitted\n");
            break;

        default:
            ret_val = false;
            break;
    }
    return ret_val;
}