#ifndef BANO_PARSER_H_INCLUDED
#define BANO_PARSER_H_INCLUDED


#include <stdint.h>
#include <sys/types.h>
#include "bano_list.h"


typedef struct bano_parser_string
{
  const uint8_t* data;
  size_t size;
} bano_parser_string_t;


typedef struct bano_parser_pair
{
  bano_parser_string_t key;
  bano_parser_string_t val;
} bano_parser_pair_t;


typedef struct bano_parser_struct
{
  bano_parser_string_t name;
  bano_list_t pairs;
} bano_parser_struct_t;


typedef struct bano_parser
{
  size_t off;

  uint8_t* mmap_data;
  size_t mmap_size;

  bano_list_t structs;

  size_t err_line;
  size_t err_col;

} bano_parser_t;


int bano_parser_open_file(bano_parser_t*, const char*);
int bano_parser_close(bano_parser_t*);
int bano_parser_foreach_struct(bano_parser_t*, bano_list_fn_t, void*);
int bano_parser_foreach_pair(bano_parser_struct_t*);

int bano_parser_find_struct(bano_parser_t*, bano_list_item_t**, const char*);
int bano_parser_find_pair(bano_parser_struct_t*, bano_list_item_t**, const char*);

int bano_parser_string_to_bool(const bano_parser_string_t*, unsigned int*);
int bano_parser_string_to_uint32(const bano_parser_string_t*, uint32_t*);
int bano_parser_string_to_array(const bano_parser_string_t*, uint8_t*, size_t);
int bano_parser_string_cmp(const bano_parser_string_t*, const char*);


#endif /* ! BANO_PARSER_H_INCLUDED */
