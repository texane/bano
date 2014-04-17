#include <stdlib.h>
#include <sys/types.h>
#include "bano_nodl.h"
#include "bano_list.h"
#include "bano_dict.h"
#include "bano_perror.h"


int bano_nodl_alloc(bano_nodl_t** nodlp)
{
  *nodlp = malloc(sizeof(bano_nodl_t));
  if (*nodlp == NULL)
  {
    BANO_PERROR();
    return -1;
  }

  bano_dict_init(&(*nodlp)->keyvals);

  return 0;
}

static int free_keyval_item(bano_list_item_t* li, void* p)
{
  bano_dict_pair_t* const pair = li->data;
  bano_nodl_keyval_free((bano_nodl_keyval_t*)pair->val);
  return 0;
}

int bano_nodl_free(bano_nodl_t* nodl)
{
  bano_dict_fini(&nodl->keyvals, free_keyval_item, NULL);
  free(nodl);
  return 0;
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

unsigned int bano_nodl_has_key(bano_nodl_t* nodl, uint16_t key)
{
  uintptr_t args[2];

  args[0] = (uintptr_t)key;
  args[1] = (uintptr_t)NULL;

  bano_dict_foreach(&nodl->keyvals, find_keyval_item, args);

  return (args[1] == (uintptr_t)NULL) ? 0 : 1;
}

int bano_nodl_keyval_alloc(bano_nodl_keyval_t** kvp)
{
  *kvp = malloc(sizeof(bano_nodl_keyval_t));
  if (*kvp == NULL)
  {
    BANO_PERROR();
    return -1;
  }

  (*kvp)->flags = 0;
  (*kvp)->key = 0;
  (*kvp)->name[0] = 0;

  return 0;
}

int bano_nodl_keyval_free(bano_nodl_keyval_t* kv)
{
  free(kv);
  return 0;
}
