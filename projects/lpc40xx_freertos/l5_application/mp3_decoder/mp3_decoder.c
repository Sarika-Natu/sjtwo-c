
#include "mp3_decoder.h"
#include <stdio.h>

static void mp3_configure_gpio(void);
static void mp3_cs(void);
static void mp3_ds(void);
void mp3_datacs(void);
void mp3_datads(void);
static void mp3_reset_high(void);
static void mp3_reset_low(void);
static void mp3_write(uint8_t opcode, uint16_t data);
static uint16_t mp3_read_data(uint8_t opcode);
static void mp3_hardware_init(void);
static void mp3_setmode(void);
static void mp3_setclockf(void);
static void mp3_audata(void);
static void mp3_bass(void);
static void mp3_volume(void);

/*******************************************************************************
 *
 *                      P U B L I C    F U N C T I O N S
 *
 ******************************************************************************/

void mp3_init(void) {
  uint32_t spi_clock_mhz = 1 * 1000 * 1000;
  ssp1__initialize(spi_clock_mhz);

  mp3_configure_gpio();
  mp3_hardware_init();
  // mp3_pin_status();
  mp3_setmode();
  mp3_setclockf();
  mp3_audata();
  mp3_bass();
  mp3_volume();

  mp3_read_data(SCI_MODE);
  mp3_read_data(SCI_CLOCKF);
  // mp3_pin_status();
  spi_clock_mhz = spi_clock_mhz * 12;
  ssp1__initialize(spi_clock_mhz);
}

void mp3_pin_status(void) {
  const uint8_t mp3_datareq = 1;
  const gpio_s mp3_datareq_gpio = {GPIO__PORT_0, mp3_datareq};
  const uint8_t mp3_chipsel = 22;
  const gpio_s mp3_chipsel_gpio = {GPIO__PORT_0, mp3_chipsel};
  const uint8_t mp3_datacs = 0;
  const gpio_s mp3_datacs_gpio = {GPIO__PORT_0, mp3_datacs};
  const uint8_t mp3_reset = 18;
  const gpio_s mp3_reset_gpio = {GPIO__PORT_0, mp3_reset};
  printf("Get pin__mp3_cs status: %i\n", gpio__get(mp3_chipsel_gpio));
  printf("Get pin__mp3_data_cs status: %i\n", gpio__get(mp3_datacs_gpio));
  printf("Get pin__mp3_reset status: %i\n", gpio__get(mp3_reset_gpio));
  printf("Get pin__mp3_dreq status: %i\n", gpio__get(mp3_datareq_gpio));
  printf("\n");
}

bool mp3_dreq_get_status(void) {
  const uint8_t mp3_datareq = 1;
  const gpio_s mp3_datareq_gpio = {GPIO__PORT_0, mp3_datareq};
  bool dreq_status = gpio__get(mp3_datareq_gpio);
  return dreq_status;
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

void mp3_datacs(void) {
  const uint8_t mp3_datacs = 0;
  const gpio_s mp3_datacs_gpio = {GPIO__PORT_0, mp3_datacs};
  gpio__reset(mp3_datacs_gpio);
}

void mp3_datads(void) {
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
  // while (!mp3_dreq_get_status())
  ;
  mp3_cs();
  {
    (void)ssp1__exchange_byte(MP3_WRITE);
    (void)ssp1__exchange_byte(opcode);
    uint8_t data_lsb = (byte_mask & data);
    uint8_t data_msb = (byte_mask & (data >> one_byte));
    (void)ssp1__exchange_byte(data_msb);
    (void)ssp1__exchange_byte(data_lsb);
    printf("WRITE:    %.2x", data_msb);
    printf("    %.2x\n", data_lsb);
  }
  mp3_ds();
}

static uint16_t mp3_read_data(uint8_t opcode) {
  uint16_t data = 0;
  // while (!mp3_dreq_get_status())
  ;
  mp3_cs();
  {
    (void)ssp1__exchange_byte(MP3_READ);
    (void)ssp1__exchange_byte(opcode);
    printf("READ: %.2x", ssp1__exchange_byte(0xFF));
    printf("%.2x\n", ssp1__exchange_byte(0xFF));
  }
  mp3_ds();

  return data;
}
static void mp3_reset(void) {
  mp3_reset_low();
  delay__ms(2);
  mp3_reset_high();
  delay__ms(2);
}

static void mp3_hardware_init(void) {
  mp3_ds();
  mp3_datads();
  mp3_reset();
}

static void mp3_setmode(void) {
  const uint16_t SM_SDINEW = (1 << 11);
  const uint16_t SM_LINE1 = (1 << 14);
  uint16_t mp3_mode = (SM_SDINEW | SM_LINE1); // 0x4800;
  mp3_write(SCI_MODE, mp3_mode);
}

static void mp3_setclockf(void) {
  const uint16_t SC_MULT = (0b110 << 13);
  const uint16_t SC_ADD = (0 << 11);
  const uint16_t SC_FREQ = (0 << 0);
  uint16_t mp3_clockf = (SC_MULT | SC_ADD | SC_FREQ); // 0xD800
  mp3_write(SCI_CLOCKF, mp3_clockf);
}

static void mp3_audata(void) {
  const uint16_t stereo_mode = 0xAC80;
  mp3_write(SCI_AUDATA, stereo_mode);
}

static void mp3_volume(void) {
  const uint16_t mid_vol = 0x7F7F; // 0x0101;
  mp3_write(SCI_VOL, mid_vol);
}

static void mp3_bass(void) {
  const uint16_t bass = 0x00F6;
  mp3_write(SCI_BASS, bass);
}