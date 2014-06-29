#include <stdlib.h>
#include <sys/types.h>
#include "bano_nodl.h"
#include "bano_list.h"
#include "bano_dict.h"
#include "bano_perror.h"


void bano_nodl_init(bano_nodl_t* nodl)
{
  bano_dict_init(&nodl->keyvals, sizeof(bano_nodl_keyval_t));
}

void bano_nodl_fini(bano_nodl_t* nodl)
{
  bano_dict_fini(&nodl->keyvals, NULL, NULL);
}

static int find_keyval_item(bano_list_item_t* li, void* p)
{
  bano_dict_pair_t* const pair = li->data;
  bano_nodl_keyval_t* const kv = (bano_nodl_keyval_t*)pair->val;
  uintptr_t* const args = p;
  const uint16_t key = (uint16_t)args[0];

  if (kv->key != key) return 0;

  args[1] = (uintptr_t)li;
  return -1;
}

bano_nodl_keyval_t* bano_nodl_find_key(bano_nodl_t* nodl, uint16_t key)
{
  bano_list_item_t* li;
  bano_dict_pair_t* pair;
  uintptr_t args[2];

  args[0] = (uintptr_t)key;
  args[1] = (uintptr_t)NULL;

  bano_dict_foreach(&nodl->keyvals, find_keyval_item, args);

  if (args[1] == NULL) return NULL;

  li = (bano_list_item_t*)args[1];
  pair = li->data;
  return (bano_nodl_keyval_t*)pair->val;
}

unsigned int bano_nodl_has_key(bano_nodl_t* nodl, uint16_t key)
{
  return (bano_nodl_find_key(nodl, key) == NULL) ? 0 : 1;
}

void bano_nodl_keyval_init(bano_nodl_keyval_t* kv)
{
  kv->flags = 0;
  kv->key = 0;
  kv->name[0] = 0;
}
