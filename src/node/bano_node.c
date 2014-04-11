#ifndef BANO_NODE_C_INCLUDED
#define BANO_NODE_C_INCLUDED


#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "bano_node.h"
#define CLK_PRESCAL (256UL)

#define NRF_CONFIG_NRF905 1
#if (NRF_CONFIG_NRF24L01P == 1)
#define NRF24L01P_PAYLOAD_WIDTH BANO_MSG_SIZE
#endif
#include "nrf/src/nrf.c"


/* architecture portability */

static inline uint16_t le_to_uint16(uint16_t x)
{
  return x;
}

static inline uint16_t uint16_to_le(uint16_t x)
{
  return x;
}

static inline uint32_t uint32_to_le(uint32_t x)
{
  return x;
}

static inline void uint32_to_le_buf(uint32_t x, uint8_t b[4])
{
  b[0] = ((uint8_t*)&x)[0];
  b[1] = ((uint8_t*)&x)[1];
  b[2] = ((uint8_t*)&x)[2];
  b[3] = ((uint8_t*)&x)[3];
}

static inline uint32_t le_to_uint32(uint32_t x)
{
  return x;
}


/* cryptographic cipher */

#if (BANO_CONFIG_CIPHER_ALG != BANO_CIPHER_ALG_NONE)
static const uint8_t cipher_key[] =
{
  BANO_CONFIG_CIPHER_KEY
};
#endif

#if (BANO_CONFIG_CIPHER_ALG == BANO_CIPHER_ALG_AES)
#include "aes.h"
static aes128_ctx_t cipher_ctx;
#elif (BANO_CONFIG_CIPHER_ALG == BANO_CIPHER_ALG_XTEA)
#include "xtea.h"
#endif

static void cipher_init(void)
{
#if (BANO_CONFIG_CIPHER_ALG == BANO_CIPHER_ALG_AES)
  aes128_init(cipher_key, &cipher_ctx);
#endif
}

static void cipher_enc(uint8_t* data)
{
#if (BANO_CONFIG_CIPHER_ALG == BANO_CIPHER_ALG_XTEA)
  xtea_enc(data + 0, data + 0, cipher_key);
  xtea_enc(data + 8, data + 8, cipher_key);
#elif (BANO_CONFIG_CIPHER_ALG == BANO_CIPHER_ALG_AES)
  aes128_dec(data, &cipher_ctx);
#endif
}

static void cipher_dec(uint8_t* data)
{
#if (BANO_CONFIG_CIPHER_ALG == BANO_CIPHER_ALG_XTEA)
  xtea_dec(data + 0, data + 0, cipher_key);
  xtea_dec(data + 8, data + 8, cipher_key);
#elif (BANO_CONFIG_CIPHER_ALG == BANO_CIPHER_ALG_AES)
  aes128_dec(data, &cipher_ctx);
#endif
}


/* core clock prescaler */

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


/* timer */

#define TIMER_PRESCAL (1024UL)

static volatile uint8_t timer_irq = 0;

ISR(TIMER1_COMPA_vect)
{
  /* stop the timer, signal interrupt */
  TCCR1B &= ~((1 << 3) - 1);
  timer_irq = 1;
}

static void timer_setup(uint16_t n)
{
  /* n the counter */
  
  /* stop timer */
  TCCR1B = 0;

  /* CTC mode, overflow when OCR1A reached */
  TCCR1A = 0;
  OCR1A = n;
  TCNT1 = 0;
  TCCR1C = 0;

  /* interrupt on OCIE0A match */
  TIMSK1 = 1 << 1;

  /* clear soft irq flag */
  timer_irq = 0;

  /* set mode high bits, 1024 prescal */
  TCCR1B = (1 << 3) | (5 << 0);
}


/* interrupt on change handlers */

/* NOTE: non applicative interrupts may be */
/* triggered due to the internal nrf logic */

static uint8_t pcint_irq = 0;

ISR(PCINT0_vect)
{
  /* pin change 0 interrupt handler */
  /* do nothing, processed by sequential in do_nrf */
  pcint_irq = 1;
}

ISR(PCINT1_vect)
{
  pcint_irq = 1;
}

ISR(PCINT2_vect)
{
  pcint_irq = 1;
}

static inline void pcint_setup(uint8_t m0, uint8_t m1, uint8_t m2)
{
  pcint_irq = 0;

  if (m0)
  {
    /* portb, mask0, pcint0 to pcint7 */
    /* disable pullups, set as input pin */
    PORTB &= ~m0;
    DDRB &= ~m0;
    PCICR |= 1 << 0;
    PCMSK0 |= m0;
  }

  if (m1)
  {
    /* portc, mask1, pcint8 to pcint15 */
    PORTC &= ~m1;
    DDRC &= ~m1;
    PCICR |= 1 << 1;
    PCMSK1 |= m1;
  }

  if (m2)
  {
    /* portd, mask2, pcint16 to pcint23 */
    PORTD &= ~m2;
    DDRD &= ~m2;
    PCICR |= 1 << 2;
    PCMSK2 |= m2;
  }
}


/* captured info */

static uint8_t bano_wake_mask = 0;
static uint16_t bano_timer_counter = 0;
static uint16_t bano_timer_500ms = 0;


/* constructors */

uint8_t bano_init(const bano_info_t* info)
{
  uint8_t addr[4];

  /* initialize cryptographic cipher context */
  cipher_init();

  /* nrf setup and default state */
  nrf_setup();
  nrf_set_payload_width(BANO_MSG_SIZE);
  nrf_set_addr_width(4);

  uint32_to_le_buf(BANO_DEFAULT_BASE_ADDR, addr);
  nrf_set_tx_addr(addr);

  uint32_to_le_buf(BANO_CONFIG_NODE_ADDR, addr);
  nrf_set_rx_addr(addr);

  nrf_set_powerdown_mode();

  /* capture wake mask */
  bano_wake_mask = info->wake_mask;

  /* wake on message */
  if (info->wake_mask & BANO_WAKE_MSG)
  {
    nrf_setup_rx_irq();
  }

  /* wake on timer */
  if (info->wake_mask & (BANO_WAKE_TIMER | BANO_WAKE_POLL))
  {
    /* compute the timer counter to minimize the */
    /* interrupt frequency and reduce power consumption */

    static const uint32_t p = TIMER_PRESCAL * CLK_PRESCAL;
    bano_timer_counter = (F_CPU * (uint32_t)info->timer_100ms) / (p * 10UL);
    bano_timer_500ms = (F_CPU * (uint32_t)5) / (p * 10UL);
  }

  /* wake on pcint */
  if (info->wake_mask & BANO_WAKE_PCINT)
  {
    const uint8_t m0 = (info->pcint_mask >> 0UL) & 0xff;
    const uint8_t m1 = (info->pcint_mask >> 8UL) & 0xff;
    const uint8_t m2 = (info->pcint_mask >> 16UL) & 0xff;
    pcint_setup(m0, m1, m2);
  }

  /* sleep mode */
  if (info->wake_mask & (BANO_WAKE_TIMER | BANO_WAKE_POLL))
  {
    set_sleep_mode(SLEEP_MODE_IDLE);
  }
  else if (info->wake_mask & BANO_WAKE_MSG)
  {
    /* TODO: set_sleep_mode(SLEEP_MODE_PWR_DOWN); */
    set_sleep_mode(SLEEP_MODE_IDLE);
  }

  /* clock prescaler */
  clk_set_prescal_max();

  /* disable modules in sleep mode */
  /* refer to ch 9.10, minimizing power consumption */

  if (info->disable_mask & BANO_DISABLE_USART)
  {
    UCSR0B = 0;
    PRR |= (1 << 1);
  }

  if (info->disable_mask & BANO_DISABLE_WDT)
  {
    wdt_reset();
    MCUSR &= ~(1 << WDRF);
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = 0x00;
  }

  if (info->disable_mask & BANO_DISABLE_CMP)
  {
    ACSR = 1 << 7;
  }

  if (info->disable_mask & BANO_DISABLE_ADC)
  {
    ADMUX = 0;
    ADCSRA = 0;
    PRR |= (1 << 0);
  }

  return 0;
}

uint8_t bano_fini(void)
{
  return 0;
}


/* messaging */

static inline void send_msg(bano_msg_t* msg)
{
  msg->hdr.saddr = uint32_to_le(BANO_CONFIG_NODE_ADDR);

#if (BANO_CONFIG_CIPHER_ALG != BANO_CIPHER_ALG_NONE)
  cipher_enc(((uint8_t*)msg) + 1);
  msg->hdr.flags |= BANO_MSG_FLAG_ENC;
#endif

  nrf_send_payload_zero((uint8_t*)msg);
}

static inline void make_set_msg(bano_msg_t* msg, uint16_t key, uint32_t val)
{
  msg->hdr.op = BANO_MSG_OP_SET;
  msg->hdr.flags = 0;
  msg->u.set.key = uint16_to_le(key);
  msg->u.set.val = uint32_to_le(val);
}

static inline void make_get_msg(bano_msg_t* msg, uint16_t key)
{
  msg->hdr.op = BANO_MSG_OP_GET;
  msg->hdr.flags = 0;
  msg->u.get.key = uint16_to_le(key);
}

uint8_t bano_send_set(uint16_t key, uint32_t val)
{
  bano_msg_t msg;
  make_set_msg(&msg, key, val);
  send_msg(&msg);
  return 0;
}


/* event */

static uint8_t wait_msg_or_timer(bano_msg_t* msg)
{
  /* return (uint8_t)-1 if no msg received */
  return (uint8_t)-1;
}

uint8_t bano_wait_event(bano_msg_t* msg)
{
  uint8_t ev;

 redo_wait:
  /* wait for the next event */
  ev = 0;

  /* enable interrupts before looping */
  sei();

  if (bano_wake_mask & (BANO_WAKE_TIMER | BANO_WAKE_POLL))
  {
    timer_setup(bano_timer_counter);
  }

  /* TODO: move at init time ? */
  if (bano_wake_mask & BANO_WAKE_MSG)
  {
    nrf_set_rx_mode();
  }

  while (1)
  {
    sleep_disable();

    /* handle timer first */
    if ((bano_wake_mask & (BANO_WAKE_TIMER | BANO_WAKE_POLL)) && timer_irq)
    {
      timer_irq = 0;
      ev |= BANO_EV_TIMER;
      goto on_event;
    }

    if ((bano_wake_mask & BANO_WAKE_MSG) && nrf_get_rx_irq())
    {
      /* disable pcint interrupt since we are already awake */
      /* and entering the handler perturbates the execution */
      nrf_disable_rx_irq();

      /* capture the message */
      nrf_read_payload_zero((uint8_t*)msg);

      /* reenable interrupts */
      nrf_enable_rx_irq();

      cipher_dec(((uint8_t*)msg) + 1);

      ev |= BANO_EV_MSG;

      goto on_event;
    }

    if ((bano_wake_mask & BANO_WAKE_PCINT) && pcint_irq)
    {
      pcint_irq = 0;
      ev |= BANO_EV_PCINT;
      goto on_event;
    }

    /* the following procedure is used to not miss interrupts */
    /* disable interrupts, check if something available */
    /* otherwise, enable interrupt and sleep (sei, sleep) */
    /* the later ensures now interrupt is missed */

    sleep_enable();
    sleep_bod_disable();

    cli();

    if ((bano_wake_mask & (BANO_WAKE_TIMER | BANO_WAKE_POLL)) && timer_irq)
    {
      /* continue, do not sleep */
      sei();
    }
    else if ((bano_wake_mask & BANO_WAKE_MSG) && nrf_peek_rx_irq())
    {
      /* continue, do not sleep */
      sei();
    }
    else if ((bano_wake_mask & BANO_WAKE_PCINT) && pcint_irq)
    {
      /* continue, do not sleep */
      sei();
    }
    else
    {
      /* warning: keep the 2 instructions in the same block */
      /* atomic, no int schedule between sei and sleep_cpu */
      sei();
      sleep_cpu();
    }
  }

 on_event:
  /* TODO: move in at init time ? */
  if (bano_wake_mask & BANO_WAKE_MSG)
  {
    nrf_set_powerdown_mode();
  }

  if ((bano_wake_mask & BANO_WAKE_POLL) && (ev & BANO_EV_TIMER))
  {
    /* ask and wait for next message */
    make_get_msg(msg, BANO_KEY_NEXT_MSG);
    send_msg(msg);
    timer_setup(bano_timer_500ms);

    if (wait_msg_or_timer(msg) == (uint8_t)-1)
    {
      /* the user wants to execute on timer */
      if (!(bano_wake_mask & BANO_WAKE_TIMER))
	goto redo_wait;
    }
    else /* msg got */
    {
      ev |= BANO_EV_MSG;
    }
  }

  return ev;
}


/* main loop, event scheduling */

uint8_t bano_loop(void)
{
  bano_msg_t msg;
  uint8_t ev;

  while (1)
  {
    ev = bano_wait_event(&msg);

    /* do BANO_EV_MSG first as msg is lost in other handlers */

    if (ev & BANO_EV_MSG)
    {
      const uint8_t op = msg.hdr.op;

      if (op == BANO_MSG_OP_SET)
      {
	/* TODO: handle default keys */

	const uint16_t key = le_to_uint16(msg.u.set.key);
	uint32_t val = le_to_uint32(msg.u.set.val);
	const uint8_t flags = bano_set_handler(key, val);

	if (flags & BANO_MSG_FLAG_REPLY)
	{
	  msg.hdr.flags = flags;
	  send_msg(&msg);
	}
      }
      else if (op == BANO_MSG_OP_GET)
      {
	/* TODO: handle default keys */

	uint32_t val = 0;
	const uint16_t key = le_to_uint16(msg.u.get.key);
	const uint8_t flags = bano_get_handler(key, &val);
	make_set_msg(&msg, key, val);
	msg.hdr.flags = BANO_MSG_FLAG_REPLY | flags;
	send_msg(&msg);
      }
    }

    if (ev & BANO_EV_TIMER)
    {
      bano_timer_handler();
    }

    if (ev & BANO_EV_PCINT)
    {
      bano_pcint_handler();
    }

    if (ev & BANO_EV_ERROR)
    {
      /* TODO: report error */
    }

  } /* while */

  return 0;
}


#endif /* BANO_NODE_C_INCLUDED */
