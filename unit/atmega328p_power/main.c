#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

static void set_led(uint8_t mask)
{
#define LED_GREEN_POS 6
#define LED_RED_POS 7
#define LED_GREEN_MASK (1 << LED_GREEN_POS)
#define LED_RED_MASK (1 << LED_RED_POS)

  DDRD |= (LED_RED_MASK | LED_GREEN_MASK);
  PORTD &= ~(LED_RED_MASK | LED_GREEN_MASK);
  PORTD |= mask;
}


/* clock prescaler */

static inline void clk_set_prescal(uint8_t x)
{
  /* pow2, max 8 = 256 */

  CLKPR = 1 << 7;
  CLKPR = x << 0;

  /* wait for 4 - 2 cycles */
  __asm__ __volatile__ ("nop\n");
  __asm__ __volatile__ ("nop\n");
}

static inline void clk_set_prescal_max(void)
{
  clk_set_prescal(8);
}


/* sleep */

static void sleep_until_cond(uint8_t x)
{
  /* refer to ch 9.10, minimizing power consumption */

  /* disable usart0 */
  UCSR0B = 0;

  /* then set DDR as input */
  DDRD = 0;

  /* disable watchdog */
  cli();
  wdt_reset();
  MCUSR &= ~(1<<WDRF);
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  WDTCSR = 0x00;
  sei();

  /* disable comparator */
  ACSR = 1 << 7;

  /* disable adc */
  ADMUX = 0;
  ADCSRA = 0;
  PRR = 0xff;

  set_sleep_mode(x);
  sleep_enable();
  sleep_bod_disable();
  sleep_cpu();
}

static inline void sleep_until_pcint(void)
{
  sleep_until_cond(SLEEP_MODE_PWR_DOWN);
}

static inline void sleep_until_tmint_or_pcint(void)
{
  /* same as above, plus timer interrupt */
  sleep_until_cond(SLEEP_MODE_PWR_SAVE);
}

/* main */

int main(void)
{
  set_led(LED_RED_MASK | LED_GREEN_MASK);
  clk_set_prescal_max();
  set_led(0);

  sleep_until_pcint();
  /* sleep_until_tmint_or_pcint(); */

  /* set_led(LED_RED_MASK); */

  while (1) ;

  return 0;
}
