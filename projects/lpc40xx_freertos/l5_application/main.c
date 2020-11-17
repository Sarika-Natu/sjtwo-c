#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "mp3_decoder.h"
#include "queue.h"
#include "semphr.h"

#define READ_BYTES_FROM_FILE 1024

static const uint16_t MAX_BYTES_TX = 32U;
static const uint16_t COUNT = READ_BYTES_FROM_FILE / MAX_BYTES_TX;
static SemaphoreHandle_t mp3_mutex = NULL;
static QueueHandle_t mp3_queue = NULL;

void read_song(void *p);
void send_data_to_decoder(void *p);

int main(void) {

  mp3_init();

  mp3_mutex = xSemaphoreCreateMutex();
  mp3_queue = xQueueCreate(1, sizeof(uint8_t[READ_BYTES_FROM_FILE]));

  xTaskCreate(read_song, "read_song", configMINIMAL_STACK_SIZE * 14, (void *)NULL, PRIORITY_LOW, NULL);
  xTaskCreate(send_data_to_decoder, "spi_send", configMINIMAL_STACK_SIZE * 12, (void *)NULL, PRIORITY_HIGH, NULL);

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

static void send_bytes_to_decoder(const uint32_t start_index, const uint32_t last_index, const uint8_t *bytes_to_send) {
  mp3_datacs();
  {
    for (uint32_t index = start_index; index < last_index; index++) {
      UNUSED ssp1__exchange_byte(bytes_to_send[index]);
      // printf("%ld. %d\n", index, bytes_to_send[index]);
    }
  }
  mp3_datads();
}

void read_song(void *p) {
  const char *filename = "RangDeBasanti.mp3";
  static uint8_t bytes_to_read[READ_BYTES_FROM_FILE];
  FRESULT result;
  FIL file;

  result = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
  UINT bytes_read;
  while (1) {
    xSemaphoreTake(mp3_mutex, portMAX_DELAY);

    result = f_read(&file, &bytes_to_read[0], READ_BYTES_FROM_FILE, &bytes_read);
    if (0 != result) {
      printf("Result of %s is %i\n", filename, result);
    }

    xSemaphoreGive(mp3_mutex);
    xQueueSend(mp3_queue, &bytes_to_read[0], portMAX_DELAY);
  }
}

void send_data_to_decoder(void *p) {
  static uint8_t bytes_to_read[READ_BYTES_FROM_FILE];
  static uint8_t current_count = 0;
  uint32_t start_index = 0, last_index = 0;
  while (1) {
    // bool earlier_transfer_complete = false;
    if (current_count == 0) {
      xQueueReceive(mp3_queue, &bytes_to_read[0], portMAX_DELAY);
      // earlier_transfer_complete = true;
    }
    start_index = (current_count * MAX_BYTES_TX);
    last_index = ((current_count + 1) * MAX_BYTES_TX);

    while (!mp3_dreq_get_status()) {
      printf("data not requested\n");
      vTaskDelay(2);
    }
    if (xSemaphoreTake(mp3_mutex, portMAX_DELAY)) {

      send_bytes_to_decoder(start_index, last_index, &bytes_to_read[0]);
      xSemaphoreGive(mp3_mutex);
      if (current_count == COUNT - 1) {
        // printf("count done\n");
        current_count = start_index = last_index = 0;
      } else {
        current_count += 1;
        // printf("count = %d\n", current_count);
      }
    }
  }
}
