#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include "bano_base.h"
#include "bano_socket.h"
#include "bano_timer.h"
#include "bano_list.h"
#include "bano_perror.h"
#include "../common/bano_common.h"


/* conversion routines */

static inline uint16_t uint16_to_le(uint16_t x)
{
  return x;
}

static inline uint16_t le_to_uint16(uint16_t x)
{
  return x;
}

static inline uint32_t uint32_to_le(uint32_t x)
{
  return x;
}

static inline uint32_t le_to_uint32(uint32_t x)
{
  return x;
}

/* io related routines */

static void free_io(bano_io_t* io)
{
  free(io);
}

static int free_io_item(bano_list_item_t* li, void* p)
{
  bano_io_t* const io = li->data;
  free_io(io);
  return 0;
}

/* node related routines */

static bano_node_t* alloc_node(void)
{
  bano_node_t* const node = malloc(sizeof(bano_node_t));
  if (node == NULL) return NULL;
  bano_list_init(&node->posted_ios);
  bano_list_init(&node->pending_ios);
  return node;
}

static void free_node(bano_node_t* node)
{
  bano_list_fini(&node->posted_ios, free_io_item, NULL);
  bano_list_fini(&node->pending_ios, free_io_item, NULL);
  free(node);
}

static int free_node_item(bano_list_item_t* li, void* p)
{
  bano_node_t* const node = li->data;
  free_node(node);
  return 0;
}

/* socket related routines */

static bano_socket_t* alloc_socket(const bano_socket_info_t* si)
{
  bano_socket_t* const s = malloc(sizeof(bano_socket_t));

  if (s == NULL) return NULL;

  bano_socket_init(s);

  switch (si->type)
  {
  case BANO_SOCKET_TYPE_SNRF:
    {
      if (bano_socket_snrf_open(s, &si->u.snrf))
	goto on_error;
      break ;
    }

  default:
    {
    on_error:
      BANO_PERROR();
      free(s);
      return NULL;
      break ;
    }
  }

  return s;
}

static void free_socket(bano_socket_t* socket, bano_base_t* base)
{
  bano_list_foreach(&base->nodes, free_node_item, base);
  bano_socket_close(socket);
  free(socket);
}

static int free_socket_item(bano_list_item_t* li, void* p)
{
  bano_socket_t* const socket = li->data;
  bano_base_t* const base = p;
  free_socket(socket, base);
  return 0;
}


/* exported */

static void on_sigint(int x)
{
}

int bano_init(void)
{
  signal(SIGINT, on_sigint);
  return 0;
}

int bano_fini(void)
{
  return 0;
}

int bano_open(bano_base_t* base, const bano_base_info_t* info)
{
  bano_list_init(&base->nodes);
  bano_list_init(&base->sockets);
  bano_timer_init(&base->timers);
  return 0;
}

int bano_close(bano_base_t* base)
{
  bano_timer_fini(&base->timers);
  bano_list_fini(&base->nodes, free_node_item, NULL);
  bano_list_fini(&base->sockets, free_socket_item, base);
  return 0;
}

int bano_add_socket(bano_base_t* base, const bano_socket_info_t* si)
{
  bano_socket_t* s;

  s = alloc_socket(si);
  if (s == NULL)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  /* setup the socket with sinfo */

  if (bano_list_add_tail(&base->sockets, s))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  return 0;

 on_error_1:
  free_socket(s, base);
 on_error_0:
  return -1;
}

int bano_add_node(bano_base_t* base, uint32_t addr)
{
  /* NOTE: node is added to the first socket */

  bano_node_t* node;

  if (base->sockets.head == NULL)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  node = alloc_node();
  if (node == NULL)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  node->addr = addr;
  node->socket = base->sockets.head->data;

  if (bano_list_add_tail(&base->nodes, node))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  return 0;

 on_error_1:
  free_node(node);
 on_error_0:
  return -1;
}

int bano_post_io(bano_base_t* base, bano_node_t* node, bano_io_t* io)
{
  if (bano_list_add_tail(&node->posted_ios, io))
  {
    BANO_PERROR();
    return -1;
  }

  io->node = node;

  return 0;
}

/* event loop, peek pending messages  */

/* peek, read or write msg associated data */
typedef struct prw_msg_data
{
  bano_base_t* base;
  const bano_loop_info_t* linfo;
  fd_set* fds;
} prw_msg_data_t;

static int cmp_pending_io(bano_list_item_t* li, void* p)
{
  const bano_io_t* const pending_io = li->data;
  const bano_msg_t* pend_msg = &pending_io->msg;
  const bano_msg_t* answ_msg = (const bano_msg_t*)((uintptr_t*)p)[0];
  const bano_node_t* node = (const bano_node_t*)((uintptr_t*)p)[1];
  uint16_t pend_key;
  uint16_t answ_key;

  if (node->addr != answ_msg->hdr.saddr) return 0;

  if (pend_msg->hdr.op == BANO_OP_SET)
  {
    pend_key = pend_msg->u.set.key;
    answ_key = answ_msg->u.set.key;
  }
  else
  {
    pend_key = pend_msg->u.get.key;
    answ_key = answ_msg->u.get.key;
  }

  if (pend_key != answ_key) return 0;

  /* key found */
  ((uintptr_t*)p)[2] = (uintptr_t)li;
  return -1;
}

static bano_io_t* find_pending_io(bano_node_t* node, const bano_msg_t* answ_msg)
{
  uintptr_t data[3];
  bano_list_item_t* it;
  bano_io_t* io;

  data[0] = (uintptr_t)answ_msg;
  data[1] = (uintptr_t)node;
  data[2] = (uintptr_t)NULL;
  bano_list_foreach(&node->pending_ios, cmp_pending_io, data);

  it = (bano_list_item_t*)data[2];

  /* not found */
  if (it == NULL) return NULL;

  /* free list item */
  io = it->data;

  bano_list_del(&node->pending_ios, it);

  return io;
}

static int handle_set_msg
(prw_msg_data_t* prwmd, const bano_msg_t* msg, bano_node_t* node)
{
  if (msg->hdr.flags & BANO_FLAG_REPLY)
  {
    /* this is a reply, check pending messages */

    bano_io_t* const io = find_pending_io(node, msg);
    if (io != NULL)
    {
      if (io->compl_fn != NULL)
      {
	if ((msg->hdr.flags & BANO_FLAG_ERR) == 0)
	{
	  io->compl_err = 0;
	  io->compl_val = le_to_uint32(msg->u.set.val);
	}
	else
	{
	  io->compl_err = BANO_IO_ERR_FAILURE;
	}
	io->compl_fn(io, io->compl_data);
      }

      bano_timer_del(&prwmd->base->timers, io->timer);
      free_io(io);
    }
  }
  else if (prwmd->linfo->set_fn != NULL)
  {
    /* standard set message */

    bano_io_t io;
    io.msg.u.set.key = msg->u.set.key;
    io.msg.u.set.val = msg->u.set.val;
    prwmd->linfo->set_fn(prwmd->linfo->user_data, node, &io);
  }

  return 0;
}

static int handle_get_msg
(prw_msg_data_t* prwmd, const bano_msg_t* msg, bano_node_t* node)
{
  bano_io_t io;

  if (prwmd->linfo->get_fn != NULL)
  {
    io.msg.u.get.key = msg->u.get.key;
    prwmd->linfo->get_fn(prwmd->linfo->user_data, node, &io);
  }

  return 0;
}

static int cmp_node_addr(bano_list_item_t* li, void* p)
{
  const bano_node_t* const node = li->data;
  const uint32_t addr = (uint32_t)((uintptr_t*)p)[0];

  if (node->addr != addr) return 0;

  /* node found */
  ((uintptr_t*)p)[1] = (uintptr_t)node;
  return -1;
}

static bano_node_t* find_node_by_addr(bano_list_t* nodes, uint32_t addr)
{
  uintptr_t data[2];

  data[0] = (uintptr_t)addr;
  data[1] = (uintptr_t)NULL;
  bano_list_foreach(nodes, cmp_node_addr, data);

  return (bano_node_t*)data[1];
}

static int handle_msg
(prw_msg_data_t* prwmd, bano_socket_t* socket, bano_msg_t* msg)
{
  const bano_loop_info_t* const linfo = prwmd->linfo;
  uint32_t saddr;
  bano_node_t* node;
  int err = 0;

  bano_cipher_dec((uint8_t*)msg);

  saddr = le_to_uint32(msg->hdr.saddr);
  node = find_node_by_addr(&prwmd->base->nodes, saddr);
  if (node == NULL)
  {
    /* new node */
    node = alloc_node();
    if (node == NULL)
    {
      BANO_PERROR();
      return -1;
    }

    node->addr = saddr;
    node->socket = socket;

    if (bano_list_add_tail(&prwmd->base->nodes, node))
    {
      BANO_PERROR();
      free_node(node);
      return -1;
    }

    if (linfo->node_fn != NULL)
    {
      linfo->node_fn(linfo->user_data, node, BANO_NODE_REASON_NEW);
    }
  }

  switch (msg->hdr.op)
  {
  case BANO_OP_SET:
    err = handle_set_msg(prwmd, msg, node);
    break ;

  case BANO_OP_GET:
    err = handle_get_msg(prwmd, msg, node);
    break ;

  default:
    /* not a fatal error */
    BANO_PERROR();
    err = 0;
    break ;
  }

  return err;
}

/* event loop, process pending messages */

static int peek_msg(bano_list_item_t* li, void* p)
{
  bano_socket_t* const socket = li->data;
  struct prw_msg_data* const prwmd = p;
  bano_msg_t msg;

  while (bano_socket_peek(socket, &msg) == 0)
  {
    handle_msg(prwmd, socket, &msg);
  }

  return 0;
}

/* event loop, read and process messages */

static int read_msg(bano_list_item_t* li, void* p)
{
  bano_socket_t* const socket = li->data;
  const int fd = bano_socket_get_fd(socket);
  struct prw_msg_data* const prwmd = p;
  int err;
  bano_msg_t msg;

  if (FD_ISSET(fd, prwmd->fds) == 0) return 0;

  /* TODO: use while loop instead of if. doing so */
  /* TODO: requires to retrieve the exact errno */
  /* TODO: code from sub layer and differentiate */
  /* TODO: between end of file, wouldblock ... */
  /* TODO: for now, assume closed on error */

  err = bano_socket_read(socket, &msg);
  if (err == -2)
  {
    /* msg partially filled, redo select */
    return 0;
  }

  if (err)
  {
    BANO_PERROR();
    free_socket(socket, prwmd->base);
    bano_list_del(&prwmd->base->sockets, li);
    return 0;
  }

  handle_msg(prwmd, socket, &msg);

  return 0;
}

/* event loop, write posted messages */

struct post_io_data
{
  bano_base_t* base;
  bano_node_t* node;
  int err;
};

static int post_io(bano_list_item_t* li, void* p)
{
  bano_io_t* const io = li->data;
  struct post_io_data* const pid = p;
  bano_msg_t enc_msg;

  memcpy(&enc_msg, &io->msg, sizeof(enc_msg));
  bano_cipher_enc((uint8_t*)&enc_msg);

  if (bano_socket_write(pid->node->socket, pid->node->addr, &enc_msg))
  {
    BANO_PERROR();
    free_socket(pid->node->socket, pid->base);
    bano_list_del(&pid->base->sockets, li);
    return 0;
  }

  if (io->flags & BANO_IO_FLAG_REPLY)
  {
    /* move in pending io list */

    bano_list_add_tail(&pid->node->pending_ios, io);

    if (bano_timer_add(&pid->base->timers, &io->timer, io->retry_ms))
    {
      BANO_PERROR();
      return 0;
    }

    io->timer->data[0] = &pid->node->pending_ios;
    io->timer->data[1] = pid->node->pending_ios.tail;
  }
  else
  {
    /* destroy the io */
    free_io(io);
  }

  /* destroy list item */
  bano_list_del(&pid->node->posted_ios, li);

  return 0;
}

static int write_msg(bano_list_item_t* li, void* p)
{
  /* write the node posted messages */

  bano_node_t* const node = li->data;
  const int fd = bano_socket_get_fd(node->socket);
  struct prw_msg_data* const prwmd = p;
  struct post_io_data pid;

  if (FD_ISSET(fd, prwmd->fds) == 0) return 0;

  pid.base = prwmd->base;
  pid.node = node;
  pid.err = 0;
  bano_list_foreach(&node->posted_ios, post_io, &pid);

  /* some nodes have been deleted from the list */
  /* stop the iteration, to avoid using dangling */
  /* pointers. any posted message will be written */
  /* during the next iteration */

  if (pid.err) return -1;

  return 0;
}

/* event loop, fill select read set info */

struct fill_sets_data
{
  fd_set rset;
  fd_set wset;
  int nfd;
};

static int fill_rset(bano_list_item_t* li, void* p)
{
  bano_socket_t* const s = li->data;
  struct fill_sets_data* const fsd = p;
  const int fd = bano_socket_get_fd(s);

  FD_SET(fd, &fsd->rset);
  if ((fd + 1) > fsd->nfd) fsd->nfd = fd + 1;

  return 0;
}

static int fill_wset(bano_list_item_t* li, void* p)
{
  bano_node_t* const node = li->data;
  struct fill_sets_data* const fsd = p;
  const int fd = bano_socket_get_fd(node->socket);

  if (bano_list_is_empty(&node->posted_ios)) return 0;

  FD_SET(fd, &fsd->wset);
  if ((fd + 1) > fsd->nfd) fsd->nfd = fd + 1;

  return 0;
}

static int do_new_node(bano_list_item_t* li, void* p)
{
  const bano_loop_info_t* const linfo = p;
  linfo->node_fn(linfo->user_data, li->data, BANO_NODE_REASON_NEW);
  return 0;
}

int bano_start_loop(bano_base_t* base, const bano_loop_info_t* linfo)
{
  struct fill_sets_data fsd;
  struct prw_msg_data prwmd;
  struct timeval timer_tv;
  struct timeval* timer_tvp;
  bano_timer_t* timer;
  bano_timer_t* loop_timer;
  int err;

  if (bano_cipher_init(linfo->cipher_alg, linfo->cipher_key))
  {
    BANO_PERROR();
    goto on_loop_done;
  }

  /* process new nodes */
  if (linfo->node_fn != NULL)
  {
    bano_list_foreach(&base->nodes, do_new_node, (void*)linfo);
  }

  loop_timer = NULL;

  while (1)
  {
    /* insert the loop timer if not already */

    if ((loop_timer == NULL) && (linfo->timer_fn != NULL))
    {
      if (bano_timer_add(&base->timers, &loop_timer, linfo->timer_ms))
      {
	BANO_PERROR();
	err = -1;
	goto on_loop_done;
      }
    }

    /* process pending messages */

    prwmd.base = base;
    prwmd.linfo = linfo;
    prwmd.fds = NULL;
    bano_list_foreach(&base->sockets, peek_msg, &prwmd);

    /* load select fd sets and process msgs */

    fsd.nfd = 0;

    FD_ZERO(&fsd.rset);
    bano_list_foreach(&base->sockets, fill_rset, &fsd);

    FD_ZERO(&fsd.wset);
    bano_list_foreach(&base->nodes, fill_wset, &fsd);

    timer_tvp = &timer_tv;
    if (bano_timer_get_next(&base->timers, &timer, &timer_tv))
    {
      /* no timer */
      timer_tvp = NULL;
    }

    if (fsd.nfd || (timer_tvp != NULL))
    {
      err = select(fsd.nfd, &fsd.rset, &fsd.wset, NULL, timer_tvp);
      if (err < 0)
      {
	BANO_PERROR();
	err = -1;
	goto on_loop_done;
      }

      if (err == 0)
      {
	if (timer != loop_timer)
	{
	  /* pending io timer */

	  bano_list_t* const li = timer->data[0];
	  bano_list_item_t* const it = timer->data[1];
	  bano_io_t* const io = it->data;

	  if ((io->retry_count--) == 0)
	  {
	    /* complete and destroy */
	    if (io->compl_fn != NULL)
	    {
	    do_complete_io:
	      io->compl_err = BANO_IO_ERR_TIMEOUT;
	      io->compl_fn(io, io->compl_data);
	    }
	    free_io(io);
	  }
	  else
	  {
	    /* repost */

	    printf("repost: %u\n", io->retry_count);

	    if (bano_list_add_tail(&io->node->posted_ios, io))
	    {
	      BANO_PERROR();
	      goto do_complete_io;
	    }
	  }

	  /* delete from list */
	  bano_list_del(li, it);
	}
	else
	{
	  linfo->timer_fn(linfo->user_data);
	  loop_timer = NULL;
	}

	bano_timer_del(&base->timers, timer);
      }

      /* fds in set ready */
      if (err > 0)
      {
	/* socket ready to read */
	prwmd.base = base;
	prwmd.linfo = linfo;
	prwmd.fds = &fsd.rset;
	bano_list_foreach(&base->sockets, read_msg, &prwmd);

	/* write messages */
	prwmd.base = base;
	prwmd.linfo = linfo;
	prwmd.fds = &fsd.wset;
	bano_list_foreach(&base->nodes, write_msg, &prwmd);
      }
    }
  }

 on_loop_done:
  bano_cipher_fini();
  return err;
}


/* io routines */

static void bano_init_common_io(bano_io_t* io, bano_compl_fn_t fn, void* data)
{
  io->flags = 0;
  io->compl_fn = fn;
  io->compl_data = data;
}

bano_io_t* bano_alloc_get_io
(uint16_t key, bano_compl_fn_t fn, void* data)
{
  bano_io_t* const io = malloc(sizeof(bano_io_t));

  if (io == NULL)
  {
    BANO_PERROR();
    return NULL;
  }

  bano_init_common_io(io, fn, data);

    /* default values: wait for 2s, retry 3 times */
  io->flags |= BANO_IO_FLAG_REPLY;
  io->retry_ms = 2000;
  io->retry_count = 3;

  io->msg.hdr.op = BANO_OP_GET;
  io->msg.hdr.flags = 0;
  io->msg.hdr.saddr = 0;
  io->msg.u.get.key = key;

  return io;
}

bano_io_t* bano_alloc_set_io
(
 uint16_t key, uint32_t val,
 unsigned int is_ack,
 bano_compl_fn_t fn, void* data
)
{
  bano_io_t* const io = malloc(sizeof(bano_io_t));

  if (io == NULL)
  {
    BANO_PERROR();
    return NULL;
  }

  bano_init_common_io(io, fn, data);

  if (is_ack)
  {
    /* default values: wait for 2s, retry 3 times */
    io->flags |= BANO_IO_FLAG_REPLY;
    io->retry_ms = 2000;
    io->retry_count = 3;
  }

  io->msg.hdr.op = BANO_OP_SET;
  io->msg.hdr.flags = 0;
  io->msg.hdr.saddr = 0;
  io->msg.u.set.key = key;
  io->msg.u.set.val = val;

  return io;
}

void bano_free_io(bano_io_t* io)
{
  free_io(io);
}
