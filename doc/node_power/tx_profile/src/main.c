#include <stdint.h>
#include <avr/io.h>
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

#define KEY_LED_VALUE 0x0000
#define KEY_MAGIC_SEED 0x0001
#define KEY_MAGIC_VALUE 0x0002

static uint8_t led_value = 0;
static volatile uint32_t magic_value = 0xdeadbeef;

uint8_t bano_get_handler(uint16_t key, uint32_t* val)
{
  switch (key)
  {
  case KEY_LED_VALUE: *val = led_value; break ;
  case KEY_MAGIC_SEED: *val = magic_value; break ;
  default: return BANO_MSG_FLAG_ERR;
  }

  return 0;
}

uint8_t bano_set_handler(uint16_t key, uint32_t val)
{
  switch (key)
  {
  case KEY_LED_VALUE:
    {
      if (led_value != val)
      {
	led_value = val;
	if (led_value == 0) led_set_low();
	else led_set_high();
      }
      return BANO_MSG_FLAG_REPLY;
    }

  case KEY_MAGIC_SEED:
    {
      magic_value = val;
      return 0;
    }

  default: break ;
  }

  return BANO_MSG_FLAG_ERR;
}

void bano_timer_handler(void)
{
  bano_send_set(KEY_MAGIC_VALUE, magic_value);
  bano_send_set(KEY_MAGIC_VALUE, magic_value);
  bano_send_set(KEY_MAGIC_VALUE, magic_value);
  bano_send_set(KEY_MAGIC_VALUE, magic_value);
  bano_send_set(KEY_MAGIC_VALUE, magic_value);

  ++magic_value;
}

void bano_pcint_handler(void)
{
}

/* main */

int main(void)
{
  bano_info_t info;

  led_setup();
  led_value = 0;
  led_set_low();

  info = bano_info_default;

#if 1
  info.wake_mask |= BANO_WAKE_TIMER;
  info.timer_100ms = 50;
#endif

#if 0
  info.wake_mask |= BANO_WAKE_MSG;
#endif
  info.disable_mask = BANO_DISABLE_ALL;

  bano_init(&info);

  nrf905_set_pa_pwr(3);
  nrf905_cmd_wc();

  bano_loop();
  bano_fini();

  return 0;
}
