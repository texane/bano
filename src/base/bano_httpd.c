#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "bano_perror.h"
#include "bano_httpd.h"
#include "mongoose.h"


static int on_event(struct mg_connection* conn, enum mg_event ev)
{
  /* bano_httpd_t* const httpd = conn->server_param; */
  int err = MG_TRUE;

  switch (ev)
  {
  case MG_REQUEST:
    {
      mg_send_header(conn, "Content-Type", "text/plain");
      mg_printf_data(conn, "This is a reply from server");
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

  if (pipe(httpd->req_pipe))
  {
    BANO_PERROR();
    goto on_error_0;
  }

  if (pthread_mutex_init(&httpd->rep_lock, NULL))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  if (pthread_cond_init(&httpd->rep_cond, NULL))
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
  pthread_cond_destroy(&httpd->rep_cond);
 on_error_2:
  pthread_mutex_destroy(&httpd->rep_lock);
 on_error_1:
  close(httpd->req_pipe[0]);
  close(httpd->req_pipe[1]);
 on_error_0:
  return -1;
}

int bano_httpd_fini(bano_httpd_t* httpd)
{
  httpd->is_done = 1;
  mg_wakeup_server(httpd->server);
  while (httpd->is_done == 1) usleep(100000);
  mg_destroy_server(&httpd->server);

  pthread_cond_destroy(&httpd->rep_cond);
  pthread_mutex_destroy(&httpd->rep_lock);
  close(httpd->req_pipe[0]);
  close(httpd->req_pipe[1]);

  return 0;
}
