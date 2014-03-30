#ifndef BANO_BASE_H_INCLUDED
#define BANO_BASE_H_INCLUDED


#include <stdint.h>
#include "bano_list.h"
#include "bano_socket.h"
#include "../common/bano_common.h"


/* forward decls */

struct bano_node;
struct bano_io;
struct bano_socket;
struct bano_socket_info;


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

  /* completion value */
  uint32_t compl_val;

  /* completion function and user data */
  bano_compl_fn_t compl_fn;
  void* compl_data;

  bano_msg_t msg;

} bano_io_t;


/* node context */

typedef struct bano_node
{
  uint32_t addr;
  struct bano_socket* socket;
  bano_list_t posted_ios;
  bano_list_t pending_ios;
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
  unsigned int dummy;
} bano_base_info_t;


/* exported */

int bano_init(void);
int bano_fini(void);
int bano_open(bano_base_t*, const bano_base_info_t*);
int bano_close(bano_base_t*);
int bano_add_socket(bano_base_t*, const struct bano_socket_info*);
int bano_add_node(bano_base_t*, uint32_t);
int bano_start_loop(bano_base_t*, const bano_loop_info_t*);
bano_io_t* bano_alloc_get_io(uint16_t, bano_compl_fn_t, void*);
bano_io_t* bano_alloc_set_io(uint16_t, uint32_t, bano_compl_fn_t, void*);
void bano_free_io(bano_io_t*);
int bano_post_io(bano_base_t*, bano_node_t*, bano_io_t*);


/* static inlined */

static inline uint32_t bano_get_node_addr(bano_node_t* node)
{
  return node->addr;
}

static inline void bano_init_base_info(bano_base_info_t* i)
{
}

static inline void bano_init_loop_info(bano_loop_info_t* i)
{
  i->flags = 0;

  i->set_fn = NULL;
  i->get_fn = NULL;
  i->node_fn = NULL;
  i->timer_fn = NULL;
}

static inline void bano_init_socket_info(bano_socket_info_t* i)
{
  i->type = BANO_SOCKET_TYPE_INVALID;
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
