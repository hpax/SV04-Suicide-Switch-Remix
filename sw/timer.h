/*
 * Timer, using the timer1 counter (also used for LED PWM)
 * It runs at 1000000/256 = 3905.25 Hz.
 */
#ifndef TIMER_H
#define TIMER_H

#include "util.h"

#define TIMER_HZ (FREQ/256)

union dword {
    uint8_t b[4];
    uint16_t w[2];
    uint32_t l;
};

/* External data accessed by timer tick */
struct timer_in {
    bool state;
    uint8_t ctr;
};
struct timer_state {
    union dword tick;
    struct timer_in button;
    struct timer_in off;
    struct {
	uint8_t flash_ctr;
	int8_t  flash_dir;
	uint8_t flash_val;
	uint8_t mode[3];
    } led;
} __attribute__((aligned(32)));	/* Keep from crossing 256-byte boundaries */
extern volatile struct timer_state _timer;

/* Atomically read the timer */
static inline uint32_t timer32(void)
{
    uint32_t v;
    uint8_t irqsave = SREG;
    cli();
    v = _timer.tick.l;
    SREG = irqsave;
    return v;
}
static inline uint16_t timer16(void)
{
    uint16_t v;
    uint8_t irqsave = SREG;
    cli();
    v = _timer.tick.w[0];
    SREG = irqsave;
    return v;
}
static inline uint16_t timer8(void)
{
    return _timer.tick.b[0];
}

/* High resolution timer read */
static inline uint32_t timer_us32(void)
{
    union dword t;

    do {
	t.b[1] = _timer.tick.b[0];
	t.b[0] = TCNT1;
	t.b[2] = _timer.tick.b[1];
	t.b[3] = _timer.tick.b[2];
    } while (t.b[1] != _timer.tick.b[0]);

    return t.l;
}
static inline uint16_t timer_us16(void)
{
    union {
	uint8_t b[2];
	uint16_t w;
    } t;

    do {
	t.b[1] = _timer.tick.b[0];
	t.b[0] = TCNT1;
    } while (t.b[1] != _timer.tick.b[0]);

    return t.w;
}
static inline uint16_t timer_us8(void)
{
    return TCNT1;
}

#endif /* TIMER_H */
