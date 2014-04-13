#ifndef BANO_SOCKET_H_INCLUDED
#define BANO_SOCKET_H_INCLUDED


#include <stdint.h>
#include "bano_perror.h"


/* fwd decls */

struct bano_base;
struct bano_node;
#ifdef BANO_CONFIG_HTTPD
struct bano_httpd;
#endif /* BANO_CONFIG_HTTPD */

/* socket abstraction layer */

typedef enum
{
  BANO_SOCKET_TYPE_SNRF = 0,

#ifdef BANO_CONFIG_HTTPD
  BANO_SOCKET_TYPE_HTTPD,
#endif /* BANO_CONFIG_HTTPD */

  BANO_SOCKET_TYPE_INVALID
} bano_socket_type_t;


typedef struct bano_socket
{
  bano_socket_type_t type;
  int (*read_fn)(void*, void*);
  int (*peek_fn)(void*, void*);
  int (*write_fn)(void*, uint32_t, const void*);
  int (*ctl_fn)(void*, unsigned int, unsigned int);
  int (*close_fn)(void*);
  int (*get_fd_fn)(void*);
  void* data;
} bano_socket_t;


/* socket info */

typedef struct
{
  const char* dev_path;
  size_t addr_width;
  uint32_t addr_val;
} bano_snrf_info_t;

typedef struct bano_socket_info
{
  bano_socket_type_t type;

  union
  {
    bano_snrf_info_t snrf;

#ifdef BANO_CONFIG_HTTPD
    struct bano_httpd* httpd;
#endif /* BANO_CONFIG_HTTPD */

  } u;

} bano_socket_info_t;


/* exported */

int bano_socket_alloc(bano_socket_t**, const bano_socket_info_t*);
int bano_socket_free(bano_socket_t*, struct bano_base*);
int bano_socket_snrf_open(bano_socket_t*, const bano_snrf_info_t*);
#ifdef BANO_CONFIG_HTTPD
int bano_socket_httpd_open(bano_socket_t*, struct bano_httpd*);
#endif /* BANO_CONFIG_HTTPD */

static inline int bano_socket_read(bano_socket_t* s, void* m)
{
  return s->read_fn(s->data, m);
}

static inline int bano_socket_peek(bano_socket_t* s, void* m)
{
  return s->peek_fn(s->data, m);
}

static inline int bano_socket_write
(bano_socket_t* s, uint32_t addr, const void* m)
{
  return s->write_fn(s->data, addr, m);
}

static inline int bano_socket_ctl
(bano_socket_t* s, unsigned int k, unsigned int v)
{
  return s->ctl_fn(s->data, k, v);
}

static inline int bano_socket_close(bano_socket_t* s)
{
  return s->close_fn(s->data);
}

static inline int bano_socket_get_fd(bano_socket_t* s)
{
  return s->get_fd_fn(s->data);
}

static inline void bano_socket_init_info(bano_socket_info_t* i)
{
  i->type = BANO_SOCKET_TYPE_INVALID;
}


#endif /* ! BANO_SOCKET_H_INCLUDED */
