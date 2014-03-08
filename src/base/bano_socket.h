#ifndef BANO_SOCKET_H_INCLUDED
#define BANO_SOCKET_H_INCLUDED


#include "bano_perror.h"


/* fwd decls */

struct bano_msg;

/* socket abstraction layer */

typedef struct bano_socket
{
  int (*read_fn)(void*, struct bano_msg*);
  int (*peek_fn)(void*, struct bano_msg*);
  int (*write_fn)(void*, const struct bano_msg*);
  int (*ctl_fn)(void*, unsigned int, unsigned int);
  int (*close_fn)(void*);
  int (*get_fd_fn)(void*);
  void* data;
} bano_socket_t;

/* exported */

int bano_socket_snrf_open(bano_socket_t*, const char*);

/* static inline */

static int bano_socket_unimpl_read(void* p, struct bano_msg* m)
{
  BANO_PERROR();
  return -1;
}

static int bano_socket_unimpl_peek(void* p, struct bano_msg* m)
{
  BANO_PERROR();
  return -1;
}

static int bano_socket_unimpl_write(void* p, const struct bano_msg* m)
{
  BANO_PERROR();
  return -1;
}

static int bano_socket_unimpl_ctl(void* p, unsigned int k, unsigned int v)
{
  BANO_PERROR();
  return -1;
}

static int bano_socket_unimpl_close(void* p)
{
  BANO_PERROR();
  return -1;
}

static int bano_socket_unimpl_get_fd(void* p)
{
  BANO_PERROR();
  return -1;
}

static inline int bano_socket_init(bano_socket_t* s)
{
  s->read_fn = bano_socket_unimpl_read;
  s->peek_fn = bano_socket_unimpl_peek;
  s->write_fn = bano_socket_unimpl_write;
  s->ctl_fn = bano_socket_unimpl_ctl;
  s->close_fn = bano_socket_unimpl_close;
  s->get_fd_fn = bano_socket_unimpl_get_fd;
  s->data = NULL;
  return 0;
}

static inline int bano_socket_read(bano_socket_t* s, struct bano_msg* m)
{
  return s->read_fn(s->data, m);
}

static inline int bano_socket_peek(bano_socket_t* s, struct bano_msg* m)
{
  return s->peek_fn(s->data, m);
}

static inline int bano_socket_write(bano_socket_t* s, const struct bano_msg* m)
{
  return s->write_fn(s->data, m);
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


#endif /* ! BANO_SOCKET_H_INCLUDED */