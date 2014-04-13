#include <stdlib.h>
#include <string.h>
#include "bano_socket.h"
#include "bano_perror.h"
#include "../common/bano_common.h"
#include "snrf.h"
#include "snrf_common.h"


static int do_read(void* p, void* m)
{
  /* return 0 on success, bano_msg filled */
  /* return -1 on error */
  /* return -2 if message partially read or not of interest */

  snrf_handle_t* const snrf = p;
  bano_msg_t* const bano_msg = m;
  snrf_msg_t snrf_msg;
  int err;

  err = snrf_read_msg(snrf, &snrf_msg);
  if (err == -1)
  {
    BANO_PERROR();
  }
  else if (err == 0)
  {
    if (snrf_msg.op == SNRF_OP_PAYLOAD)
    {
      memcpy(bano_msg, snrf_msg.u.payload.data, sizeof(bano_msg_t));
    }
    else
    {
      BANO_PERROR();
      err = -2;
    }
  }

  return err;
}

static int do_peek(void* p, void* m)
{
  snrf_handle_t* const snrf = p;
  bano_msg_t* const bano_msg = m;
  snrf_msg_t snrf_msg;

  while (snrf_get_pending_msg(snrf, &snrf_msg) == 0)
  {
    if (snrf_msg.op == SNRF_OP_PAYLOAD)
    {
      memcpy(bano_msg, snrf_msg.u.payload.data, sizeof(bano_msg_t));
      return 0;
    }
  }

  return -1;
}

static int do_write(void* p, uint32_t addr, const void* m)
{
  snrf_handle_t* const snrf = p;
  int err = -1;

  if (snrf_set_keyval(snrf, SNRF_KEY_STATE, SNRF_STATE_CONF))
  {
    BANO_PERROR();
    goto on_error;
  }

  if (snrf_set_keyval(snrf, SNRF_KEY_TX_ADDR, addr))
  {
    BANO_PERROR();
    goto on_error;
  }

  if (snrf_set_keyval(snrf, SNRF_KEY_STATE, SNRF_STATE_TXRX))
  {
    BANO_PERROR();
    goto on_error;
  }

  if (snrf_write_payload(snrf, (const uint8_t*)m, sizeof(bano_msg_t)))
  {
    BANO_PERROR();
    goto on_error;
  }

  /* success */
  err = 0;

 on_error:
  return err;
}

static int do_ctl(void* p, unsigned int k, unsigned int v)
{
  snrf_handle_t* const snrf = p;

  if (snrf_set_keyval(snrf, (uint8_t)k, (uint32_t)v))
  {
    BANO_PERROR();
    return -1;
  }

  return 0;
}

static int do_close(void* p)
{
  snrf_handle_t* const snrf = p;
  const int err = snrf_close(snrf);
  free(p);
  return err;
}

static int do_get_fd(void* p)
{
  snrf_handle_t* const snrf = p;
  return snrf_get_fd(snrf);
}

/* exported */

int bano_socket_snrf_open(bano_socket_t* socket, const bano_snrf_info_t* si)
{
  snrf_handle_t* snrf;

  snrf = malloc(sizeof(snrf_handle_t));
  if (snrf == NULL)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  if (snrf_open_with_path(snrf, si->dev_path))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  if (snrf_set_keyval(snrf, SNRF_KEY_STATE, SNRF_STATE_CONF))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  if (snrf_set_keyval(snrf, SNRF_KEY_CRC, SNRF_CRC_DISABLED))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  if (snrf_set_keyval(snrf, SNRF_KEY_TX_ACK, SNRF_TX_ACK_DISABLED))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  if (snrf_set_keyval(snrf, SNRF_KEY_ADDR_WIDTH, (uint32_t)si->addr_width))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  if (snrf_set_keyval(snrf, SNRF_KEY_RX_ADDR, si->addr_val))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  if (snrf_set_keyval(snrf, SNRF_KEY_PAYLOAD_WIDTH, BANO_MSG_SIZE))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  if (snrf_set_keyval(snrf, SNRF_KEY_STATE, SNRF_STATE_TXRX))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  /* put in listen mode */

  socket->read_fn = do_read;
  socket->peek_fn = do_peek;
  socket->write_fn = do_write;
  socket->ctl_fn = do_ctl;
  socket->close_fn = do_close;
  socket->get_fd_fn = do_get_fd;
  socket->data = snrf;
  
  return 0;

 on_error_2:
  snrf_close(snrf);
 on_error_1:
  free(snrf);
 on_error_0:
  return -1;
}
