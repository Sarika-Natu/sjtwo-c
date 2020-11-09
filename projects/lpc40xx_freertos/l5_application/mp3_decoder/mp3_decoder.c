
#include "mp3_decoder.h"

static void mp3_configure_gpio(void) {
  const uint8_t ssp1_sck = 7;
  const uint8_t ssp1_miso = 8;
  const uint8_t ssp1_mosi = 9;
  const uint8_t mp3_reset = 18;
  const uint8_t mp3_chipsel = 22;
  const uint8_t mp3_datareq = 1;
  const uint8_t mp3_datacs = 0;

  gpio__construct_with_function(GPIO__PORT_0, ssp1_sck, GPIO__FUNCTION_2);
  gpio__construct_with_function(GPIO__PORT_0, ssp1_miso, GPIO__FUNCTION_2);
  gpio__construct_with_function(GPIO__PORT_0, ssp1_mosi, GPIO__FUNCTION_2);
  gpio__construct_as_output(GPIO__PORT_0, mp3_reset);
  gpio__construct_as_output(GPIO__PORT_0, mp3_chipsel);
  gpio__construct_as_input(GPIO__PORT_0, mp3_datareq);
  gpio__construct_as_output(GPIO__PORT_0, mp3_datacs);
}

static void mp3_cs(void) {
  const uint8_t mp3_chipsel = 22;
  const gpio_s mp3_chipsel_gpio = {GPIO__PORT_0, mp3_chipsel};
  gpio__reset(mp3_chipsel_gpio);
}

static void mp3_ds(void) {
  const uint8_t mp3_chipsel = 22;
  const gpio_s mp3_chipsel_gpio = {GPIO__PORT_0, mp3_chipsel};
  gpio__set(mp3_chipsel_gpio);
}

static void mp3_write(uint8_t opcode, uint16_t data) {
  const uint8_t byte_mask = 0xFF;
  const uint8_t one_byte = 8;

  mp3_cs();
  {
    (void)ssp1__exchange_byte(MP3_WRITE);
    (void)ssp1__exchange_byte(opcode);
    uint8_t data_lsb = (byte_mask & data);
    uint8_t data_msb = (byte_mask & (data >> one_byte));
    (void)ssp1__exchange_byte(data_msb);
    (void)ssp1__exchange_byte(data_lsb);
  }
  mp3_ds();
}

static void mp3_setmode(void) {
  const uint16_t SM_SDINEW = (1 << 11);
  const uint16_t SM_LINE1 = (1 << 14);
  uint16_t mp3_mode = (SM_SDINEW | SM_LINE1);
  mp3_write(SCI_MODE, mp3_mode);
}

void mp3_init(void) {
  ssp1__initialize();

  mp3_configure_gpio();
  mp3_setmode();
}
