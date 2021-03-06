#ifndef BANO_DICT_H_INCLUDED
#define BANO_DICT_H_INCLUDED


#include <stdint.h>
#include <sys/types.h>
#include "bano_list.h"


/* dictionnaries. key,val pair store */

typedef struct bano_dict_pair
{
  uint32_t key;
  uint8_t val[1];
} bano_dict_pair_t;

typedef struct
{
  bano_list_t* lists;
  size_t log2_nlist;
  size_t val_size;
} bano_dict_t;

typedef int (*bano_dict_fn_t)(uint32_t, uintptr_t, void*);

int bano_dict_init(bano_dict_t*, size_t);
int bano_dict_init_with_nlist(bano_dict_t*, size_t, size_t);
int bano_dict_fini(bano_dict_t*, bano_list_fn_t, void*);
int bano_dict_add(bano_dict_t*, uint32_t, void**);
int bano_dict_set(bano_dict_t*, uint32_t, const void*);
int bano_dict_set_or_add(bano_dict_t*, uint32_t, const void*);
int bano_dict_get(bano_dict_t*, uint32_t, void**);
int bano_dict_foreach(bano_dict_t*, bano_list_fn_t, void*);
int bano_dict_del(bano_dict_t*, uint32_t, bano_list_fn_t, void*);


#endif /* BANO_DICT_H_INCLUDED */
