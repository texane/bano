#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bano_html.h"
#include "bano_perror.h"


static int resize_data(bano_html_t* html, size_t size)
{
  const size_t new_size = (size + 0x1000) & ~(0x1000 - 1);
  void* const new_data = realloc(html->data, new_size);

  if (new_data == NULL)
  {
    BANO_PERROR();
    html->is_err = 1;
    return -1;
  }

  html->data = new_data;
  html->size = new_size;

  return 0;
}

static void free_data(bano_html_t* html)
{
  if (html->data != NULL) free(html->data);
  html->data = NULL;
  html->size = 0;
  html->off = 0;
}

int bano_html_init(bano_html_t* html)
{
  html->off = 0;
  html->data = NULL;
  html->size = 0;
  html->is_err = 0;
  return 0;
}

void bano_html_fini(bano_html_t* html)
{
  free_data(html);
}

int bano_html_printf(bano_html_t* html, const char* fmt, ...)
{
  int len;
  size_t size;
  va_list va;
  unsigned int once = 1;

  if (html->is_err) return -1;

 do_vsnprintf:
  size = html->size - html->off;

  va_start(va, fmt);
  len = vsnprintf(html->data + html->off, size, fmt, va);
  va_end(va);

  if (len < 0)
  {
    BANO_PERROR();
    goto on_error;
  }

  if (len >= (int)size)
  {
    if (once == 0)
    {
      BANO_PERROR();
      goto on_error;
    }

    once = 0;

    if (resize_data(html, html->off + (size_t)len + 1) == -1)
    {
      BANO_PERROR();
      goto on_error;
    }

    goto do_vsnprintf;
  }

  html->off += (size_t)len;

  return 0;

 on_error:
  free_data(html);
  html->is_err = 1;
  return -1;
}

int bano_html_include(bano_html_t* html, const char* filename)
{
  struct stat st;
  size_t new_size;
  int err = -1;
  int fd;

  if (html->is_err) return -1;

  fd = open(filename, O_RDONLY);
  if (fd == -1)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  if (fstat(fd, &st))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  new_size = html->off + st.st_size;
  if (new_size > html->size)
  {
    if (resize_data(html, new_size))
    {
      BANO_PERROR();
      goto on_error_1;
    }
  }

  if (read(fd, html->data + html->off, st.st_size) != st.st_size)
  {
    BANO_PERROR();
    goto on_error_1;
  }

  html->off += st.st_size;

  err = 0;

 on_error_1:
  close(fd);
 on_error_0:
  if (err)
  {
    free_data(html);
    html->is_err = 1;
    return -1;
  }

  return 0;
}
