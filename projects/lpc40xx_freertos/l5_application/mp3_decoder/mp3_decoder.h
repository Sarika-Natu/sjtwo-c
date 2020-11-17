#pragma once

#include "file_io_sdcard.h"
#include "gpio.h"
#include "ssp1.h"
#include <stdint.h>
#include <stdlib.h>

#define MP3_WRITE 0x02
#define MP3_READ 0x03

#define SCI_MODE 0x0        // Mode control
#define SCI_STATUS 0x1      // Status of VS1053b
#define SCI_BASS 0x2        // Built-in bass/treble control
#define SCI_CLOCKF 0x3      // Clock freq + multiplier
#define SCI_DECODE_TIME 0x4 // Decode time in seconds
#define SCI_AUDATA 0x5      // Misc. audio data
#define SCI_WRAM 0x6        // RAM write/read
#define SCI_WRAMADDR 0x7    // Base address for RAM write/read
#define SCI_HDAT0 0x8       // Stream header data 0
#define SCI_HDAT1 0x9       // Stream header data 1
#define SCI_AIADDR 0xA      // Start address of application
#define SCI_VOL 0xB         // Volume control
#define SCI_AICTRL0 0xC     // Application control register 0
#define SCI_AICTRL1 0xD     // Application control register 1
#define SCI_AICTRL2 0xE     // Application control register 2
#define SCI_AICTRL3 0xF     // Application control register 3

void mp3_init(void);
bool mp3_dreq_get_status(void);
void mp3_datacs(void);
void mp3_datads(void);
