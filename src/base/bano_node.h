#ifndef BANO_NODE_H_INCLUDED
#define BANO_NODE_H_INCLUDED


#include <stdint.h>
#include <time.h>
#include "bano_list.h"
#include "bano_dict.h"


struct bano_socket;
struct bano_nodl;


typedef struct bano_node_val
{
  /* actual value */
  uint32_t val;
  /* last modification time */
  struct timespec mtime;
} bano_node_val_t;


typedef struct bano_node
{
  /* valid: BANO_NODE_FLAG_CIPHER,NAME */
  uint32_t flags;

  uint32_t addr;

  char name[32];

  struct bano_socket* socket;
  struct bano_nodl* nodl;

  bano_list_t posted_ios;
  bano_list_t pending_ios;

  /* {key, bano_node_val_t} dict */
  bano_dict_t keyval_pairs;

} bano_node_t;


typedef struct bano_node_info
{
#define BANO_NODE_FLAG_ADDR (1 << 0)
#define BANO_NODE_FLAG_SEED (1 << 1)
#define BANO_NODE_FLAG_NODL_ID (1 << 2)
#define BANO_NODE_FLAG_CIPHER (1 << 3)
#define BANO_NODE_FLAG_SOCKET (1 << 4)
#define BANO_NODE_FLAG_NAME (1 << 5)
  uint32_t flags;
  uint32_t addr;
  char name[32];
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

static inline const char* bano_node_get_name(const bano_node_t* node)
{
  if (node->flags & BANO_NODE_FLAG_NAME) return node->name;
  return "none";
}

static inline void bano_init_node_info(bano_node_info_t* info)
{
  info->flags = 0;
}


#endif /* ! BANO_NODE_H_INCLUDED */
