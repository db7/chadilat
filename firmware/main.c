#include "usb_serial.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <stdbool.h>
#include <util/atomic.h>
#include <util/delay.h>

#define LED_CONFIG (DDRD |= (1 << 6))
#define LED_ON (PORTD |= (1 << 6))
#define LED_OFF (PORTD &= ~(1 << 6))

// -----------------------------------------------------------------------------
// buffer
// -----------------------------------------------------------------------------

#ifdef TIMER32BIT
#define RECORD_SIZE 8
#else
#define RECORD_SIZE 6
#endif
#define BUFFER_RECORDS (300)
#define BUFFER_SIZE (RECORD_SIZE * BUFFER_RECORDS)
#define FLUSH_IDLE 50

volatile uint8_t buf[BUFFER_SIZE];
volatile uint16_t head;
volatile uint8_t counter;

bool buf_write(uint32_t timestamp, uint8_t pinb) {
  // Write new record into the buffer. Return true if buffer is full.
  if (head + RECORD_SIZE >= BUFFER_SIZE) {
    LED_ON;
    return true;
  }

  buf[head++] = 0xAA;
  buf[head++] = counter++;
#ifdef TIMER32BIT
  buf[head++] = (timestamp >> 24) & 0xFF;
  buf[head++] = (timestamp >> 16) & 0xFF;
#endif
  buf[head++] = (timestamp >> 8) & 0xFF;
  buf[head++] = (timestamp >> 0) & 0xFF;
  buf[head++] = pinb;
  buf[head++] = 0xBB;
  return false;
}
void buf_reset(void) {
  counter = 0;
  head = 0;
  LED_OFF;
}

void buf_flush(void) {
  if (head == 0)
    return;
  while (!(usb_serial_get_control() & USB_SERIAL_DTR))
    continue;
  usb_serial_write((const uint8_t *)buf, head);
  buf_reset();
}

// -----------------------------------------------------------------------------
// timer
// -----------------------------------------------------------------------------

void timer1_init(void) {
  TCCR1A = 0;
  TCCR1B = (1 << CS01) | (1 << CS00); // clk/64 → ~4µs per tick
  TCNT1 = 0;
#ifdef TIMER32BIT
  TIMSK1 = _BV(TOIE1);
  sei();
#endif
}

#ifdef TIMER32BIT
volatile uint32_t timer1_overflows = 0;

uint32_t read_clock() {
  uint16_t t1;
  uint32_t high;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    high = timer1_overflows;
    t1 = TCNT1;
  }
  return (high << 16) | t1;
}

ISR(TIMER1_OVF_vect) { timer1_overflows++; }

#else
uint32_t read_clock() {
  uint16_t t1 = TCNT1;
  return (uint32_t)t1;
}
#endif

// -----------------------------------------------------------------------------
// main loop
// -----------------------------------------------------------------------------

int main(void) {
  clock_prescale_set(clock_div_1);
  timer1_init();
  usb_init();
  LED_CONFIG;

  // configure port B as input and enable pull-up resistors
  DDRB &= ~0x0F;
  PORTB |= 0x0F;

  while (!usb_configured())
    continue;

  while (!(usb_serial_get_control() & USB_SERIAL_DTR))
    continue;

  uint8_t last_pinb = 0;
  uint32_t last_ts = 0;

  while (1) {
    uint32_t ts = read_clock();
    uint8_t pinb = PINB & 0x0F;

    if (pinb != last_pinb) {
      while (buf_write(last_ts, last_pinb))
        buf_flush();
      while (buf_write(ts, pinb))
        buf_flush();
      last_pinb = pinb;
    }

    if (ts - last_ts > FLUSH_IDLE)
      buf_flush();

    last_ts = ts;
  }
  return 0;
}
