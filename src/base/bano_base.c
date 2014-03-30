#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include "bano_base.h"
#include "bano_socket.h"
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
  return 0;
}

int bano_close(bano_base_t* base)
{
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

int bano_post_io(bano_base_t* base, bano_node_t* node, bano_io_t* io)
{
  if (bano_list_add_tail(&node->posted_ios, io))
  {
    BANO_PERROR();
    return -1;
  }

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

static int cmp_io_keys(bano_list_item_t* li, void* p)
{
  const bano_io_t* const get_io = li->data;
  const uint16_t set_key = (const uint16_t)((uintptr_t*)p)[0];

  if (get_io->msg.u.get.key != set_key) return 0;

  /* key found */
  ((uintptr_t*)p)[1] = (uintptr_t)li;
  return -1;
}

static bano_io_t* find_get_io(bano_node_t* node, const bano_msg_set_t* set_msg)
{
  uintptr_t data[2];
  bano_list_item_t* li;
  bano_io_t* get_io;

  data[0] = (uintptr_t)set_msg->key;
  data[1] = (uintptr_t)NULL;
  bano_list_foreach(&node->pending_ios, cmp_io_keys, data);

  li = (bano_list_item_t*)data[1];

  /* not found */
  if (li == NULL) return NULL;

  /* free list item */
  get_io = li->data;

  bano_list_del(&node->pending_ios, li);

  return get_io;
}

static int handle_set_msg
(prw_msg_data_t* prwmd, const bano_msg_set_t* msg, bano_node_t* node)
{
  bano_io_t* get_io;

  get_io = find_get_io(node, msg);
  if (get_io != NULL)
  {
    /* reply to previous get message */

    if (get_io->compl_fn != NULL)
    {
      get_io->compl_err = 0;
      get_io->compl_val = msg->val;
      get_io->compl_fn(get_io, get_io->compl_data);
    }

    free_io(get_io);
  }
  else
  {
    /* standard set message */

    bano_io_t io;
    io.msg.u.set.key = msg->key;
    io.msg.u.set.val = msg->val;
    prwmd->linfo->set_fn(prwmd->linfo->user_data, node, &io);
  }

  return 0;
}

static int handle_get_msg
(prw_msg_data_t* prwmd, const bano_msg_get_t* msg, bano_node_t* node)
{
  bano_io_t io;

  io.msg.u.get.key = msg->key;
  prwmd->linfo->get_fn(prwmd->linfo->user_data, node, &io);

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
(prw_msg_data_t* prwmd, bano_socket_t* socket, const bano_msg_t* msg)
{
  const bano_loop_info_t* const linfo = prwmd->linfo;
  const uint32_t saddr = le_to_uint32(msg->hdr.saddr);
  bano_node_t* node;
  int err = 0;

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
    bano_list_init(&node->posted_ios);
    bano_list_init(&node->pending_ios);

    if (bano_list_add_tail(&prwmd->base->nodes, node))
    {
      BANO_PERROR();
      free_node(node);
      return -1;
    }

    if (linfo->flags & BANO_LOOP_FLAG_NODE)
    {
      linfo->node_fn(linfo->user_data, node, BANO_NODE_REASON_NEW);
    }
  }

  switch (msg->hdr.op)
  {
  case BANO_OP_SET:
    if ((linfo->flags & BANO_LOOP_FLAG_SET) == 0) goto unhandled_op;
    err = handle_set_msg(prwmd, &msg->u.set, node);
    break ;

  case BANO_OP_GET:
    if ((linfo->flags & BANO_LOOP_FLAG_GET) == 0) goto unhandled_op;
    err = handle_get_msg(prwmd, &msg->u.get, node);
    break ;

  default:
  unhandled_op:
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
  struct prw_msg_data* const prwmd = p;
  int err;
  bano_msg_t msg;

  if (FD_ISSET(bano_socket_get_fd(socket), prwmd->fds) == 0)
    return 0;

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

  if (bano_socket_write(pid->node->socket, pid->node->addr, &io->msg))
  {
    BANO_PERROR();
    free_socket(pid->node->socket, pid->base);
    bano_list_del(&pid->base->sockets, li);
    return 0;
  }

  if (io->msg.hdr.op == BANO_OP_GET)
  {
    /* move in pending io list */
    bano_list_add_tail(&pid->node->pending_ios, io);
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
  struct prw_msg_data* const prwmd = p;
  struct post_io_data pid;

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

static void ms_to_timeval(struct timeval* tv, unsigned int ms)
{
  tv->tv_sec = ms / 1000;
  tv->tv_usec = (ms % 1000) * 1000;
}

#define loop_flag1(__a) \
(BANO_LOOP_FLAG_ ## __a)
#define loop_flag2(__a, __b) \
(loop_flag1(__a) | loop_flag1(__b))
#define loop_flag3(__a, __b, __c) \
(loop_flag1(__a) | loop_flag1(__b) | loop_flag1(__c))

int bano_start_loop(bano_base_t* base, const bano_loop_info_t* linfo)
{
  struct fill_sets_data fsd;
  struct prw_msg_data prwmd;
  struct timeval tm_diff;
  struct timeval tm_start;
  struct timeval tm_stop;
  struct timeval tm_sel;
  struct timeval* tm_selp;
  struct timeval saved_tm_sel;
  int err;

  /* compute timer */
  tm_selp = NULL;
  if (linfo->flags & loop_flag1(TIMER))
  {
    ms_to_timeval(&saved_tm_sel, linfo->timer_ms);
    tm_sel = saved_tm_sel;
    tm_selp = &tm_sel;
  }

  while (1)
  {
    if (linfo->flags & loop_flag1(TIMER))
    {
      gettimeofday(&tm_start, NULL);
    }

    /* process pending messages */
    if (linfo->flags & loop_flag3(SET, GET, NODE))
    {
      /* process pending messages */
      prwmd.base = base;
      prwmd.linfo = linfo;
      prwmd.fds = NULL;
      bano_list_foreach(&base->sockets, peek_msg, &prwmd);
    }

    /* load select fd sets and process msgs */
    /* TODO: optimize by precomputing sets */

    fsd.nfd = 0;
    if (linfo->flags & loop_flag3(SET, GET, NODE))
    {
      FD_ZERO(&fsd.rset);
      bano_list_foreach(&base->sockets, fill_rset, &fsd);

      FD_ZERO(&fsd.wset);
      bano_list_foreach(&base->nodes, fill_wset, &fsd);
    }

    if (fsd.nfd || (tm_selp != NULL))
    {
      err = select(fsd.nfd, &fsd.rset, &fsd.wset, NULL, tm_selp);
      if (err < 0)
      {
	BANO_PERROR();
	err = -1;
	goto on_loop_done;
      }

      if ((err == 0) && (linfo->flags & loop_flag1(TIMER)))
      {
	goto on_timer;
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

    /* recompute timer */
    if (linfo->flags & loop_flag1(TIMER))
    {
      gettimeofday(&tm_stop, NULL);
      timersub(&tm_stop, &tm_start, &tm_diff);
      if (timercmp(&tm_sel, &tm_diff, <))
      {
      on_timer:
	if (linfo->timer_fn(linfo->user_data))
	{
	  err = 0;
	  goto on_loop_done;
	}
	/* reload select timer */
	tm_sel = saved_tm_sel;
      }
      else
      {
	/* update select timer */
	timersub(&tm_sel, &tm_diff, &tm_sel);
      }
    }
  }

 on_loop_done:
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
  io->msg.hdr.op = BANO_OP_GET;
  io->msg.hdr.flags = 0;
  io->msg.hdr.saddr = 0;
  io->msg.u.get.key = key;

  return io;
}

bano_io_t* bano_alloc_set_io
(uint16_t key, uint32_t val, bano_compl_fn_t fn, void* data)
{
  bano_io_t* const io = malloc(sizeof(bano_io_t));

  if (io == NULL)
  {
    BANO_PERROR();
    return NULL;
  }

  bano_init_common_io(io, fn, data);
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
