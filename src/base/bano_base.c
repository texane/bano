#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include "bano_base.h"
#include "bano_node.h"
#include "bano_socket.h"
#include "bano_timer.h"
#include "bano_list.h"
#include "bano_dict.h"
#ifdef BANO_CONFIG_HTTPD
#include "bano_httpd.h"
#endif /* BANO_CONFIG_HTTPD */
#include "bano_parser.h"
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

/* list item freeing routines */

static int free_node_item(bano_list_item_t* li, void* p)
{
  bano_node_t* const node = li->data;
  bano_node_free(node);
  return 0;
}

static int free_socket_item(bano_list_item_t* li, void* p)
{
  bano_socket_t* const socket = li->data;
  bano_base_t* const base = p;
  bano_socket_free(socket, base);
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

struct apply_data
{
  bano_base_t* base;
  bano_parser_t* parser;
  int err;

  union
  {
    bano_socket_info_t socket_info;
    bano_node_info_t node_info;
#ifdef BANO_CONFIG_HTTPD
    bano_httpd_info_t httpd_info;
#endif /* BANO_CONFIG_HTTPD */
  } u;

};

static int apply_base_pair(bano_list_item_t* it, void* p)
{
  const bano_parser_pair_t* const pair = it->data;
  struct apply_data* const ad = p;
  bano_base_t* const base = ad->base;

  if (bano_string_cmp_cstr(&pair->key, "addr") == 0)
  {
    if (bano_string_cmp_cstr(&pair->val, "default") == 0)
    {
      base->addr = BANO_DEFAULT_BASE_ADDR;
    }
    else if (bano_string_to_uint32(&pair->val, &base->addr))
    {
      BANO_PERROR();
      ad->err = -1;
    }
  }
  else if (bano_string_cmp_cstr(&pair->key, "cipher_alg") == 0)
  {
    if (bano_string_cmp_cstr(&pair->val, "none") == 0)
    {
      base->cipher.alg = BANO_CIPHER_ALG_NONE;
    }
    else if (bano_string_cmp_cstr(&pair->val, "xtea") == 0)
    {
      base->cipher.alg = BANO_CIPHER_ALG_XTEA;
    }
    else
    {
      BANO_PERROR();
      ad->err = -1;
    }
  }
  else if (bano_string_cmp_cstr(&pair->key, "cipher_key") == 0)
  {
    if (bano_string_to_cipher_key(&pair->val, base->cipher.key))
    {
      BANO_PERROR();
      ad->err = -1;
    }
  }

  return ad->err;
}

static int apply_socket_pair(bano_list_item_t* it, void* p)
{
  const bano_parser_pair_t* const pair = it->data;
  struct apply_data* const ad = p;
  bano_socket_info_t* const sinfo = &ad->u.socket_info;
  bano_parser_t* const parser = ad->parser;

  if (bano_string_cmp_cstr(&pair->key, "type") == 0)
  {
    if (bano_string_cmp_cstr(&pair->val, "snrf") == 0)
    {
      sinfo->type = BANO_SOCKET_TYPE_SNRF;
      sinfo->u.snrf.dev_path = "/dev/ttyUSB0";
      sinfo->u.snrf.addr_width = 4;
      sinfo->u.snrf.addr_val = ad->base->addr;
    }
  }
  else if (bano_string_cmp_cstr(&pair->key, "dev_path") == 0)
  {
    if (sinfo->type == BANO_SOCKET_TYPE_SNRF)
    {
      if (bano_parser_add_cstr(parser, &pair->val, &sinfo->u.snrf.dev_path))
      {
	BANO_PERROR();
	ad->err = -1;
      }
    }
  }
  else
  {
    BANO_PERROR();
    ad->err = -1;
  }

  return ad->err;
}

static int apply_node_pair(bano_list_item_t* it, void* p)
{
  const bano_parser_pair_t* const pair = it->data;
  struct apply_data* const ad = p;
  bano_node_info_t* const ninfo = &ad->u.node_info;

  if (bano_string_cmp_cstr(&pair->key, "addr") == 0)
  {
    ninfo->flags |= BANO_NODE_FLAG_ADDR;
    if (bano_string_to_uint32(&pair->val, &ninfo->addr))
    {
      BANO_PERROR();
      goto on_error;
    }
  }
  else if (bano_string_cmp_cstr(&pair->key, "seed") == 0)
  {
    ninfo->flags |= BANO_NODE_FLAG_SEED;
    if (bano_string_to_uint32(&pair->val, &ninfo->seed))
    {
      BANO_PERROR();
      goto on_error;
    }
  }
  else if (bano_string_cmp_cstr(&pair->key, "nodl_id") == 0)
  {
    if (bano_string_to_uint32(&pair->val, &ninfo->nodl_id))
    {
      BANO_PERROR();
      goto on_error;
    }
  }
  else if (bano_string_cmp_cstr(&pair->key, "cipher") == 0)
  {
    unsigned int is_true;
    if (bano_string_to_bool(&pair->val, &is_true))
    {
      BANO_PERROR();
      goto on_error;
    }
    if (is_true) ninfo->flags |= BANO_NODE_FLAG_CIPHER;
    else ninfo->flags &= ~BANO_NODE_FLAG_CIPHER;
  }
  else
  {
    BANO_PERROR();
    goto on_error;
  }

  return 0;

 on_error:
  ad->err = -1;
  return -1;
}

#ifdef BANO_CONFIG_HTTPD
static int apply_httpd_pair(bano_list_item_t* it, void* p)
{
  const bano_parser_pair_t* const pair = it->data;
  struct apply_data* const ad = p;
  bano_httpd_info_t* const info = &ad->u.httpd_info;

  if (bano_string_cmp_cstr(&pair->key, "port") == 0)
  {
    info->flags |= BANO_HTTPD_FLAG_PORT;
    if (bano_string_to_uint16(&pair->val, &info->port))
    {
      BANO_PERROR();
      goto on_error;
    }
  }
  else
  {
    BANO_PERROR();
    goto on_error;
  }

  return 0;

 on_error:
  ad->err = -1;
  return -1;
}
#endif /* BANO_CONFIG_HTTPD */

static int apply_struct(bano_list_item_t* it, void* p)
{
  bano_parser_struct_t* const strukt = it->data;
  struct apply_data* const ad = p;

  if (bano_string_cmp_cstr(&strukt->name, "base") == 0)
  {
    bano_parser_foreach_pair(strukt, apply_base_pair, ad);
    if (ad->err) goto on_error;
  }
  else if (bano_string_cmp_cstr(&strukt->name, "socket") == 0)
  {
    bano_socket_info_t* const sinfo = &ad->u.socket_info;

    bano_socket_init_info(sinfo);
    bano_parser_foreach_pair(strukt, apply_socket_pair, ad);

    if (ad->err) goto on_error;

    if (bano_add_socket(ad->base, sinfo))
    {
      BANO_PERROR();
      ad->err = -1;
      goto on_error;
    }
  }
  else if (bano_string_cmp_cstr(&strukt->name, "node") == 0)
  {
    bano_node_info_t* const ninfo = &ad->u.node_info;

    if (ad->base->sockets.head == NULL)
    {
      BANO_PERROR();
      ad->err = -1;
      goto on_error;
    }

    bano_init_node_info(ninfo);
    ninfo->flags |= BANO_NODE_FLAG_SOCKET;
    ninfo->socket = ad->base->sockets.head->data;
    bano_parser_foreach_pair(strukt, apply_node_pair, ad);

    if (ad->err) goto on_error;

    if (bano_add_node(ad->base, ninfo))
    {
      BANO_PERROR();
      ad->err = -1;
      goto on_error;
    }
  }
#ifdef BANO_CONFIG_HTTPD
  else if (bano_string_cmp_cstr(&strukt->name, "http_server") == 0)
  {
    bano_httpd_info_t* const info = &ad->u.httpd_info;

    bano_init_httpd_info(info, ad->base);
    bano_parser_foreach_pair(strukt, apply_httpd_pair, ad);

    if (ad->err) goto on_error;

    if (bano_httpd_init(&ad->base->httpd, info))
    {
      BANO_PERROR();
      ad->err = -1;
      goto on_error;
    }

    ad->base->is_httpd = 1;
  }
#endif /* BANO_CONFIG_HTTPD */

 on_error:
  return ad->err;
}

static int apply_conf(bano_base_t* base, const char* conf_path)
{
  bano_parser_t parser;
  struct apply_data ad;

  if (bano_parser_load_file(&parser, conf_path))
  {
    BANO_PERROR();
    return -1;
  }

  ad.base = base;
  ad.parser = &parser;
  ad.err = 0;
  bano_parser_foreach_struct(&parser, apply_struct, &ad);

  bano_parser_fini(&parser);

  return ad.err;
}

int bano_open(bano_base_t* base, const bano_base_info_t* info)
{
#ifdef BANO_CONFIG_HTTPD
  base->is_httpd = 0;
#endif /* BANO_CONFIG_HTTPD */

  bano_list_init(&base->nodes);
  bano_list_init(&base->sockets);
  bano_timer_init(&base->timers);

  bano_cipher_init(&base->cipher, &bano_cipher_info_none);

  if (info->flags & BANO_BASE_FLAG_ADDR) base->addr = info->addr;
  else base->addr = BANO_DEFAULT_BASE_ADDR;

  if (info->flags & BANO_BASE_FLAG_CONF)
  {
    if (apply_conf(base, info->conf_path))
    {
      BANO_PERROR();
      goto on_error_0;
    }
  }

  return 0;

 on_error_0:
#ifdef BANO_CONFIG_HTTPD
  if (base->is_httpd) bano_httpd_fini(&base->httpd);
#endif /* BANO_CONFIG_HTTPD */
  bano_list_fini(&base->nodes, free_node_item, NULL);
  bano_list_fini(&base->sockets, free_socket_item, NULL);
  bano_timer_fini(&base->timers);
  return -1;
}

int bano_close(bano_base_t* base)
{
#ifdef BANO_CONFIG_HTTPD
  if (base->is_httpd) bano_httpd_fini(&base->httpd);
#endif /* BANO_CONFIG_HTTPD */
  bano_timer_fini(&base->timers);
  bano_list_fini(&base->nodes, free_node_item, NULL);
  bano_list_fini(&base->sockets, free_socket_item, base);
  return 0;
}

int bano_add_socket(bano_base_t* base, const bano_socket_info_t* si)
{
  bano_socket_t* s;

  if (bano_socket_alloc(&s, si))
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
  bano_socket_free(s, base);
 on_error_0:
  return -1;
}

int bano_add_node(bano_base_t* base, const bano_node_info_t* info)
{
  bano_node_t* node;

  if ((info->flags & BANO_NODE_FLAG_SOCKET) == 0)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  if ((info->flags & BANO_NODE_FLAG_ADDR) == 0)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  if (bano_node_alloc(&node))
  {
    BANO_PERROR();
    goto on_error_0;
  }

  if (info->flags & BANO_NODE_FLAG_CIPHER)
  {
    node->flags |= BANO_NODE_FLAG_CIPHER;
  }

  node->addr = info->addr;
  node->socket = info->socket;

  if (bano_list_add_tail(&base->nodes, node))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  return 0;

 on_error_1:
  bano_node_free(node);
 on_error_0:
  return -1;
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

int bano_find_node_by_addr
(bano_base_t* base, uint32_t addr, bano_node_t** nodep)
{
  *nodep = find_node_by_addr(&base->nodes, addr);
  return *nodep == NULL ? -1 : 0;
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

  if (pend_msg->hdr.op == BANO_MSG_OP_SET)
  {
    /* not the value asked for */
    if (pend_msg->u.set.val != answ_msg->u.set.val)
      return 0;

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
  const uint16_t key = le_to_uint16(msg->u.set.key);
  const uint32_t val = le_to_uint32(msg->u.set.val);

  if (msg->hdr.flags & BANO_MSG_FLAG_REPLY)
  {
    /* this is a reply, check pending messages */

    bano_io_t* const io = find_pending_io(node, msg);
    if (io != NULL)
    {
      if (io->compl_fn != NULL)
      {
	if ((msg->hdr.flags & BANO_MSG_FLAG_ERR) == 0)
	{
	  io->compl_err = 0;
	  io->compl_val = val;
	}
	else
	{
	  io->compl_err = BANO_IO_ERR_FAILURE;
	}

	io->compl_fn(io, io->compl_data);
      }

      bano_timer_del(&prwmd->base->timers, io->timer);
      bano_free_io(io);
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

  /* add to node pairs */

  if ((msg->hdr.flags & BANO_MSG_FLAG_ERR) == 0)
  {
    if (bano_dict_set_or_add(&node->keyval_pairs, key, val))
    {
      BANO_PERROR();
    }
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

typedef int (*msg_handler_t)(prw_msg_data_t*, bano_socket_t*, void*);

static int handle_bano_msg
(prw_msg_data_t* prwmd, bano_socket_t* socket, void* m)
{
  const bano_loop_info_t* const linfo = prwmd->linfo;
  bano_msg_t* const msg = m;
  uint32_t saddr;
  bano_node_t* node;
  int err = 0;

  if (msg->hdr.flags & BANO_MSG_FLAG_ENC)
  {
    uint8_t* const p = ((uint8_t*)msg) + BANO_MSG_ENC_OFF;
    bano_cipher_dec(&prwmd->base->cipher, p);
  }

  saddr = le_to_uint32(msg->hdr.saddr);
  node = find_node_by_addr(&prwmd->base->nodes, saddr);
  if (node == NULL)
  {
    /* new node */
    if (bano_node_alloc(&node))
    {
      BANO_PERROR();
      return -1;
    }

    node->addr = saddr;
    node->socket = socket;

    if (bano_list_add_tail(&prwmd->base->nodes, node))
    {
      BANO_PERROR();
      bano_node_free(node);
      return -1;
    }

    if (linfo->node_fn != NULL)
    {
      linfo->node_fn(linfo->user_data, node, BANO_NODE_REASON_NEW);
    }
  }

  switch (msg->hdr.op)
  {
  case BANO_MSG_OP_SET:
    err = handle_set_msg(prwmd, msg, node);
    break ;

  case BANO_MSG_OP_GET:
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


#ifdef BANO_CONFIG_HTTPD

/* html generator */

struct gen_html_data
{
  char* buf;
  size_t size;
  size_t off;
};

static int gen_pair_html(bano_list_item_t* it, void* p)
{
  bano_dict_pair_t* const pair = it->data;
  struct gen_html_data* const ghd = p;

  size_t off = ghd->off;
  char* const buf = ghd->buf;
  const size_t size = ghd->size;

  const uint16_t key = (uint16_t)pair->key;
  const uint32_t val = (uint32_t)pair->val;

  off += snprintf(buf + off, size - off, "<li>");
  off += snprintf(buf + off, size - off, "%04x = %08x\n", key, val);
  off += snprintf(buf + off, size - off, "</li>\n");

  ghd->off = off;

  return 0;
}

static int gen_node_html(bano_list_item_t* it, void* p)
{
  bano_node_t* const node = it->data;
  struct gen_html_data* const ghd = p;
  size_t off = ghd->off;
  char* const buf = ghd->buf;
  const size_t size = ghd->size;

  off += snprintf(buf + off, size - off, "<li>\n");
  off += snprintf(buf + off, size - off, "0x%08x\n", bano_node_get_addr(node));
  off += snprintf(buf + off, size - off, "<ul>\n");

  ghd->off = off;
  bano_dict_foreach(&node->keyval_pairs, gen_pair_html, ghd);
  off = ghd->off;

  off += snprintf(buf + off, size - off, "</ul>\n");
  off += snprintf(buf + off, size - off, "</li>\n");

  ghd->off = off;
  
  return 0;
}

static size_t gen_base_html(bano_base_t* base, char* buf, size_t size)
{
  struct gen_html_data ghd;
  size_t off = 0;

  off += snprintf(buf + off, size - off, "<html><body>\n");

  /* node related html */

  off += snprintf(buf + off, size - off, "<ul>\n");
  ghd.buf = buf;
  ghd.size = size;
  ghd.off = off;
  bano_list_foreach(&base->nodes, gen_node_html, &ghd);
  off = ghd.off;
  off += snprintf(buf + off, size - off, "</ul>\n");

  off += snprintf(buf + off, size - off, "</body></html>\n");

  return off;
}


/* httpd message handler */

struct httpd_op_data
{
  bano_base_t* base;
  bano_httpd_msg_t* msg;
};

static void complete_httpd_msg
(bano_httpd_msg_t* msg, bano_base_t* base, int err)
{
  /* generate html and complete */

#define HTML_ERR_STR "<html><body> base error </body></html>"
  static const char* error_buf = HTML_ERR_STR;
  static const size_t error_size = sizeof(HTML_ERR_STR) - 1;

  char* buf;
  size_t size;

  size  = 8 * 1024;
  buf = malloc(size);

  if (buf == NULL)
  {
    BANO_PERROR();
    buf = (char*)error_buf;
    size = error_size;
    goto on_error;
  }

  size = gen_base_html(base, buf, size);

 on_error:
  bano_httpd_complete_msg(msg, 0, buf, size);
  if (buf != error_buf) free(buf);
}

static int on_httpd_io_compl(bano_io_t* io, void* p)
{
  struct httpd_op_data* const hod = p;
  int err;

  printf("%s\n", __FUNCTION__); fflush(stdout);

  if (io->compl_err != BANO_IO_ERR_SUCCESS)
  {
    BANO_PERROR();
    err = -1;
  }
  else
  {
    err = 0;
  }

  printf("%s %u\n", __FUNCTION__, __LINE__); fflush(stdout);

  complete_httpd_msg(hod->msg, hod->base, err);

  printf("%s %u\n", __FUNCTION__, __LINE__); fflush(stdout);

  free(hod);

  return 0;
}

static int handle_httpd_msg
(prw_msg_data_t* prwmd, bano_socket_t* socket, void* m)
{
  bano_httpd_msg_t* const msg = m;
  int err = 0;

  switch (msg->op)
  {
  case BANO_HTTPD_MSG_OP_GET:
  case BANO_HTTPD_MSG_OP_SET:
    {
      const uint32_t naddr = msg->naddr;
      const uint16_t key = msg->key;
      const uint32_t val = msg->val;
      const unsigned int is_ack = (unsigned int)msg->is_ack;

      struct httpd_op_data* hod;
      bano_node_t* node;
      bano_io_t* io;

      if (bano_find_node_by_addr(prwmd->base, naddr, &node))
      {
	BANO_PERROR();
	goto on_error_0;
      }

      hod = malloc(sizeof(struct httpd_op_data));
      if (hod == NULL)
      {
	BANO_PERROR();
	goto on_error_0;
      }

      hod->base = prwmd->base;
      hod->msg = msg;

      if (msg->op == BANO_HTTPD_MSG_OP_SET)
      {
	io = bano_alloc_set_io(key, val, is_ack, on_httpd_io_compl, hod);
      }
      else
      {
	io = bano_alloc_get_io(key, on_httpd_io_compl, hod);
      }

      if (io == NULL)
      {
	BANO_PERROR();
	goto on_error_1;
      }

      if (bano_post_io(prwmd->base, node, io))
      {
	BANO_PERROR();
	goto on_error_2;
      }

      break ;

    on_error_2:
      free(io);
    on_error_1:
      free(hod);
    on_error_0:
      err = -1;
      goto on_invalid_op;
      break ;
    }

  case BANO_HTTPD_MSG_OP_INVALID:
  default:
    {
    on_invalid_op:
      complete_httpd_msg(msg, prwmd->base, err);
      break ;
    }
  }

  return 0;
}

#endif /* BANO_CONFIG_HTTPD */


/* event loop, process pending messages */

static int peek_msg(bano_list_item_t* li, void* p)
{
  bano_socket_t* const socket = li->data;
  struct prw_msg_data* const prwmd = p;
#ifdef BANO_CONFIG_HTTPD
  bano_httpd_msg_t httpd_msg;
#endif /* BANO_CONFIG_HTTPD */
  bano_msg_t bano_msg;
  void* msg;
  msg_handler_t handle_msg;

  if (socket->type == BANO_SOCKET_TYPE_SNRF)
  {
    msg = &bano_msg;
    handle_msg = handle_bano_msg;
  }
#ifdef BANO_CONFIG_HTTPD
  else if (socket->type == BANO_SOCKET_TYPE_HTTPD)
  {
    msg = &httpd_msg;
    handle_msg = handle_httpd_msg;
  }
#endif /* BANO_CONFIG_HTTPD */
  else
  {
    BANO_PERROR();
    return 0;
  }

  while (bano_socket_peek(socket, msg) == 0)
  {
    handle_msg(prwmd, socket, msg);
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
#ifdef BANO_CONFIG_HTTPD
  bano_httpd_msg_t httpd_msg;
#endif /* BANO_CONFIG_HTTPD */
  bano_msg_t bano_msg;
  void* msg;
  msg_handler_t handle_msg;

  if (FD_ISSET(fd, prwmd->fds) == 0) return 0;

  /* TODO: use while loop instead of if. doing so */
  /* TODO: requires to retrieve the exact errno */
  /* TODO: code from sub layer and differentiate */
  /* TODO: between end of file, wouldblock ... */
  /* TODO: for now, assume closed on error */

  if (socket->type == BANO_SOCKET_TYPE_SNRF)
  {
    msg = &bano_msg;
    handle_msg = handle_bano_msg;
  }
#ifdef BANO_CONFIG_HTTPD
  else if (socket->type == BANO_SOCKET_TYPE_HTTPD)
  {
    msg = &httpd_msg;
    handle_msg = handle_httpd_msg;
  }
#endif /* BANO_CONFIG_HTTPD */
  else
  {
    BANO_PERROR();
    return 0;
  }

  err = bano_socket_read(socket, msg);
  if (err == -2)
  {
    /* msg partially filled, redo select */
    return 0;
  }

  if (err)
  {
    BANO_PERROR();
    bano_socket_free(socket, prwmd->base);
    bano_list_del(&prwmd->base->sockets, li);
    return 0;
  }

  handle_msg(prwmd, socket, msg);

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

  if (pid->node->flags & BANO_NODE_FLAG_CIPHER)
  {
    uint8_t* const p = ((uint8_t*)&enc_msg) + BANO_MSG_ENC_OFF;
    bano_cipher_enc(&pid->base->cipher, p);
    enc_msg.hdr.flags |= BANO_MSG_FLAG_ENC;
  }

  if (bano_socket_write(pid->node->socket, pid->node->addr, &enc_msg))
  {
    BANO_PERROR();
    bano_socket_free(pid->node->socket, pid->base);
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
    bano_free_io(io);
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
	    bano_free_io(io);
	  }
	  else
	  {
	    /* repost */
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

  io->msg.hdr.op = BANO_MSG_OP_GET;
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

  io->msg.hdr.op = BANO_MSG_OP_SET;
  io->msg.hdr.flags = 0;
  io->msg.hdr.saddr = 0;
  io->msg.u.set.key = key;
  io->msg.u.set.val = val;

  return io;
}

void bano_free_io(bano_io_t* io)
{
  free(io);
}
