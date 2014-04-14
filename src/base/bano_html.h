#ifndef BANO_HTML_H_INCLUDED
#define BANO_HTML_H_INCLUDED


#include <sys/types.h>


typedef struct bano_html
{
  char* data;
  size_t off;
  size_t size;
  unsigned int is_err;
} bano_html_t;


int bano_html_init(bano_html_t*);
void bano_html_fini(bano_html_t*);
int bano_html_printf(bano_html_t*, const char*, ...);

static inline const char* bano_html_get_data(bano_html_t* html)
{
  return html->data;
}

static inline size_t bano_html_get_size(bano_html_t* html)
{
  /* actually the offset field */
  return html->off;
}

static inline unsigned int bano_html_is_err(bano_html_t* html)
{
  return html->is_err;
}


#endif /* BANO_HTML_H_INCLUDED */
