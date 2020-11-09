#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "mp3_decoder.h"

void read_song(void *p) {}

void send_data_to_decoder(void *p) {}

int main(void) {

  mp3_init();

  xTaskCreate(read_song, "read_song", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
  xTaskCreate(send_data_to_decoder, "spi_send", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}