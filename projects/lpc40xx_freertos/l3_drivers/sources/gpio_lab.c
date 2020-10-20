#include "gpio_lab.h"
#include "lpc40xx.h"

#define GPIO_STRUCT(PORTNUM) (LPC_AHB_BASE + 0x18000 + (PORTNUM * 0x20))

static void gpio__set_as_input(uint8_t port, uint8_t pin_num);
static void gpio__set_as_output(uint8_t port, uint8_t pin_num);
static void gpio__set_high(uint8_t port, uint8_t pin_num);
static void gpio__set_low(uint8_t port, uint8_t pin_num);
static void gpio__set(uint8_t port, uint8_t pin_num, bool high);
static bool gpio__get_level(uint8_t port, uint8_t pin_num);

bool vReadSw(uint8_t port, uint8_t pin_num);
void vCtrlLed(uint8_t port, uint8_t pin_num, bool high);
void vLedOn(uint8_t port, uint8_t pin_num);
void vLedOff(uint8_t port, uint8_t pin_num);
void vSetasOutput(uint8_t port, uint8_t pin_num);
void vSetasInput(uint8_t port, uint8_t pin_num);

static void gpio__set_as_input(port_t port, uint8_t pin_num) {
  switch (port) {
  case PORT0:
    LPC_GPIO0->DIR &= ~(1 << pin_num);
    break;

  case PORT1:
    LPC_GPIO1->DIR &= ~(1 << pin_num);
    break;

  case PORT2:
    LPC_GPIO2->DIR &= ~(1 << pin_num);
    break;

  default:
    break;
  }
}

static void gpio__set_as_output(port_t port, uint8_t pin_num) {
  switch (port) {
  case PORT0:
    LPC_GPIO0->DIR |= (1 << pin_num);
    break;

  case PORT1:
    LPC_GPIO1->DIR |= (1 << pin_num);
    break;

  case PORT2:
    LPC_GPIO2->DIR |= (1 << pin_num);
    break;

  default:
    break;
  }
}

static void gpio__set_high(port_t port, uint8_t pin_num) {
  switch (port) {
  case PORT0:
    LPC_GPIO0->SET = (1 << pin_num);
    break;

  case PORT1:
    LPC_GPIO1->SET = (1 << pin_num);
    break;

  case PORT2:
    LPC_GPIO2->SET = (1 << pin_num);
    break;

  default:
    break;
  }
}

static void gpio__set_low(port_t port, uint8_t pin_num) {
  switch (port) {
  case PORT0:
    LPC_GPIO0->CLR = (1 << pin_num);
    break;

  case PORT1:
    LPC_GPIO1->CLR = (1 << pin_num);
    break;

  case PORT2:
    LPC_GPIO2->CLR = (1 << pin_num);
    break;

  default:
    break;
  }
}

static void gpio__set(port_t port, uint8_t pin_num, bool high) {
  if (high) {
    switch (port) {
    case PORT0:
      LPC_GPIO0->PIN |= (1 << pin_num);
      break;

    case PORT1:
      LPC_GPIO1->PIN |= (1 << pin_num);
      break;

    case PORT2:
      LPC_GPIO2->PIN |= (1 << pin_num);
      break;

    default:
      break;
    }
  } else {
    switch (port) {
    case PORT0:
      LPC_GPIO0->PIN &= ~(1 << pin_num);
      break;

    case PORT1:
      LPC_GPIO1->PIN &= ~(1 << pin_num);
      break;

    case PORT2:
      LPC_GPIO2->PIN &= ~(1 << pin_num);
      break;

    default:
      break;
    }
  }
}

static bool gpio__get_level(port_t port, uint8_t pin_num) {
  bool level = false;
  const uint32_t pin = (1 << pin_num);

  if (((LPC_GPIO_TypeDef *)GPIO_STRUCT(port))->PIN & pin) {
    level = true;
  } else {
    level = false;
  }
  return level;
}

/*******************************************************************************
 *
 *                      P U B L I C    F U N C T I O N S
 *
 ******************************************************************************/

bool vReadSw(uint8_t port, uint8_t pin_num) { return gpio__get_level(port, pin_num); }

void vCtrlLed(uint8_t port, uint8_t pin_num, bool high) { gpio__set(port, pin_num, high); }

void vLedOn(uint8_t port, uint8_t pin_num) { gpio__set_high(port, pin_num); }

void vLedOff(uint8_t port, uint8_t pin_num) { gpio__set_low(port, pin_num); }

void vSetasOutput(uint8_t port, uint8_t pin_num) { gpio__set_as_output(port, pin_num); }

void vSetasInput(uint8_t port, uint8_t pin_num) { gpio__set_as_input(port, pin_num); }