#ifndef BANO_BASE_H_INCLUDED
#define BANO_BASE_H_INCLUDED


#include <stdint.h>
#include <time.h>
#include "bano_list.h"
#include "bano_dict.h"
#include "bano_timer.h"
#include "bano_socket.h"
#include "bano_node.h"
#include "bano_cipher.h"
#include "../common/bano_common.h"

#ifdef BANO_CONFIG_HTTPD
#include "bano_httpd.h"
#endif /* BANO_CONFIG_HTTPD */


/* forward decls */

struct bano_io;
struct bano_socket;
struct bano_socket_info;


/* functions pointer defs */

typedef int (*bano_set_fn_t)(void*, bano_node_t*, struct bano_io*);
typedef int (*bano_get_fn_t)(void*, bano_node_t*, struct bano_io*);
typedef int (*bano_timer_fn_t)(void*);
typedef int (*bano_compl_fn_t)(struct bano_io*, void*);
#define BANO_NODE_REASON_NEW 0
#define BANO_NODE_REASON_UNREACH 1
typedef int (*bano_node_fn_t)(void*, bano_node_t*, unsigned int);


/* loop infos */

typedef struct
{
  /* loop flags */
  uint32_t flags;

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
  bano_node_t* node;

  /* io flags */
#define BANO_IO_FLAG_REPLY (1 << 0)
  uint32_t flags;

  /* valid if BANO_IO_FLAG_PEND */
  bano_timer_t* timer;
  unsigned int retry_ms;
  unsigned int retry_count;

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


/* base context */

typedef struct bano_base
{
  bano_list_t nodes;
  bano_list_t sockets;
  bano_list_t timers;
  bano_cipher_t cipher;
  uint32_t addr;

#ifdef BANO_CONFIG_HTTPD
  unsigned int is_httpd;
  bano_httpd_t httpd;
#endif /* BANO_CONFIG_HTTPD */

} bano_base_t;


/* base info */

typedef struct
{
#define BANO_BASE_FLAG_CONF (1 << 0)
#define BANO_BASE_FLAG_ADDR (1 << 1)
  uint32_t flags;
  const char* conf_path;
  uint32_t addr;
} bano_base_info_t;


/* exported */

int bano_init(void);
int bano_fini(void);
int bano_open(bano_base_t*, const bano_base_info_t*);
int bano_close(bano_base_t*);
int bano_add_socket(bano_base_t*, const bano_socket_info_t*);
int bano_add_node(bano_base_t*, const bano_node_info_t*);
int bano_find_node_by_addr(bano_base_t*, uint32_t, bano_node_t**);
int bano_start_loop(bano_base_t*, const bano_loop_info_t*);
bano_io_t* bano_alloc_get_io(uint16_t, bano_compl_fn_t, void*);
bano_io_t* bano_alloc_set_io
(uint16_t, uint32_t, unsigned int, bano_compl_fn_t, void*);
void bano_free_io(bano_io_t*);
int bano_post_io(bano_base_t*, bano_node_t*, bano_io_t*);


/* static inlined */

static inline void bano_init_base_info(bano_base_info_t* i)
{
  i->flags = 0;
}

static inline void bano_init_loop_info(bano_loop_info_t* i)
{
  i->flags = 0;

  i->set_fn = NULL;
  i->get_fn = NULL;
  i->node_fn = NULL;
  i->timer_fn = NULL;
}

static inline void bano_set_io_error(bano_io_t* io, unsigned int err)
{
  io->compl_err = err;
}


#endif /* ! BANO_BASE_H_INCLUDED */
