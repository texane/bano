#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include "bano_list.h"
#include "bano_dict.h"
#include "bano_perror.h"


/* hash routines */

static uint32_t hash_32_to_n(uint32_t k, uint32_t n)
{
  /* 32 to n bit hash function */
  /* http://stackoverflow.com/questions/11359441 */
  /* /hash-function-for-64-bit-to-10-bits */

  k = (k + 0x479ab41d) + (k << 8);
  k = (k ^ 0xe4aa10ce) ^ (k >> 5);
  k = (k + 0x9942f0a6) - (k << 14);
  k = (k ^ 0x5aedd67d) ^ (k >> 3);
  k = (k + 0x17bea992) + (k << 7);

  return k >> (32 - n);
}

static uint32_t get_hash(const bano_dict_t* d, uint32_t k)
{
  return hash_32_to_n(k, d->log2_nlist);
}

static size_t get_nlist(const bano_dict_t* d)
{
  return 1 << d->log2_nlist;
}

static int find_pair_item(bano_list_item_t* it, void* p)
{
  const bano_dict_pair_t* const pair = it->data;
  uintptr_t* const args = p;

  if ((uint32_t)args[0] != pair->key) return 0;

  args[1] = (uintptr_t)it;
  return -1;
}

static bano_list_item_t* find_pair(bano_dict_t* d, uint32_t k)
{
  const uint32_t h = get_hash(d, k);
  uintptr_t args[2];

  args[0] = (uintptr_t)k;
  args[1] = (uintptr_t)NULL;
  bano_list_foreach(&d->lists[h], find_pair_item, (void*)args);

  return (bano_list_item_t*)args[1];
}


/* exported */

int bano_dict_init(bano_dict_t* d)
{
  /* default list count is 8 */

  static const size_t log2_nlist = 3;
  static const size_t nlist = 1 << log2_nlist;
  size_t i;

  d->log2_nlist = log2_nlist;

  d->lists = malloc(nlist * sizeof(bano_list_t));
  if (d->lists == NULL)
  {
    BANO_PERROR();
    return -1;
  }

  for (i = 0; i != nlist; ++i) bano_list_init(&d->lists[i]);

  return 0;
}

static int fini_pair_item(bano_list_item_t* it, void* p)
{
  uintptr_t* const args = (uintptr_t*)p;
  bano_list_fn_t fn = (bano_list_fn_t)args[0];

  if (fn != NULL) fn(it, (void*)args[1]);
  free(it->data);

  return 0;
}

int bano_dict_fini(bano_dict_t* d, bano_list_fn_t fn, void* p)
{
  const size_t n = get_nlist(d);
  uintptr_t args[2];
  size_t i;

  args[0] = (uintptr_t)fn;
  args[1] = (uintptr_t)p;

  for (i = 0; i != n; ++i)
  {
    bano_list_fini(&d->lists[i], fini_pair_item, (void*)args);
  }

  free(d->lists);

  return 0;
}

int bano_dict_add(bano_dict_t* d, uint32_t k, uintptr_t v)
{
  const uint32_t h = get_hash(d, k);
  bano_dict_pair_t* p;

  p = malloc(sizeof(bano_dict_pair_t));
  if (p == NULL)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  p->key = k;
  p->val = v;

  if (bano_list_add_tail(&d->lists[h], p))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  return 0;

 on_error_1:
  free(p);
 on_error_0:
  return -1;
}

int bano_dict_set(bano_dict_t* d, uint32_t k, uintptr_t v)
{
  /* assume the key already exists */

  bano_list_item_t* const it = find_pair(d, k);

  if (it == NULL) return -1;
  ((bano_dict_pair_t*)it->data)->val = v;

  return 0;
}

int bano_dict_get(bano_dict_t* d, uint32_t k, uintptr_t* v)
{
  bano_list_item_t* const it = find_pair(d, k);

  if (it == NULL) return -1;

  *v = ((bano_dict_pair_t*)it->data)->val;

  return 0;
}

int bano_dict_foreach(bano_dict_t* d, bano_list_fn_t f, void* p)
{
  const size_t n = get_nlist(d);
  size_t i;

  for (i = 0; i != n; ++i)
  {
    if (bano_list_foreach(&d->lists[i], f, p)) return -1;
  }

  return 0;
}
