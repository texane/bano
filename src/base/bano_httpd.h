#ifndef BANO_HTTPD_H_INCLUDED
#define BANO_HTTPD_H_INCLUDED


#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include "mongoose.h"


struct bano_base;


typedef struct bano_httpd
{
  struct mg_server* server;
  struct bano_base* base;
  volatile unsigned int is_done;
  /* message passing interface */
  int msg_pipe[2];
  pthread_mutex_t compl_lock;
  pthread_cond_t compl_cond;
} bano_httpd_t;


typedef struct bano_httpd_info
{
#define BANO_HTTPD_FLAG_PORT (1 << 0)
  uint32_t flags;
  struct bano_base* base;
  uint16_t port;
} bano_httpd_info_t;


typedef struct bano_httpd_msg
{
  enum
  {
    BANO_HTTPD_MSG_OP_GET = 0,
    BANO_HTTPD_MSG_OP_SET,
    BANO_HTTPD_MSG_OP_INVALID
  } op;
  uint32_t naddr;
  uint16_t key;
  uint32_t val;
  bano_httpd_t* httpd;
  struct mg_connection* conn;
  int* compl_err;
} bano_httpd_msg_t;


int bano_httpd_init(bano_httpd_t*, const bano_httpd_info_t*);
int bano_httpd_fini(bano_httpd_t*);
int bano_httpd_complete_msg(bano_httpd_msg_t*, int, const void*, size_t);

static inline void bano_init_httpd_info
(bano_httpd_info_t* info, struct bano_base* base)
{
  info->flags = 0;
  info->base = base;
}


#endif /* BANO_HTTPD_H_INCLUDED */
