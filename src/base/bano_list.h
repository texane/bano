#ifndef BANO_LIST_H_INCLUDED
#define BANO_LIST_H_INCLUDED


/* linked list */

typedef struct bano_list_item
{
  void* data;
  struct bano_list_item* next;
  struct bano_list_item* prev;
} bano_list_item_t;

typedef struct
{
  bano_list_item_t* head;
  bano_list_item_t* tail;
} bano_list_t;

typedef int (*bano_list_fn_t)(bano_list_item_t*, void*);

int bano_list_init(bano_list_t*);
int bano_list_fini(bano_list_t*, bano_list_fn_t, void*);
int bano_list_foreach(bano_list_t*, bano_list_fn_t, void*);
int bano_list_foreach_at(bano_list_item_t*, bano_list_fn_t, void*);
int bano_list_find(bano_list_t*, bano_list_fn_t, void*, bano_list_item_t**);
int bano_list_add_tail(bano_list_t*, void*);
int bano_list_add_head(bano_list_t*, void*);
int bano_list_add_before(bano_list_t*, bano_list_item_t*, void*);
int bano_list_del(bano_list_t*, bano_list_item_t*);
unsigned int bano_list_is_empty(bano_list_t*);


#endif /* BANO_LIST_H_INCLUDED */
