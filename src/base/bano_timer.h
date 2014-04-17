#ifndef BANO_TIMER_H_INCLUDED
#define BANO_TIMER_H_INCLUDED


#include <sys/time.h>
#include "bano_list.h"


typedef struct bano_timer
{
  unsigned int rel_ms;
  bano_list_item_t* it;
  void* data[2];
} bano_timer_t;


int bano_timer_init(bano_list_t*);
int bano_timer_fini(bano_list_t*);
int bano_timer_add(bano_list_t*, bano_timer_t**, unsigned int);
int bano_timer_del(bano_list_t*, bano_timer_t*);
int bano_timer_get_next(bano_list_t*, bano_timer_t**, struct timeval*);
int bano_timer_update(bano_timer_t*, const struct timeval*);


#endif /* BANO_TIMER_H_INCLUDED */
