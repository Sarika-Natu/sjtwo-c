#pragma once

#include "ff.h"

FRESULT file_read(const char *filename, void *data_to_read, uint32_t byte_to_read, UINT *bytes_read);
