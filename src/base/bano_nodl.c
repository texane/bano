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
