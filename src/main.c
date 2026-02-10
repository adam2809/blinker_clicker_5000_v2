///////////////////////////////////////////////////////////////////////////////
//  Copyright Christopher Kormanyos 2007 - 2025.
//  Distributed under the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt
//  or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// The LED program.

#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stddef.h>
#include <string.h>

#include "pins.h"
#include "uart.h"

#define BAUD 9600                     // Baud rate in bits per second (bps).
#define MY_UBRR F_CPU / 8 / BAUD - 1  // UART Baud Rate Register.
#define RX_BUFFER_SIZE 16
#define TCNT1_IDLE_LED_TOGGLE_THRESHOLD 31249U
#define TCNT1_SUCCESS_LED_BLINK_ON_THRESHOLD        62498
#define TCNT1_SUCCESS_LED_BLINK_OFF_THRESHOLD        63498


T_Pin pin_led_red = {
  .port_ptr = &PORTD,
  .ddr_ptr = &DDRD,
  .pin_ptr = &PIND,
  .bit_index = 3U
};

T_Pin pin_led_blue = {
  .port_ptr = &PORTD,
  .ddr_ptr = &DDRD,
  .pin_ptr = &PIND,
  .bit_index = 5U
};
T_Pin pin_button = {
  .port_ptr = &PORTB,
  .ddr_ptr = &DDRB,
  .pin_ptr = &PINB,
  .bit_index = 1U
};

volatile bool led_timer_flag = false;

void timer1_init(void)
{
  TCCR1B |= (1 << CS12);
}

volatile static int timeout  = 0;

ISR(TIMER2_COMPA_vect){
  timeout = 1;
}

void timer0_init(void){
  TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20); // Prescaler 1024
}

enum {
  eTxReleased,
  eRxReleased,
  eTxPressed,
  eRxPressed,
} E_UsartState;

int main(void)
{
  uint8_t data_pressed[] = {
    0x01U,
    0x05U,
    0x00U, 0x00U,
    0xFFU, 0x00U,
    0x8CU, 0x3AU
  };

  uint8_t data_pressed_expected_response[] = {
    0x1U,
    0x5U,
    0x0U, 0x0U,
    0xFFU, 0x0U,
    0x8CU, 0x3AU
  };

  uint8_t data_released[] = {
    0x01U,
    0x05U,
    0x00U, 0x01U,
    0xFFU, 0x00U,
    0xDDU, 0xFAU
  };

  uint8_t data_released_expected_response[] = {
    0x01U,
    0x05U,
    0x00U, 0x01U,
    0xFFU, 0x00U,
    0xDDU, 0xFAU
  };

  uint8_t rx_buffer_relased[RX_BUFFER_SIZE];
  uint8_t rx_buffer_pressed[RX_BUFFER_SIZE];

  T_UartCtx tx_ctx_released;
  tx_ctx_released.data = data_released;
  tx_ctx_released.data_len = sizeof(data_released);
  usart_ctx_init(&tx_ctx_released);

  T_UartCtx rx_ctx_released;
  rx_ctx_released.data = rx_buffer_relased;
  rx_ctx_released.data_len = sizeof(data_released_expected_response);
  usart_ctx_init(&rx_ctx_released);

  T_UartCtx tx_ctx_pressed;
  tx_ctx_pressed.data = data_pressed;
  tx_ctx_pressed.data_len = sizeof(data_pressed);
  usart_ctx_init(&tx_ctx_pressed);

  T_UartCtx rx_ctx_pressed;
  rx_ctx_pressed.data = rx_buffer_pressed;
  rx_ctx_pressed.data_len = sizeof(data_pressed_expected_response);
  usart_ctx_init(&rx_ctx_pressed);

  int usart_state = eTxReleased; 

  usart_init(MY_UBRR);

  pin_out_enable(&pin_led_red);
  pin_out_enable(&pin_led_blue);
  pin_in_enable(&pin_button);
  pin_in_pullup(&pin_button);

  timer1_init();
  timer0_init();
  sei();

  int is_connected = 0;
  int blinks_requested = 0;
  for (;;)
  {

    if(usart_state == eTxReleased){
      usart_transmit(&tx_ctx_released);
      if(tx_ctx_released.is_timeout){
        is_connected = 0;
        usart_ctx_init(&tx_ctx_released);
      }
      if (tx_ctx_released.is_done)
      {
        usart_ctx_init(&rx_ctx_released);
        usart_state = eRxReleased;
      }
    }

    if(usart_state == eRxReleased){
      usart_receive(&rx_ctx_released);
      if(rx_ctx_released.is_timeout){
        is_connected = 0;
        usart_ctx_init(&tx_ctx_released);
        usart_state = eTxReleased;
      }
      if (rx_ctx_released.is_done)
      {
        if (memcmp(rx_ctx_released.data, data_released_expected_response, rx_ctx_released.data_len) == 0){
          is_connected = 1;
        }
        usart_ctx_init(&tx_ctx_released);
        usart_state = eTxReleased;

        if(pin_in_check(&pin_button) == 0 && blinks_requested == 0){
          usart_ctx_init(&tx_ctx_pressed);
          usart_state = eTxPressed;
        }
      }
    }

    if(usart_state == eTxPressed){
      usart_transmit(&tx_ctx_pressed);
      if(tx_ctx_pressed.is_timeout){
        is_connected = 0;
        usart_ctx_init(&tx_ctx_released);
        usart_state = eTxReleased;
      }
      if (tx_ctx_pressed.is_done)
      {
        usart_ctx_init(&rx_ctx_pressed);
        usart_state = eRxPressed;
      }
    }

    if(usart_state == eRxPressed){
      usart_receive(&rx_ctx_pressed);
      if(rx_ctx_pressed.is_timeout){
        is_connected = 0;
        usart_ctx_init(&tx_ctx_released);
        usart_state = eTxReleased;
      }
      if (rx_ctx_pressed.is_done)
      {
        if (memcmp(rx_ctx_pressed.data, data_pressed_expected_response, rx_ctx_pressed.data_len) == 0){
          is_connected = 1;
          blinks_requested = 3;
          cli();
          TCNT1 = 0;
          sei();
        }
        usart_ctx_init(&tx_ctx_released);
        usart_state = eTxReleased;
      }
    }

    if(is_connected == 1){
      pin_out_high(&pin_led_blue);
      pin_out_low(&pin_led_blue);

      if(blinks_requested > 0){
        if (TCNT1 > TCNT1_SUCCESS_LED_BLINK_ON_THRESHOLD){
          pin_out_high(&pin_led_red);
          
        }
        if (TCNT1 > TCNT1_SUCCESS_LED_BLINK_OFF_THRESHOLD){
          pin_out_low(&pin_led_red);
          cli();
          TCNT1 = 0;
          sei();
          blinks_requested--;
        }
        
      }

    }else{
      pin_out_low(&pin_led_blue);

      if(TCNT1 > TCNT1_IDLE_LED_TOGGLE_THRESHOLD){
        pin_out_toggle(&pin_led_red);
        cli();
        TCNT1 = 0;
        sei();
      }
    }
  }
}
