
#include "mp3_decoder.h"

static void mp3_configure_gpio(void);
static void mp3_cs(void);
static void mp3_ds(void);
static void mp3_datacs(void);
static void mp3_datads(void);
static void mp3_reset_high(void);
static void mp3_reset_low(void);
static void mp3_write(uint8_t opcode, uint16_t data);
uint16_t mp3_read_data(uint8_t opcode);

static void mp3_setmode(void);
static void mp3_setclockf(void);
static void mp3_audata(void);
static void mp3_volume(void);

/*******************************************************************************
 *
 *                      P U B L I C    F U N C T I O N S
 *
 ******************************************************************************/

void mp3_init(void) {
  ssp1__initialize();

  mp3_configure_gpio();
  mp3_setmode();
  mp3_setclockf();
  mp3_audata();
  mp3_volume();
}

/*******************************************************************************
 *
 *                      P R I V A T E    F U N C T I O N S
 *
 ******************************************************************************/
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

static void mp3_datacs(void) {
  const uint8_t mp3_datacs = 0;
  const gpio_s mp3_datacs_gpio = {GPIO__PORT_0, mp3_datacs};
  gpio__reset(mp3_datacs_gpio);
}

static void mp3_datads(void) {
  const uint8_t mp3_datacs = 0;
  const gpio_s mp3_datacs_gpio = {GPIO__PORT_0, mp3_datacs};
  gpio__set(mp3_datacs_gpio);
}

static void mp3_reset_high(void) {
  const uint8_t mp3_reset = 18;
  const gpio_s mp3_reset_gpio = {GPIO__PORT_0, mp3_reset};
  gpio__set(mp3_reset_gpio);
}

static void mp3_reset_low(void) {
  const uint8_t mp3_reset = 18;
  const gpio_s mp3_reset_gpio = {GPIO__PORT_0, mp3_reset};
  gpio__reset(mp3_reset_gpio);
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

uint16_t mp3_read_data(uint8_t opcode) {
  uint16_t data = 0;
  const uint8_t dummy_byte = 0xFF;
  const uint8_t one_byte = 8;

  mp3_cs();
  {
    (void)ssp1__exchange_byte(MP3_READ);
    (void)ssp1__exchange_byte(opcode);
    uint8_t data_msb = ssp1__exchange_byte(dummy_byte);
    uint8_t data_lsb = ssp1__exchange_byte(dummy_byte);
    data = (data_msb << one_byte);
    data |= data_lsb;
  }
  mp3_ds();

  return data;
}

static void mp3_setmode(void) {
  const uint16_t SM_SDINEW = (1 << 11);
  const uint16_t SM_LINE1 = (1 << 14);
  uint16_t mp3_mode = (SM_SDINEW | SM_LINE1);
  mp3_write(SCI_MODE, mp3_mode);
}

static void mp3_setclockf(void) {
  const uint16_t SC_MULT = (0b110 << 13);
  const uint16_t SC_ADD = (0 << 11);
  const uint16_t SC_FREQ = (0 << 0);
  uint16_t mp3_clockf = (SC_MULT | SC_ADD | SC_FREQ);
  mp3_write(SCI_CLOCKF, mp3_clockf);
}

static void mp3_audata(void) {
  const uint16_t stereo_mode = 0xAC80;
  mp3_write(SCI_AUDATA, stereo_mode);
}

static void mp3_volume(void) {
  const uint16_t mid_vol = 0x7FFF;
  mp3_write(SCI_VOL, mid_vol);
}