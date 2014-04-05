#ifndef BANO_NODE_H_INCLUDED
#define BANO_NODE_H_INCLUDED


#include <stdint.h>
#include "bano/src/common/bano_common.h"


/* static configuration */

#ifndef BANO_CONFIG_NODE_ADDR
#error "BANO_CONFIG_NODE_ADDR not defined"
#endif /* BANO_CONFIG_NODE_ADDR */

#ifndef BANO_CONFIG_NODE_SEED
#error "BANO_CONFIG_NODE_SEED not defined"
#endif /* BANO_CONFIG_NODE_SEED */


/* events */

#define BANO_EV_ERROR (1 << 0)
#define BANO_EV_TIMER (1 << 1)
#define BANO_EV_MSG (1 << 2)
#define BANO_EV_PCINT (1 << 3)
/* max: 1 << 8 */

/* info given to bano_init */
typedef struct
{
  /* 100 milliseconds units, max 10736 */
  uint16_t timer_100ms;

  /* waking event mask */
#define BANO_WAKE_NONE 0
#define BANO_WAKE_TIMER (1 << 0)
#define BANO_WAKE_MSG (1 << 1)
#define BANO_WAKE_POLL (1 << 2)
#define BANO_WAKE_PCINT (1 << 3)
  uint8_t wake_mask;

  /* module disabling mask */
#define BANO_DISABLE_ADC (1 << 0)
#define BANO_DISABLE_WDT (1 << 1)
#define BANO_DISABLE_CMP (1 << 2)
#define BANO_DISABLE_USART (1 << 3)
#define BANO_DISABLE_NONE 0x00
#define BANO_DISABLE_ALL 0xff
  uint8_t disable_mask;

  uint32_t pcint_mask;

  /* nodle identifier */
  uint32_t nodl_id;

} bano_info_t;

static const bano_info_t bano_info_default =
{
  .wake_mask = BANO_WAKE_NONE,
  .disable_mask = BANO_DISABLE_NONE,
  .pcint_mask = 0,
  .nodl_id = 0
};

/* exported */
uint8_t bano_init(const bano_info_t*);
uint8_t bano_fini(void);
uint8_t bano_send_set(uint16_t, uint32_t);
uint8_t bano_wait_event(bano_msg_t*);
uint8_t bano_loop(void);

/* implemented by the user */
extern uint8_t bano_set_handler(uint16_t, uint32_t);
extern uint8_t bano_get_handler(uint16_t, uint32_t*);
extern uint8_t bano_timer_handler(void);
extern uint8_t bano_pcint_handler(void);


/* implementation */
#include "bano/src/node/bano_node.c"


#endif /* ! BANO_NODE_H_INCLUDED */
