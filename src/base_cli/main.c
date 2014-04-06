/* implement a command line base */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bano_base.h"
#include "bano_perror.h"


/* event handlers */

static int on_loop_timer(void* p)
{
  printf("%s\n", __FUNCTION__);
  return 0;
}

static int on_get(void* p, bano_node_t* node, bano_io_t* io)
{
  /* handle get messages */

  const uint16_t key = io->msg.u.get.key;

  printf("%s(0x%04x)\n", __FUNCTION__, key);
  bano_set_io_error(io, BANO_IO_ERR_UNIMPL);

  return 0;
}

static int on_set(void* p, bano_node_t* node, bano_io_t* io)
{
  /* handle set messages */

  const uint16_t key = io->msg.u.set.key;
  const uint32_t val = io->msg.u.set.val;

  printf("%s(0x%04x, 0x%08x)\n", __FUNCTION__, key, val);
  bano_set_io_error(io, BANO_IO_ERR_UNIMPL);

  return 0;
}


/* print node info */

static int on_node_print(void* p, bano_node_t* node, unsigned int reason)
{
  /* on new node, print node info */

  printf("-- node\n");
  printf(". reason: 0x%08x\n", reason);
  printf(". addr  : 0x%08x\n", bano_get_node_addr(node));
  printf("\n");

  return 0;
}


/* get a node key value */

struct node_get_data
{
  bano_base_t* base;
  uint32_t naddr;
  uint16_t key;
};

static int on_get_compl(bano_io_t* io, void* p)
{
  struct node_get_data* const ngd = p;
  const uint32_t naddr = ngd->naddr;
  const uint16_t key = io->msg.u.get.key;

  printf("%s: node{0x%08x, 0x%04x} == ", __FUNCTION__, naddr, key);

  if (io->compl_err == BANO_IO_ERR_SUCCESS) printf(" 0x%08x", io->compl_val);
  else printf(" error (0x%08x)", io->compl_err);
  printf("\n");

  return 0;
}

static int on_node_get(void* p, bano_node_t* node, unsigned int reason)
{
  /* on new node, send a get message */

  struct node_get_data* const ngd = p;
  bano_base_t* const base = ngd->base;
  const uint32_t naddr = ngd->naddr;
  const uint16_t key = ngd->key;

  bano_io_t* io;

  printf("%s\n", __FUNCTION__);

  if (bano_get_node_addr(node) != naddr)
  {
    BANO_PERROR();
    return 0;
  }

  if (reason != BANO_NODE_REASON_NEW)
  {
    BANO_PERROR();
    return -1;
  }

  /* setup and post io */

  io = bano_alloc_get_io(key, on_get_compl, ngd);
  if (io == NULL)
  {
    BANO_PERROR();
    return -1;
  }

  if (bano_post_io(base, node, io, 2000))
  {
    BANO_PERROR();
    return -1;
  }

  return 0;
}


/* set a node key value */

struct node_set_data
{
  bano_base_t* base;
  uint32_t naddr;
  uint16_t key;
  uint32_t val;
};

static int on_set_compl(bano_io_t* io, void* p)
{
  printf("%s\n", __FUNCTION__);

  switch (io->compl_err)
  {
  case BANO_IO_ERR_SUCCESS:
    printf("success\n");
    break ;

  default:
    printf("error: 0x%08x\n", io->compl_err);
    break ;
  }

  return 0;
}

static int on_node_set(void* p, bano_node_t* node, unsigned int reason)
{
  /* on new node, send a get message */

  struct node_set_data* const nsd = p;

  bano_base_t* const base = nsd->base;
  const uint32_t naddr = nsd->naddr;
  const uint16_t key = nsd->key;
  const uint32_t val = nsd->val;

  bano_io_t* io;

  printf("%s\n", __FUNCTION__);

  if (bano_get_node_addr(node) != naddr) return 0;

  if (reason != BANO_NODE_REASON_NEW)
  {
    BANO_PERROR();
    return -1;
  }

  /* setup and post io */

  io = bano_alloc_set_io(key, val, on_set_compl, nsd);
  if (io == NULL)
  {
    BANO_PERROR();
    return -1;
  }

  if (bano_post_io(base, node, io, 0))
  {
    BANO_PERROR();
    bano_free_io(io);
    return -1;
  }

  return 0;
}


/* cmdline */

typedef struct cmdline_info
{
  /* flags */
#define CMDLINE_FLAG_OP_NAME (1 << 0)
#define CMDLINE_FLAG_OP_KEY (1 << 1)
#define CMDLINE_FLAG_OP_VAL (1 << 2)
#define CMDLINE_FLAG_CIPHER_ALG (1 << 3)
#define CMDLINE_FLAG_CIPHER_KEY (1 << 4)
#define CMDLINE_FLAG_NODE_ADDR (1 << 5)
  uint32_t flags;
  /* op */
  const char* op_name;
  uint32_t op_key;
  uint32_t op_val;
  /* cipher */
  enum bano_cipher_alg cipher_alg;
  uint8_t cipher_key[BANO_CIPHER_KEY_SIZE];
  /* node */
  uint32_t node_addr;
} cmdline_info_t;

static uint32_t str_to_uint32(const char* s)
{
  int base = 10; 
  if ((strlen(s) > 2) && (s[0] == '0') && (s[1] == 'x'))
    base = 16;
  return (uint32_t)strtoul(s, NULL, base);
}

static void str_to_cipher_key(uint8_t* k, const char* s)
{
  size_t i;

  for (i = 0; i != BANO_CIPHER_KEY_SIZE; ++i)
  {
    k[i] = strtoul(s + i * 5, NULL, 16);
  }
}

static int get_cmdline_info(cmdline_info_t* ci, int ac, char** av)
{
  int i;

  if (ac & 1)
  {
    BANO_PERROR();
    return -1;
  }

  ci->flags = 0;
  ci->op_name = NULL;
  ci->op_key = 0;
  ci->op_val = 0;
  ci->cipher_alg = BANO_CIPHER_ALG_NONE;
  ci->node_addr = 0;

  for (i = 0; i != ac; i += 2)
  {
    const char* const k = av[i + 0];
    const char* const v = av[i + 1];

    if (strcmp(k, "-op_name") == 0)
    {
      /* list, set, get, listen */
      ci->flags |= CMDLINE_FLAG_OP_NAME;
      ci->op_name = v;
    }
    else if (strcmp(k, "-op_key") == 0)
    {
      ci->flags |= CMDLINE_FLAG_OP_KEY;
      ci->op_key = str_to_uint32(v);
    }
    else if (strcmp(k, "-op_val") == 0)
    {
      ci->flags |= CMDLINE_FLAG_OP_VAL;
      ci->op_val = str_to_uint32(v);
    }
    else if (strcmp(k, "-cipher_alg") == 0)
    {
      ci->flags |= CMDLINE_FLAG_CIPHER_ALG;
      if (strcmp(v, "none") == 0) ci->cipher_alg = BANO_CIPHER_ALG_NONE;
      else if (strcmp(v, "xtea") == 0) ci->cipher_alg = BANO_CIPHER_ALG_XTEA;
      else if (strcmp(v, "aes") == 0) ci->cipher_alg = BANO_CIPHER_ALG_AES;
      else return -1;
    }
    else if (strcmp(k, "-cipher_key") == 0)
    {
      ci->flags |= CMDLINE_FLAG_CIPHER_KEY;
      str_to_cipher_key(ci->cipher_key, v);
    }
    else if (strcmp(k, "-node_addr") == 0)
    {
      ci->flags |= CMDLINE_FLAG_NODE_ADDR;
      ci->node_addr = str_to_uint32(v);
    }
  }

  return 0;
}


/* main */

int main(int ac, char** av)
{
  bano_base_t base;
  bano_base_info_t binfo;
  bano_loop_info_t linfo;
  bano_socket_info_t sinfo;
  cmdline_info_t cinfo;
  int err = -1;

  seteuid(0);

  if (get_cmdline_info(&cinfo, ac - 1, av + 1))
  {
    BANO_PERROR();
    goto on_error_0;
  }

  if (bano_init())
  {
    BANO_PERROR();
    goto on_error_0;
  }

  bano_init_base_info(&binfo);
  if (bano_open(&base, &binfo))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  bano_init_socket_info(&sinfo);
  sinfo.type = BANO_SOCKET_TYPE_SNRF;
  sinfo.u.snrf.dev_path = "/dev/ttyUSB0";
  sinfo.u.snrf.addr_width = 4;
  sinfo.u.snrf.addr_val = BANO_DEFAULT_BASE_ADDR;
  if (bano_add_socket(&base, &sinfo))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  bano_init_loop_info(&linfo);

  if ((cinfo.flags & CMDLINE_FLAG_CIPHER_ALG) == 0)
  {
    cinfo.flags |= CMDLINE_FLAG_CIPHER_ALG;
    cinfo.cipher_alg = BANO_CIPHER_ALG_NONE;
  }

  linfo.cipher_alg = cinfo.cipher_alg;
  memcpy(linfo.cipher_key, cinfo.cipher_key, BANO_CIPHER_KEY_SIZE);

  if ((cinfo.flags & CMDLINE_FLAG_OP_NAME) == 0)
  {
    cinfo.flags |= CMDLINE_FLAG_OP_NAME;
    cinfo.op_name = "listen";
  }

  if (strcmp(cinfo.op_name, "list") == 0)
  {
    linfo.node_fn = on_node_print;
    linfo.user_data = NULL;
  }
  else if (strcmp(cinfo.op_name, "set") == 0)
  {
    /* set node_addr key val */

    static struct node_set_data nsd;

    nsd.base = &base;
    nsd.naddr = cinfo.node_addr;
    nsd.key = cinfo.op_key;
    nsd.val = cinfo.op_val;

    linfo.node_fn = on_node_set;
    linfo.user_data = &nsd;

    if (bano_add_node(&base, nsd.naddr))
    {
      BANO_PERROR();
      goto on_error_2;
    }
  }
  else if (strcmp(cinfo.op_name, "get") == 0)
  {
    /* get node_addr key */

    static struct node_get_data ngd;

    ngd.base = &base;
    ngd.naddr = cinfo.node_addr;
    ngd.key = cinfo.op_key;

    linfo.timer_fn = on_loop_timer;
    linfo.timer_ms = 1000;

    linfo.node_fn = on_node_get;
    linfo.user_data = &ngd;

    if (bano_add_node(&base, ngd.naddr))
    {
      BANO_PERROR();
      goto on_error_2;
    }
  }
  else if (strcmp(cinfo.op_name, "listen") == 0)
  {
    linfo.get_fn = on_get;
    linfo.set_fn = on_set;
    linfo.node_fn = on_node_print;
    linfo.user_data = NULL;
  }
  else
  {
    BANO_PERROR();
    goto on_error_2;
  }

  if (bano_start_loop(&base, &linfo))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  /* success */
  err = 0;

 on_error_2:
  bano_close(&base);
 on_error_1:
  bano_fini();
 on_error_0:
  return err;
}
