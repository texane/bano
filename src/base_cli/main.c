/* implement a command line base */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bano_base.h"
#include "bano_perror.h"


/* capture, to remove */

#include <sys/time.h>

static struct timeval cap_tm_start;

static void cap_init(void)
{
  gettimeofday(&cap_tm_start, NULL);
}

static void cap_loop(void)
{
  struct timeval tm_cap;
  struct timeval tm_now;
  struct timeval tm_dif;
  char cmd[256];
  char py_cmd[1024];
  char im_path[256];
  size_t i;

  gettimeofday(&tm_now, NULL);
  timersub(&tm_now, &cap_tm_start, &tm_dif);
  if (tm_dif.tv_sec < (20 * 60))
  {
    printf("cap not yet enabled\n");
    return ;
  }

  printf("start capturing\n");

  tm_cap = tm_now;

  while (1)
  {
    gettimeofday(&tm_now, NULL);
    timersub(&tm_now, &tm_cap, &tm_dif);
    if (tm_dif.tv_sec > (20 * 60)) break ;

    printf("sending images\n");

    strcpy(py_cmd, "./py/main.py");
    for (i = 0; i != 2; ++i)
    {
      usleep(750000);
      system("beep");

      sprintf(im_path, "/tmp/%02x.bmp", (uint8_t)i);

      sprintf(cmd, "rm %s", im_path);
      system(cmd);

      sprintf(cmd, "/home/texane/repo/eyed/build/eyed %s", im_path);
      system(cmd);

      strcat(py_cmd, " ");
      strcat(py_cmd, im_path);
    }

    printf("%s\n", py_cmd);
    system(py_cmd);
  }

  printf("done capturing\n");
}


/* event handlers */

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

  cap_loop();

  return 0;
}


/* print node info */

static int on_node_print(void* p, bano_node_t* node, unsigned int reason)
{
  /* on new node, print node info */

  printf("-- node\n");
  printf(". reason: 0x%08x\n", reason);
  printf(". id    : 0x%04x\n", bano_get_node_id(node));
  printf("\n");

  return 0;
}


/* get a node key value */

struct node_get_data
{
  bano_base_t* base;
  uint16_t nid;
  uint16_t key;
};

static int on_get_compl(bano_io_t* io, void* p)
{
  struct node_get_data* const ngd = p;
  const uint16_t nid = ngd->nid;
  const uint16_t key = io->msg.u.get.key;
  const uint32_t val = io->compl_val;

  printf("%s: node{0x%04x, 0x%04x} == 0x%08x\n", __FUNCTION__, nid, key, val);

  return 0;
}

static int on_node_get(void* p, bano_node_t* node, unsigned int reason)
{
  /* on new node, send a get message */

  struct node_get_data* const ngd = p;
  bano_base_t* const base = ngd->base;
  const uint16_t nid = ngd->nid;
  const uint16_t key = ngd->key;

  bano_io_t* io;

  printf("%s\n", __FUNCTION__);

  if (bano_get_node_id(node) != nid) return 0;

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

  bano_set_io_timer(io, 1000000);

  if (bano_post_io(base, node, io))
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
  uint16_t nid;
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
  const uint16_t nid = nsd->nid;
  const uint16_t key = nsd->key;
  const uint32_t val = nsd->val;

  bano_io_t* io;

  printf("%s\n", __FUNCTION__);

  if (bano_get_node_id(node) != nid) return 0;

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

  bano_set_io_timer(io, 1000000);

  if (bano_post_io(base, node, io))
  {
    BANO_PERROR();
    bano_free_io(io);
    return -1;
  }

  return 0;
}


/* main */

static uint32_t str_to_uint32(const char* s)
{
  int base = 10; 
  if ((strlen(s) > 2) && (s[0] == '0') && (s[1] == 'x'))
    base = 16;
  return (uint32_t)strtoul(s, NULL, base);
}

int main(int ac, char** av)
{
  const char* const op = av[1];
  bano_base_t base;
  bano_base_info_t binfo;
  bano_loop_info_t linfo;
  int err = -1;

  seteuid(0);

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

  bano_init_loop_info(&linfo);

  if (strcmp(op, "list") == 0)
  {
    linfo.flags |= BANO_LOOP_FLAG_NODE;
    linfo.node_fn = on_node_print;
    linfo.user_data = NULL;
  }
  else if (strcmp(op, "set") == 0)
  {
    /* set node_id key val */

    static struct node_set_data nsd;

    nsd.base = &base;
    nsd.nid = str_to_uint32(av[2]);
    nsd.key = str_to_uint32(av[3]);
    nsd.val = str_to_uint32(av[4]);

    linfo.flags |= BANO_LOOP_FLAG_NODE;
    linfo.node_fn = on_node_set;
    linfo.user_data = &nsd;
  }
  else if (strcmp(op, "get") == 0)
  {
    /* get node_id key */

    static struct node_get_data ngd;

    ngd.base = &base;
    ngd.nid = str_to_uint32(av[2]);
    ngd.key = str_to_uint32(av[3]);

    linfo.flags |= BANO_LOOP_FLAG_NODE;
    linfo.node_fn = on_node_get;
    linfo.user_data = &ngd;
  }
  else if (strcmp(op, "listen") == 0)
  {
    cap_init();

    linfo.flags |= BANO_LOOP_FLAG_GET;
    linfo.get_fn = on_get;
    linfo.flags |= BANO_LOOP_FLAG_SET;
    linfo.set_fn = on_set;
    linfo.flags |= BANO_LOOP_FLAG_NODE;
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
