#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include "bano_base.h"
#include "bano_node.h"
#include "bano_list.h"
#include "bano_perror.h"


/* list item freeing routines */

static int free_io_item(bano_list_item_t* li, void* p)
{
  bano_io_t* const io = li->data;
  bano_free_io(io);
  return 0;
}

static int free_pair_item(bano_list_item_t* li, void* p)
{
  return 0;
}


/* exported */

int bano_node_alloc(bano_node_t** nodep)
{
  bano_node_t* const node = malloc(sizeof(bano_node_t));

  if (node == NULL)
  {
    BANO_PERROR();
    return -1;
  }

  node->flags = 0;
  node->nodl = NULL;
  bano_list_init(&node->posted_ios);
  bano_list_init(&node->pending_ios);
  bano_dict_init(&node->keyval_pairs);

  *nodep = node;

  return 0;
}

void bano_node_free(bano_node_t* node)
{
  bano_list_fini(&node->posted_ios, free_io_item, NULL);
  bano_list_fini(&node->pending_ios, free_io_item, NULL);
  bano_dict_fini(&node->keyval_pairs, free_pair_item, NULL);
  free(node);
}
