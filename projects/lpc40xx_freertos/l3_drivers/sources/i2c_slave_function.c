#include "i2c_slave_function.h"
#include <stdio.h>

static uint8_t i2c_slave_memory[80];

bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory) {
  bool ret_val = false;
  uint8_t no_of_memory_locations = sizeof(i2c_slave_memory) / sizeof(uint8_t); // 80/8=10 diff memory locations

  if (memory_index < no_of_memory_locations) {
    *memory = i2c_slave_memory[memory_index];
    ret_val = true;
  } else {
    printf("Memory index out of bound\n");
  }

  return ret_val;
}

bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
  bool ret_val = false;
  uint8_t no_of_memory_locations = sizeof(i2c_slave_memory) / sizeof(uint8_t);

  if (memory_index < no_of_memory_locations) {
    i2c_slave_memory[memory_index] = memory_value;
    ret_val = true;
  } else {
    printf("Memory index out of bound\n");
  }

  return ret_val;
}
