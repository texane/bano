#include <stdlib.h>
#include <unistd.h>
#include "bano_perror.h"
#include "bano_socket.h"
#include "bano_httpd.h"
#include "../common/bano_common.h"


typedef struct socket_httpd
{
  bano_httpd_t* httpd;
} socket_httpd_t;


static int do_read(void* p, void* m)
{
  socket_httpd_t* const socket = p;
  bano_httpd_msg_t* const msg = m;
  const int fd = socket->httpd->msg_pipe[0];

  if (read(fd, msg, sizeof(bano_httpd_msg_t)) != sizeof(bano_httpd_msg_t))
  {
    BANO_PERROR();
    return -1;
  }

  return 0;
}

static int do_peek(void* p, void* m)
{
  return 0;
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
  socket->close_fn = do_close;
  socket->get_fd_fn = do_get_fd;
  socket->data = hs;

  return 0;
}
