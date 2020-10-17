#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "semphr.h"
#include "ssp2_lab.h"

// TODO: Study the Adesto flash 'Manufacturer and Device ID' section
typedef struct {
    uint8_t manufacturer_id;
    uint8_t device_id_1;
    uint8_t device_id_2;
    uint8_t extended_device_id;
} adesto_flash_id_s;

typedef struct {
    bool BPL;
    bool EPE;
    bool WPP;
    bool BP0;
    bool WEL;
    bool RDYBSY;
    bool RSTE;

} adesto_statusreg_s;

static SemaphoreHandle_t spi_bus_mutex;

// TODO: Implement Adesto flash memory CS signal as a GPIO driver
void adesto_cs(void);
void adesto_ds(void);
static void set_ssp_iocon(void);
static adesto_flash_id_s ssp2__adesto_read_signature(void);
void spi_task(void *p);
void spi_id_verification_task1(void *p);
void spi_id_verification_task2(void *p);
static adesto_statusreg_s ssp2__adesto_read_status_register(void);

void adesto_cs(void) {
    const uint32_t cs_pin = (1 << 10);
    const uint32_t cs_pin_dummy = (1 << 28);
    LPC_GPIO1->CLR = cs_pin;
    LPC_GPIO4->CLR = cs_pin_dummy;
}

void adesto_ds(void) {
    const uint32_t cs_pin = (1 << 10);
    const uint32_t cs_pin_dummy = (1 << 28);
    LPC_GPIO1->SET = cs_pin;
    LPC_GPIO4->SET = cs_pin_dummy;
}

// TODO: Implement the code to read Adesto flash memory signature
// TODO: Create struct of type 'adesto_flash_id_s' and return it
static adesto_flash_id_s ssp2__adesto_read_signature(void) {
    adesto_flash_id_s data = {0};
    const uint8_t opcode = 0x9F;
    const uint8_t dummy_send = 0xFF;
    adesto_cs();
    {
        // Send opcode and read bytes
        ssp2__exchg_byte(opcode);
        // TODO: Populate members of the 'adesto_flash_id_s' struct
        data.manufacturer_id = ssp2__exchg_byte(dummy_send);
        data.device_id_1 = ssp2__exchg_byte(dummy_send);
        data.device_id_2 = ssp2__exchg_byte(dummy_send);
        data.extended_device_id = ssp2__exchg_byte(dummy_send);
    }
    adesto_ds();

    return data;
}

static adesto_statusreg_s ssp2__adesto_read_status_register(void) {
    adesto_statusreg_s data = {0};
    const uint8_t opcode = 0x05;
    const uint8_t dummy_send = 0xFF;
    uint8_t dummy_read1 = 0x00;
    uint8_t dummy_read2 = 0x00;
    adesto_cs();
    {
        // Send opcode and read bytes
        ssp2__exchg_byte(opcode);
        // TODO: Populate members of the 'adesto_flash_id_s' struct
        dummy_read1 = ssp2__exchg_byte(dummy_send);
        dummy_read2 = ssp2__exchg_byte(dummy_send);

        data.BPL = (0x01 & (dummy_read1 >> 7));
        data.EPE = (0x01 & (dummy_read1 >> 5));
        data.WPP = (0x01 & (dummy_read1 >> 4));
        data.BP0 = (0x01 & (dummy_read1 >> 2));
        data.WEL = (0x01 & (dummy_read1 >> 1));
        data.RDYBSY = (0x01 & (dummy_read1 >> 0)); // RDY/BSY status can be read from Status Register 1 or 2
        data.RSTE = (0x01 & (dummy_read2 >> 4));
    }
    adesto_ds();

    return data;
}

/**
 * Adesto flash asks to send 24-bit address
 * We can use our usual uint32_t to store the address
 * and then transmit this address over the SPI driver
 * one byte at a time
 */
uint16_t adesto_flash_send_address(uint32_t address) {
    const uint8_t opcode = 0x0B;
    const uint8_t dummy_send = 0xAA;
    uint8_t readvalue = 0x00;
    uint16_t register_value = 0x0000;
    adesto_cs();
    {
        ssp2__exchg_byte(opcode);
        ssp2__exchg_byte((address >> 16) & 0xFF);
        ssp2__exchg_byte((address >> 8) & 0xFF);
        ssp2__exchg_byte((address >> 0) & 0xFF);
        ssp2__exchg_byte(dummy_send); // Additional dummy byte for opcode 0x0B Read Array
        readvalue = ssp2__exchg_byte(dummy_send);
        register_value = (readvalue << 8);
        readvalue = ssp2__exchg_byte(dummy_send);
        register_value |= (readvalue << 0);
    }
    adesto_ds();
    return register_value;
}

static void set_ssp_iocon(void) {
    const uint32_t func_mask = 0x07;
    const uint32_t enable_spi_func = 0x04;
    const uint32_t cs_pin = (1 << 10);
    const uint32_t cs_pin_dummy = (1 << 28);

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
    LPC_GPIO4->DIR |= cs_pin_dummy;
}

void spi_id_verification_task1(void *p) {
    while (1) {
        if (xSemaphoreTake(spi_bus_mutex, 1000)) {
            // Use Guarded Resource
            const adesto_flash_id_s id = ssp2__adesto_read_signature();

            // When we read a manufacturer ID we do not expect, we will kill this task
            if (id.manufacturer_id != 0x1F) {
                fprintf(stderr, "Manufacturer ID read failure\n");
                vTaskSuspend(NULL); // Kill this task
            }
            // Give Semaphore back:
            xSemaphoreGive(spi_bus_mutex);
        }
    }
}

void spi_id_verification_task2(void *p) {
    while (1) {
        if (xSemaphoreTake(spi_bus_mutex, 1000)) {
            // Use Guarded Resource
            const adesto_flash_id_s id = ssp2__adesto_read_signature();

            // When we read a manufacturer ID we do not expect, we will kill this task
            if (id.manufacturer_id != 0x1F) {
                fprintf(stderr, "Manufacturer ID read failure\n");
                vTaskSuspend(NULL); // Kill this task
            }
            // Give Semaphore back:
            xSemaphoreGive(spi_bus_mutex);
        }
    }
}

void spi_task(void *p) {
#ifdef PART1
    const uint32_t spi_clock_mhz = 24;
  ssp2__init(spi_clock_mhz);

  // From the LPC schematics pdf, find the pin numbers connected to flash memory
  // Read table 84 from LPC User Manual and configure PIN functions for SPI2 pins
  // You can use gpio__construct_with_function() API from gpio.h
  //
  // Note: Configure only SCK2, MOSI2, MISO2.
  // CS will be a GPIO output pin(configure and setup direction)
  set_ssp_iocon();
#endif
    while (1) {
        adesto_flash_id_s id = ssp2__adesto_read_signature();
        // TODO: printf the members of the 'adesto_flash_id_s' struct
        fprintf(stderr, "\n\nMANUFACTURER ID = %x\n", id.manufacturer_id);
        fprintf(stderr, "DEVICE ID BYTE 1 = %x\n", id.device_id_1);
        fprintf(stderr, "DEVICE ID BYTE 2 = %x\n", id.device_id_2);
        fprintf(stderr, "EXTENDED INFORMATION STRING LENGTH = %x\n", id.extended_device_id);

        vTaskDelay(500);
    }
}

void spi_statustask(void *p) {

    while (1) {
        adesto_statusreg_s id = ssp2__adesto_read_status_register();
        // TODO: printf the members of the 'adesto_statusreg_s' struct
        fprintf(stderr, "\n\nStatus of Block Protection Locked bit is = %d\n", id.BPL);
        fprintf(stderr, "Status of Erase/Program Error bit is = %d\n", id.EPE);
        fprintf(stderr, "Status of Write Protect bit is = %d\n", id.WPP);
        fprintf(stderr, "Status of Block Protection bit is = %d\n", id.BP0);
        fprintf(stderr, "Status of Write Enable Latch bit is = %d\n", id.WEL);
        fprintf(stderr, "Status of Ready/Busy bit is = %d\n", id.RDYBSY);
        fprintf(stderr, "Status of Reset Enabled bit is = %d\n\n", id.RSTE);
        vTaskDelay(500);
    }
}

void spi_readtask(void *p) {
    uint16_t regiter_value = 0x0000;
    while (1) {
        regiter_value = adesto_flash_send_address(0x007FFE);
        // TODO: printf the members of the 'adesto_flash_id_s' struct
        fprintf(stderr, "Read value of Register is = %x\n", regiter_value);

        vTaskDelay(500);
    }
}

int main(void) {
    const uint32_t spi_clock_mhz = 24;
    ssp2__init(spi_clock_mhz);
    set_ssp_iocon();

    spi_bus_mutex = xSemaphoreCreateMutex();
    puts("Starting RTOS");
#ifdef PART1
    xTaskCreate(spi_task, "spi_task", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
#endif

#ifdef PART2
    // Create two tasks that will continously read signature
  xTaskCreate(spi_id_verification_task1, "spi_ver1", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
  xTaskCreate(spi_id_verification_task2, "spi_ver2", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
#endif

    xTaskCreate(spi_statustask, "spi_statustask", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
    xTaskCreate(spi_readtask, "spi_readtask", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
    vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

    return 0;
}
