#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "bano_perror.h"
#include "bano_httpd.h"
#include "mongoose.h"


static const int get_base(const char* s)
{
  if (!(s[0] && s[1])) return 10;
  if (!((s[0] == '0') && (s[1] == 'x'))) return 10;
  return 16;
}

static int str_to_uint16(const char* s, uint16_t* x)
{
  *x = (uint16_t)strtoul((char*)s, NULL, get_base(s));
  return 0;
}

static int str_to_uint32(const char* s, uint32_t* x)
{
  *x = (uint32_t)strtoul((char*)s, NULL, get_base(s));
  return 0;
}

static int on_event(struct mg_connection* conn, enum mg_event ev)
{
  int err = MG_TRUE;

  switch (ev)
  {
  case MG_REQUEST:
    {
      static const int invalid_compl_err = 0xdeadbeef;
      bano_httpd_t* const httpd = conn->server_param;
      int compl_err = invalid_compl_err;
      bano_httpd_msg_t msg;
      char buf[32];

      mg_get_var(conn, "op", buf, sizeof(buf));
      if (strcmp(buf, "get") == 0) msg.op = BANO_HTTPD_MSG_OP_GET;
      else if (strcmp(buf, "set") == 0) msg.op = BANO_HTTPD_MSG_OP_SET;
      else msg.op = BANO_HTTPD_MSG_OP_INVALID;

      mg_get_var(conn, "naddr", buf, sizeof(buf));
      str_to_uint32(buf, &msg.naddr);

      mg_get_var(conn, "key", buf, sizeof(buf));
      str_to_uint16(buf, &msg.key);

      mg_get_var(conn, "val", buf, sizeof(buf));
      str_to_uint32(buf, &msg.val);

      msg.httpd = httpd;
      msg.conn = conn;
      msg.compl_err = &compl_err;
      msg.is_header = 0;

      if (write(httpd->msg_pipe[1], &msg, sizeof(msg)) != sizeof(msg))
      {
	BANO_PERROR();
	mg_send_header(conn, "Content-Type", "text/plain");
	mg_printf_data(conn, "fatal error\n");
	break ;
      }

      /* wait for message completion */
      pthread_mutex_lock(&httpd->compl_lock);
      while (*msg.compl_err == invalid_compl_err)
      {
	pthread_cond_wait(&httpd->compl_cond, &httpd->compl_lock);
      }
      pthread_mutex_unlock(&httpd->compl_lock);

      break ;
    }
  case MG_AUTH:
    {
      break ;
    }

  default:
    {
      err = MG_FALSE;
      break ;
    }
  }

  return err;
}

static void* server_thread_entry(void* p)
{
  bano_httpd_t* const httpd = p;

  while (httpd->is_done == 0)
  {
    mg_poll_server((struct mg_server*)httpd->server, 1000000);
  }

  httpd->is_done = 0;

  return NULL;
}

int bano_httpd_init(bano_httpd_t* httpd, const bano_httpd_info_t* info)
{
  char buf[32];
  uint16_t port;

  httpd->base = info->base;
  httpd->is_done = 0;

  if (pipe(httpd->msg_pipe))
  {
    BANO_PERROR();
    goto on_error_0;
  }

  if (pthread_mutex_init(&httpd->compl_lock, NULL))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  if (pthread_cond_init(&httpd->compl_cond, NULL))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  httpd->server = mg_create_server((void*)httpd, on_event);
  if (httpd->server == NULL)
  {
    BANO_PERROR();
    goto on_error_3;
  }

  if (info->flags & BANO_HTTPD_FLAG_PORT) port = info->port;
  else port = 80;
  sprintf(buf, "%hd", port);
  mg_set_option(httpd->server, "listening_port", buf);
  mg_start_thread(server_thread_entry, httpd);

  return 0;

 on_error_3:
  pthread_cond_destroy(&httpd->compl_cond);
 on_error_2:
  pthread_mutex_destroy(&httpd->compl_lock);
 on_error_1:
  close(httpd->msg_pipe[0]);
  close(httpd->msg_pipe[1]);
 on_error_0:
  return -1;
}

int bano_httpd_fini(bano_httpd_t* httpd)
{
  httpd->is_done = 1;
  mg_wakeup_server(httpd->server);
  while (httpd->is_done == 1) usleep(100000);
  mg_destroy_server(&httpd->server);

  pthread_cond_destroy(&httpd->compl_cond);
  pthread_mutex_destroy(&httpd->compl_lock);
  close(httpd->msg_pipe[0]);
  close(httpd->msg_pipe[1]);

  return 0;
}

int bano_httpd_write(bano_httpd_msg_t* msg, const uint8_t* data, size_t size)
{
  if (msg->is_header == 0)
  {
    msg->is_header = 1;
    mg_send_header(msg->conn, "Content-Type", "text/html");
  }

  if (mg_write(msg->conn, data, size) != size)
  {
    BANO_PERROR();
    return -1;
  }

  return 0;
}

int bano_httpd_complete(bano_httpd_msg_t* msg, int err)
{
  pthread_mutex_lock(&msg->httpd->compl_lock);
  *msg->compl_err = err;
  pthread_cond_signal(&msg->httpd->compl_cond);
  pthread_mutex_unlock(&msg->httpd->compl_lock);
  return 0;
}
