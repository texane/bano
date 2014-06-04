#include <stdint.h>
#include <avr/io.h>
#include "./adc.c"
#include "bano/src/node/bano_node.h"


static void led_setup(void)
{
#define LED_DDR DDRB
#define LED_PORT PORTB
#define LED_MASK (1 << 1)

  LED_DDR |= LED_MASK;
}

static void led_set_high(void)
{
  LED_PORT |= LED_MASK;
}

static void led_set_low(void)
{
  LED_PORT &= ~LED_MASK;
}


/* bano handlers */

uint8_t bano_get_handler(uint16_t key, uint32_t* val)
{
  return BANO_MSG_FLAG_ERR;
}

uint8_t bano_set_handler(uint16_t key, uint32_t val)
{
  return BANO_MSG_FLAG_ERR;
}

void bano_timer_handler(void)
{
}

void bano_pcint_handler(void)
{
}

/* main */

int main(void)
{
  bano_info_t info;
  uint8_t i;
  uint8_t j;
  uint16_t x;

  info = bano_info_default;
  info.disable_mask = BANO_DISABLE_ALL;
  info.disable_mask &= ~BANO_DISABLE_ADC;
  bano_init(&info);
  nrf905_set_pa_pwr(3);
  nrf905_cmd_wc();

  adc_setup();
  adc_start_free_running();

  for (j = 0; j != 4; ++j)
  {
    x = 0;
    for (i = 0; i != 8; ++i) x += adc_read();
    x /= i;
    bano_send_set(0x2a2b, x);
    bano_send_set(0x2a2b, x);
  }

  bano_loop();
  bano_fini();

  return 0;
}
