#include <stdbool.h>

#include "file_io_sdcard.h"
#include "stdio.h"

FRESULT file_read(const char *filename, void *store_data_buffer, uint32_t bytes_to_read, UINT *bytes_read) {
  FRESULT result;
  FIL file;

  result = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
  if (FR_OK == result) {

    result = f_read(&file, store_data_buffer, bytes_to_read, bytes_read);
    if (FR_OK == result) {
    } else {
      printf("ERROR: Failed to write data to file\n");
    }
    f_close(&file);
  } else {
    printf("ERROR: Failed to open: %s\n", filename);
  }

  return result;
}
