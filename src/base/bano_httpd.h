#ifndef BANO_HTTPD_H_INCLUDED
#define BANO_HTTPD_H_INCLUDED


#include <stdint.h>
#include <pthread.h>
#include "mongoose.h"


typedef struct bano_httpd
{
  struct mg_server* mg_server;

  /* message passing interface */
  int req_pipe[2];
  pthread_mutex_t rep_lock;
  pthread_cond_t rep_cond;

} bano_httpd_t;


typedef struct bano_httpd_info
{
#define BANO_HTTPD_FLAG_PORT (1 << 0)
  uint32_t flags;
} bano_httpd_info_t;


int bano_httpd_init(bano_httpd_t*, const bano_httpd_info_t*);
int bano_httpd_fini(bano_httpd_t*);


#endif /* BANO_HTTPD_H_INCLUDED */
