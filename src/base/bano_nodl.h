#ifndef BANO_NODL_H_INCLUDED
#define BANO_NODL_H_INCLUDED


#include <stdint.h>
#include "bano_dict.h"


typedef struct bano_nodl_keyval
{
#define BANO_NODL_FLAG_FMT_BOOL (1 << 0)
#define BANO_NODL_FLAG_FMT_UINT8 (1 << 1)
#define BANO_NODL_FLAG_FMT_UINT16 (1 << 2)
#define BANO_NODL_FLAG_FMT_UINT32 (1 << 3)
#define BANO_NODL_FLAG_BASE_BIN (1 << 4)
#define BANO_NODL_FLAG_BASE_DEC (1 << 5)
#define BANO_NODL_FLAG_MTIME (1 << 6)
#define BANO_NODL_FLAG_GET (1 << 7)
#define BANO_NODL_FLAG_SET (1 << 8)
#define BANO_NODL_FLAG_RST (1 << 9)
#define BANO_NODL_FLAG_ACK (1 << 10)
  uint32_t flags;
  char name[32];
  uint16_t key;
} bano_nodl_keyval_t;

typedef struct bano_nodl
{
  bano_dict_t keyvals;
} bano_nodl_t;


int bano_nodl_alloc(bano_nodl_t**);
int bano_nodl_free(bano_nodl_t*);
unsigned int bano_nodl_has_key(bano_nodl_t*, uint16_t);
int bano_nodl_keyval_alloc(bano_nodl_keyval_t**);
int bano_nodl_keyval_free(bano_nodl_keyval_t*);


#endif /* BANO_NODL_H_INCLUDED */
