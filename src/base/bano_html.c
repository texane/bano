#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "bano_html.h"
#include "bano_perror.h"


static int resize(bano_html_t* html, size_t size)
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
  if (html->data != NULL) free(html->data);
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

    if (resize(html, html->off + (size_t)len + 1) == -1)
    {
      BANO_PERROR();
      goto on_error;
    }

    goto do_vsnprintf;
  }

  html->off += (size_t)len;

  return 0;

 on_error:
  html->is_err = 1;
  if (html->data != NULL) free(html->data);
  html->data = NULL;
  html->off = 0;
  html->size = 0;
  return -1;
}
