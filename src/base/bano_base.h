#ifndef BANO_BASE_H_INCLUDED
#define BANO_BASE_H_INCLUDED


#include <stdint.h>
#include "bano_list.h"
#include "../common/bano_common.h"


/* forward decls */

struct bano_node;
struct bano_io;
struct bano_socket;


/* functions pointer defs */

typedef int (*bano_set_fn_t)(void*, struct bano_node*, struct bano_io*);
typedef int (*bano_get_fn_t)(void*, struct bano_node*, struct bano_io*);
typedef int (*bano_timer_fn_t)(void*);
typedef int (*bano_compl_fn_t)(struct bano_io*, void*);
#define BANO_NODE_REASON_NEW 0
#define BANO_NODE_REASON_UNREACH 1
typedef int (*bano_node_fn_t)(void*, struct bano_node*, unsigned int);


/* loop infos */

typedef struct
{
  /* loop flags */
#define BANO_LOOP_FLAG_SET (1 << 0)
#define BANO_LOOP_FLAG_GET (1 << 1)
#define BANO_LOOP_FLAG_TIMER (1 << 2)
#define BANO_LOOP_FLAG_NODE (1 << 3)
  unsigned int flags;

  /* event handler fns */
  bano_set_fn_t set_fn;
  bano_get_fn_t get_fn;
  bano_node_fn_t node_fn;

  /* timer in milliseconds */
  bano_timer_fn_t timer_fn;
  unsigned int timer_ms;

  /* user pointer passed to xxx_fn */
  void* user_data;

} bano_loop_info_t;


/* io operation context */

typedef struct bano_io
{
  /* io flags */
#define BANO_IO_FLAG_TIMER (1 << 0)
#define BANO_IO_FLAG_ERR (1 << 1)
  unsigned int flags;

  /* valid if BANO_IO_FLAG_TIMER */
#define BANO_TIMER_NONE ((unsigned int)-1)
  unsigned int timer_ms;

  /* completion error */
#define BANO_IO_ERR_SUCCESS 0
#define BANO_IO_ERR_FAILURE 1
#define BANO_IO_ERR_TIMEOUT 2
#define BANO_IO_ERR_UNIMPL 3
  unsigned int compl_err;

  /* completion function and user data */
  bano_compl_fn_t compl_fn;
  void* compl_data;

  bano_msg_t msg;

} bano_io_t;


/* node context */

typedef struct bano_node
{
  uint16_t id;
  struct bano_socket* socket;
  bano_list_t posted_ios;
} bano_node_t;


/* base context */

typedef struct
{
  bano_list_t nodes;
  bano_list_t sockets;
} bano_base_t;


/* base info */

typedef struct
{
  const char* nrf_dev_path;
} bano_base_info_t;


/* exported */

int bano_init(void);
int bano_fini(void);
int bano_open(bano_base_t*, const bano_base_info_t*);
int bano_close(bano_base_t*);
int bano_start_loop(bano_base_t*, const bano_loop_info_t*);
bano_io_t* bano_alloc_get_io(uint16_t, bano_compl_fn_t, void*);
bano_io_t* bano_alloc_set_io(uint16_t, uint32_t, bano_compl_fn_t, void*);
void bano_free_io(bano_io_t*);
int bano_post_io(bano_base_t*, bano_node_t*, bano_io_t*);


/* static inlined */

static inline uint16_t bano_get_node_id(bano_node_t* node)
{
  return node->id;
}

static inline void bano_init_base_info(bano_base_info_t* i)
{
  i->nrf_dev_path = "/dev/ttyUSB0";
}

static inline void bano_init_loop_info(bano_loop_info_t* i)
{
  i->flags = 0;
}

static inline void bano_set_io_timer(bano_io_t* io, unsigned int ms)
{
  io->flags |= BANO_IO_FLAG_TIMER;
  io->timer_ms = ms;
}

static inline void bano_set_io_error(bano_io_t* io, unsigned int err)
{
  io->compl_err = err;
}


#endif /* ! BANO_BASE_H_INCLUDED */
