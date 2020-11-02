#include "i2c_slave_init.h"
#include "i2c.h"
#include "lpc40xx.h"

static const uint8_t i2c_slave_mask_addr = 0x00;
static const uint8_t i2c_iocon_bits = 0x7C;
static const uint8_t aa_bit = (1 << 2);
static const uint8_t i2en_bit = (1 << 6);

static void i2c__set_mask_addr() { LPC_I2C2->MASK0 &= ~(i2c_slave_mask_addr); }
static void i2c__set_slave_addr(uint8_t slave_addr) { LPC_I2C2->ADR0 = slave_addr; }
static void i2c__clear_all_bits() { LPC_I2C2->CONCLR = i2c_iocon_bits; }
static void i2c__set_aa_intf() { LPC_I2C2->CONSET = (aa_bit | i2en_bit); }

void i2c2__slave_init(uint8_t slave_address_to_respond_to) {
  // const uint32_t i2c_speed_hz = UINT32_C(400) * 1000;
  // i2c__initialize(I2C__1, i2c_speed_hz, (UINT32_C(96) * 1000 * 1000));

  i2c__set_slave_addr(slave_address_to_respond_to);
  i2c__set_mask_addr();
  i2c__clear_all_bits();
  i2c__set_aa_intf();
}
