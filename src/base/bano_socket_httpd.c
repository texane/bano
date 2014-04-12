#include <stdlib.h>
#include "bano_perror.h"
#include "bano_socket.h"
#include "bano_httpd.h"
#include "../common/bano_common.h"


typedef struct socket_httpd
{
  bano_httpd_t* httpd;
} socket_httpd_t;


static int do_read(void* p, bano_msg_t* bano_msg)
{
  BANO_PERROR();
  return -1;
}

static int do_peek(void* p, bano_msg_t* bano_msg)
{
  BANO_PERROR();
  return -1;
}

static int do_write(void* p, uint32_t addr, const bano_msg_t* m)
{
  BANO_PERROR();
  return -1;
}

static int do_ctl(void* p, unsigned int k, unsigned int v)
{
  BANO_PERROR();
  return -1;
}

static int do_close(void* p)
{
  free(p);
  return 0;
}

static int do_get_fd(void* p)
{
  return ((socket_httpd_t*)p)->httpd->msg_pipe[0];
}


int bano_socket_httpd_open(bano_socket_t* socket, bano_httpd_t* httpd)
{
  socket_httpd_t* const hs = malloc(sizeof(socket_httpd_t));
  if (hs == NULL)
  {
    BANO_PERROR();
    return -1;
  }

  hs->httpd = httpd;

  socket->read_fn = do_read;
  socket->peek_fn = do_peek;
  socket->write_fn = do_write;
  socket->ctl_fn = do_ctl;
  socket->close_fn = do_close;
  socket->get_fd_fn = do_get_fd;
  socket->data = hs;

  return 0;
}
