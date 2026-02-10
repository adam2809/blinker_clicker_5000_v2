#ifndef _PINS_H
#define _PINS_H

#include <avr/io.h>

typedef struct T_Pin
{
  volatile uint8_t* port_ptr;
  volatile uint8_t* ddr_ptr;
  volatile uint8_t* pin_ptr;
  uint8_t bit_index;
} T_Pin;

static inline void pin_out_enable(T_Pin* pin)
{
  *(pin->ddr_ptr) |= (1U << pin->bit_index);
}

static inline void pin_out_high(T_Pin* pin)
{
  *(pin->port_ptr) |= (1U << pin->bit_index);
}

static inline void pin_out_low(T_Pin* pin)
{
  *(pin->port_ptr) &= ~(1U << pin->bit_index);
}

static inline void pin_out_toggle(T_Pin* pin)
{
  *(pin->port_ptr) ^= (1U << pin->bit_index);
}

static inline void pin_in_enable(T_Pin* pin)
{
  *(pin->ddr_ptr) &= ~(1U << pin->bit_index);
}

static inline void pin_in_pullup(T_Pin* pin)
{
  *(pin->port_ptr) |= (1U << pin->bit_index);
}

static inline bool pin_in_check(T_Pin* pin)
{
  return (*(pin->pin_ptr) >> pin->bit_index & 1U);
}

#endif