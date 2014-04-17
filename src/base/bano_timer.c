#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "bano_timer.h"
#include "bano_list.h"
#include "bano_perror.h"


/* exported */

int bano_timer_init(bano_list_t* li)
{
  bano_list_init(li);
  return 0;
}

static int fini_timer(bano_list_item_t* it, void* p)
{
  bano_timer_t* const ti = it->data;
  free(ti);
  return 0;
}

int bano_timer_fini(bano_list_t* li)
{
  bano_list_fini(li, fini_timer, NULL);
  return 0;
}

int bano_timer_add(bano_list_t* li, bano_timer_t** tip, unsigned int ms)
{
  /* add timer in the list. list is sorted by timer ms */

  bano_timer_t* ti;
  bano_timer_t* next_ti;
  bano_list_item_t* it;
  unsigned int ms_sum;

  /* find the place to insert before */
  ms_sum = 0;
  for (it = li->head; it != NULL; it = it->next)
  {
    ti = (bano_timer_t*)it->data;
    if ((ms_sum + ti->rel_ms) > ms) break ;
    ms_sum += ti->rel_ms;
  }

  /* alloc and insert */
  ti = malloc(sizeof(bano_timer_t));
  if (ti == NULL)
  {
    BANO_PERROR();
    return -1;
  }

  if (it == NULL)
  {
    /* last timer */
    if (bano_list_add_tail(li, ti))
    {
      BANO_PERROR();
      goto on_error;
    }

    ti->it = li->tail;
    ti->rel_ms = ms - ms_sum;
  }
  else
  {
    if (bano_list_add_before(li, it, ti))
    {
      BANO_PERROR();
      goto on_error;
    }

    ti->it = it->prev;

    /* compute time relative to previous */
    ti->rel_ms = ms - ms_sum;

    /* recompute next relative time */
    next_ti = it->data;
    next_ti->rel_ms = (ms_sum + next_ti->rel_ms) - ms;
  }

  *tip = ti;

  return 0;

 on_error:
  free(ti);
  return -1;
}

int bano_timer_del(bano_list_t* li, bano_timer_t* ti)
{
#if 0
  /* recompute next timer */
  if (ti->it->next != NULL)
  {
    bano_timer_t* const next_ti = ti->it->next->data;
    next_ti->rel_ms += ti->rel_ms;
  }
#endif

  bano_list_del(li, ti->it);
  free(ti);

  return 0;
}

int bano_timer_get_next
(bano_list_t* li, bano_timer_t** ti, struct timeval* tv)
{
  if (li->head == NULL) return -1;

  *ti = li->head->data;
  tv->tv_sec = (*ti)->rel_ms / 1000;
  tv->tv_usec = ((*ti)->rel_ms % 1000) * 1000;

  return 0;
}

int bano_timer_update(bano_timer_t* ti, const struct timeval* tv)
{
  /* update the timer relative time to tv */

  ti->rel_ms = tv->tv_sec * 1000;
  ti->rel_ms += tv->tv_usec / 1000;

  return 0;
}
