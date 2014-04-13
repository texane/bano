#ifndef BANO_NODE_H_INCLUDED
#define BANO_NODE_H_INCLUDED


#include <stdint.h>
#include "bano_list.h"
#include "bano_dict.h"


struct bano_socket;


typedef struct bano_node
{
  /* valid: BANO_NODE_FLAG_CIPHER */
  uint32_t flags;

  uint32_t addr;

  bano_socket_t* socket;

  bano_list_t posted_ios;
  bano_list_t pending_ios;

  bano_dict_t keyval_pairs;

  /* TODO: bano_nodl_t* nodl; */

} bano_node_t;


typedef struct bano_node_info
{
#define BANO_NODE_FLAG_ADDR (1 << 0)
#define BANO_NODE_FLAG_SEED (1 << 1)
#define BANO_NODE_FLAG_NODL_ID (1 << 2)
#define BANO_NODE_FLAG_CIPHER (1 << 3)
#define BANO_NODE_FLAG_SOCKET (1 << 4)
  uint32_t flags;
  uint32_t addr;
  uint32_t seed;
  uint32_t nodl_id;
  struct bano_socket* socket;
} bano_node_info_t;


int bano_node_alloc(bano_node_t**);
void bano_node_free(bano_node_t*);

static inline uint32_t bano_node_get_addr(const bano_node_t* node)
{
  return node->addr;
}

static inline void bano_init_node_info(bano_node_info_t* i)
{
  i->flags = 0;
}


#endif /* ! BANO_NODE_H_INCLUDED */
