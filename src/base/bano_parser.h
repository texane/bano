#ifndef BANO_PARSER_H_INCLUDED
#define BANO_PARSER_H_INCLUDED


#include <stdint.h>
#include <sys/types.h>
#include "bano_list.h"
#include "bano_string.h"


typedef struct bano_parser_pair
{
  bano_string_t key;
  bano_string_t val;
} bano_parser_pair_t;


typedef struct bano_parser_struct
{
  bano_string_t name;
  bano_list_t pairs;
} bano_parser_struct_t;


typedef struct bano_parser_buf
{
  /* persistent until parser fini */
#define BANO_PARSER_BUF_FLAG_MMAP (1 << 0)
  uint32_t flags;
  uint8_t* data;
  size_t size;
} bano_parser_buf_t;


typedef struct bano_parser
{
#define BANO_PARSER_PATHLEN 256
  char top_dir[BANO_PARSER_PATHLEN];
  size_t top_len;
  bano_list_t bufs;
  bano_list_t structs;
  bano_list_t cstrs;
} bano_parser_t;


int bano_parser_load_file(bano_parser_t*, const char*);
int bano_parser_fini(bano_parser_t*);

int bano_parser_foreach_struct(bano_parser_t*, bano_list_fn_t, void*);
int bano_parser_foreach_pair(bano_parser_struct_t*, bano_list_fn_t, void*);

int bano_parser_find_struct(bano_parser_t*, bano_list_item_t**, const char*);
int bano_parser_find_pair(bano_parser_struct_t*, bano_list_item_t**, const char*);

int bano_parser_add_cstr(bano_parser_t*, const bano_string_t*, const char**);


#endif /* ! BANO_PARSER_H_INCLUDED */
