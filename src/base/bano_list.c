#include <stdlib.h>
#include "bano_list.h"

int bano_list_init(bano_list_t* l)
{
  l->head = NULL;
  l->tail = NULL;
  return 0;
}

int bano_list_fini(bano_list_t* l, bano_list_fn_t f, void* p)
{
  bano_list_item_t* pos;

  pos = l->head;
  while (pos)
  {
    bano_list_item_t* const tmp = pos;
    pos = pos->next;
    f(tmp, p);
    free(tmp);
  }

  l->head = NULL;
  l->tail = NULL;

  return 0;
}

int bano_list_foreach(bano_list_t* l, bano_list_fn_t f, void* p)
{
  bano_list_item_t* pos;

  pos = l->head;
  while (pos)
  {
    bano_list_item_t* const tmp = pos;
    pos = pos->next;
    if (f(tmp, p)) break ;
  }

  return 0;
}

int bano_list_find
(bano_list_t* l, bano_list_fn_t f, void* p, bano_list_item_t** res)
{
  bano_list_item_t* pos;

  for (pos = l->head; pos; pos = pos->next)
  {
    if (f(pos, p) == 0)
    {
      *res = pos;
      return 0;
    }
  }

  return -1;
}

static bano_list_item_t* alloc_item(void)
{
  bano_list_item_t* const i = malloc(sizeof(bano_list_item_t));
  if (i == NULL) return NULL;
  i->prev = NULL;
  i->next = NULL;
  return i;
}

int bano_list_add_tail(bano_list_t* l, void* p)
{
  bano_list_item_t* const i = alloc_item();
  if (i == NULL) return -1;

  i->data = p;

  i->prev = l->tail;

  if (l->tail != NULL) l->tail->next = i;
  else l->head = i;
  l->tail = i;

  return 0;
}

int bano_list_add_head(bano_list_t* l, void* p)
{
  bano_list_item_t* const i = alloc_item();
  if (i == NULL) return -1;
  i->data = p;

  i->next = l->head;
  if (l->head != NULL) l->head->prev = i;
  else l->tail = i;
  l->head = i;

  return 0;
}

int bano_list_del(bano_list_t* l, bano_list_item_t* i)
{
  if (i->prev) i->prev->next = i->next;
  else l->head = i->next;

  if (i->next) i->next->prev = i->prev;
  else l->tail = i->prev;
  
  free(i);

  return 0;
}

unsigned int bano_list_is_empty(bano_list_t* li)
{
  return li->head == NULL;
}
