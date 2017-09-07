#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef BAUD
#define BAUD 9600
#endif

static void uart_putchar( char data );
static void uart_init() __attribute__((constructor));

static FILE uart_stream = FDEV_SETUP_STREAM( uart_putchar, NULL, _FDEV_SETUP_WRITE );

static void uart_init(){
  uint16_t ubrr = F_CPU/16/BAUD-1;
  UBRR0H = (uint8_t)(ubrr>>8);
  UBRR0L = (uint8_t)ubrr;
  UCSR0B = (1<<RXEN0)|(1<<TXEN0);
  stdout = &uart_stream;
}

static void uart_putchar( char data ){
  while(!( UCSR0A & (_BV(UDRE0)) ));
  UDR0 = data;
}

