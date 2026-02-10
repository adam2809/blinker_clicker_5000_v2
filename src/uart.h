#ifndef UART_H
#define UART_H
#include <stddef.h>

#define TCNT1_RX_TIMEOUT_THRESHOLD 200U
#define TCNT1_TX_TIMEOUT_THRESHOLD 100U

typedef struct {
  uint8_t* data;
  int data_len;
  int index;
  int is_done;
  int is_timeout;
} T_UartCtx;

static inline void usart_init(unsigned int ubrr) {
  // Set baud rate.
  UBRR0H = (uint8_t)(ubrr >> 8);
  UBRR0L = (uint8_t)(ubrr);
  // UBRR0H = USART Baud Rate Register High.
  // UBRR0L = USART Baud Rate Register Low.

  // Enable receiver and transmitter.
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);
  // UCSR0B = USART Control and Status Register B.
  // RXEN0 = Receiver Enabled.
  // TXEN0 = Transmitter Enabled.

  // Double the USART transmission speed.
  UCSR0A = (1 << U2X0);

  // Set frame format: 1 stop bit, 8 data bits.
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
  // UCSR0C = USART Control and Status Register C.
  // UCSZ01, UCSZ00 = USART Character Size (8 bits).
}

static inline void usart_ctx_init(T_UartCtx* ctx) {
  cli();
  TCNT2 = 0;
  sei();

  ctx->index = 0;
  ctx->is_done = 0;
  ctx->is_timeout = 0;
}

static inline void usart_transmit(T_UartCtx* ctx) {
  if(TCNT2 > TCNT1_TX_TIMEOUT_THRESHOLD){
    ctx->is_timeout = 1;
    return;
  }

  if(UCSR0A & (1 << UDRE0)){
    UDR0 = ctx->data[ctx->index];
    ctx->index++;
    ctx->index = ctx->index % ctx->data_len;
    if(ctx->index == 0){
      ctx->is_done = 1;
    }

    cli();
    TCNT2 = 0;
    sei();
  }
}

static inline void usart_receive(T_UartCtx* ctx) {
  if(TCNT2 > TCNT1_RX_TIMEOUT_THRESHOLD){
    ctx->is_timeout = 1;
  }

  if (UCSR0A & (1 << RXC0)) {
    ctx->data[ctx->index] = UDR0;
    ctx->index++;
    ctx->index = ctx->index == ctx->data_len ? 0 : ctx->index;
    if(ctx->index == 0){
      ctx->is_done = 1;
    }

    cli();
    TCNT2 = 0;
    sei();
  }
}

#endif