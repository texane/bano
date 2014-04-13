#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include "bano_base.h"
#include "bano_list.h"
#include "bano_socket.h"
#include "bano_perror.h"
#ifdef BANO_CONFIG_HTTPD
#include "bano_httpd.h"
#endif /* BANO_CONFIG_HTTPD */
#include "../common/bano_common.h"


/* static inline */

static int unimpl_read(void* p, void* m)
{
  BANO_PERROR();
  return -1;
}

static int unimpl_peek(void* p, void* m)
{
  BANO_PERROR();
  return -1;
}

static int unimpl_write(void* p, uint32_t x, const void* m)
{
  BANO_PERROR();
  return -1;
}

static int unimpl_ctl(void* p, unsigned int k, unsigned int v)
{
  BANO_PERROR();
  return -1;
}

static int unimpl_close(void* p)
{
  BANO_PERROR();
  return -1;
}

static int unimpl_get_fd(void* p)
{
  BANO_PERROR();
  return -1;
}

static inline int init_socket(bano_socket_t* s)
{
  s->type = BANO_SOCKET_TYPE_INVALID;
  s->read_fn = unimpl_read;
  s->peek_fn = unimpl_peek;
  s->write_fn = unimpl_write;
  s->ctl_fn = unimpl_ctl;
  s->close_fn = unimpl_close;
  s->get_fd_fn = unimpl_get_fd;
  s->data = NULL;
  return 0;
}

int bano_socket_alloc(bano_socket_t** sp, const bano_socket_info_t* info)
{
  *sp = malloc(sizeof(bano_socket_t));
  if (*sp == NULL)
  {
    BANO_PERROR();
    return -1;
  }

  init_socket(*sp);

  (*sp)->type = info->type;

  switch (info->type)
  {
  case BANO_SOCKET_TYPE_SNRF:
    {
      if (bano_socket_snrf_open(*sp, &info->u.snrf)) goto on_error;
      break ;
    }

#ifdef BANO_CONFIG_HTTPD
  case BANO_SOCKET_TYPE_HTTPD:
    {
      if (bano_socket_httpd_open(*sp, info->u.httpd)) goto on_error;
      break ;
    }
#endif /* BANO_CONFIG_HTTPD */

  default:
    {
    on_error:
      BANO_PERROR();
      free(*sp);
      return -1;
      break ;
    }
  }

  return 0;
}

static int free_node_item(bano_list_item_t* li, void* p)
{
  bano_node_t* const node = li->data;
  bano_node_free(node);
  return 0;
}

int bano_socket_free(bano_socket_t* socket, bano_base_t* base)
{
  bano_list_foreach(&base->nodes, free_node_item, base);
  bano_socket_close(socket);
  free(socket);
  return 0;
}
