#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "bano_parser.h"
#include "bano_list.h"
#include "bano_perror.h"


/* exported */

static int fini_pair_item(bano_list_item_t* it, void* p)
{
  free(it->data);
  return 0;
}

static int fini_struct_item(bano_list_item_t* it, void* p)
{
  bano_parser_struct_t* const strukt = it->data;
  bano_list_fini(&strukt->pairs, fini_pair_item, NULL);
  free(strukt);
  return 0;
}

int bano_parser_close(bano_parser_t* parser)
{
  bano_list_fini(&parser->structs, fini_struct_item, NULL);
  munmap((void*)parser->mmap_data, parser->mmap_size);
  return 0;
}

static unsigned int is_ws(uint8_t c)
{
  return ((c == ' ') || (c == '\t'));
}

static unsigned int is_nl(uint8_t c)
{
  return ((c == '\r') || (c == '\n'));
}

static unsigned int is_comment(uint8_t c)
{
  return c == '#';
}

static int parse_pair(bano_parser_t* parser, bano_parser_pair_t* pair)
{
  return -1;
}

static int parse_struct(bano_parser_t* parser, bano_parser_struct_t* strukt)
{
  return -1;
}

static void parse_line(bano_parser_t* parser)
{
}

static void parse_comment(bano_parser_t* parser)
{
}

static int do_parse(bano_parser_t* parser)
{
  int err = 0;

  parser->off = 0;

  for (parser->off = 0; parser->off != parser->mmap_size; ++parser->off)
  {
    if (is_ws(parser->mmap_data[parser->off]))
      continue ;
    else if (is_nl(parser->mmap_data[parser->off]))
      continue ;
    else if (is_comment(parser->mmap_data[parser->off]))
      continue ;
  }

  return err;
}

int bano_parser_open_file(bano_parser_t* parser, const char* path)
{
  int fd;
  struct stat st;

  bano_list_init(&parser->structs);

  fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  if (fstat(fd, &st))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  parser->mmap_data = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = -1;
  if (parser->mmap_data == (void*)MAP_FAILED)
  {
    BANO_PERROR();
    goto on_error_2;
  }

  if (do_parse(parser))
  {
    BANO_PERROR();
    goto on_error_3;
  }

  return 0;

 on_error_3:
  bano_list_fini(&parser->structs, fini_struct_item, NULL);
 on_error_2:
  munmap((void*)parser->mmap_data, parser->mmap_size);
 on_error_1:
  if (fd != -1) close(fd);
 on_error_0:
  return -1;
}


struct find_item_data
{
  unsigned int is_struct;
  const char* s;
  bano_list_item_t* it;
};

static int find_item(bano_list_item_t* it, void* p)
{
  struct find_item_data* const fid = p;
  bano_parser_string_t* s;

  if (fid->is_struct) s = &((bano_parser_struct_t*)it->data)->name;
  else s = &((bano_parser_pair_t*)it->data)->key;

  if (bano_parser_string_cmp(s, fid->s)) return 0;

  fid->it = it;

  return -1;
}

int bano_parser_find_struct
(bano_parser_t* parser, bano_list_item_t** itp, const char* name)
{
  struct find_item_data fid;
  bano_list_item_t* it;

  fid.is_struct = 1;
  fid.s = name;
  fid.it = NULL;

  if (*itp == NULL) it = parser->structs.head;
  else it = (*itp)->next;
  bano_list_foreach_at(it, find_item, (void*)&fid);

  if (fid.it == NULL) return -1;
  *itp = fid.it;
  return 0;
}

int bano_parser_find_pair
(bano_parser_struct_t* parser, bano_list_item_t** itp, const char* key)
{
  struct find_item_data fid;
  bano_list_item_t* it;

  fid.is_struct = 0;
  fid.s = key;
  fid.it = NULL;

  if (*itp == NULL) it = parser->pairs.head;
  else it = (*itp)->next;
  bano_list_foreach_at(it, find_item, (void*)&fid);

  if (fid.it == NULL) return -1;
  *itp = fid.it;
  return 0;
}

int bano_parser_string_to_bool
(const bano_parser_string_t* s, unsigned int* b)
{
  /* yes or no, true or false, 1 or 0 */

  if (bano_parser_string_cmp(s, "yes") == 0) *b = 1;
  else if (bano_parser_string_cmp(s, "true") == 0) *b = 1;
  else if (bano_parser_string_cmp(s, "1") == 0) *b = 1;
  else if (bano_parser_string_cmp(s, "no") == 0) *b = 0;
  else if (bano_parser_string_cmp(s, "false") == 0) *b = 0;
  else if (bano_parser_string_cmp(s, "0") == 0) *b = 0;
  else return -1;

  return 0;
}

int bano_parser_string_to_uint32
(const bano_parser_string_t* s, uint32_t* x)
{
  int base = 10; 
  if ((s->size > 2) && (s->data[0] == '0') && (s->data[1] == 'x'))
    base = 16;
  *x = (uint32_t)strtoul((char*)s->data, NULL, base);
  return 0;
}

int bano_parser_string_to_array
(const bano_parser_string_t* s, uint8_t* arr, size_t size)
{
  size_t i;

  if (s->size != (size * 5 - 1))
  {
    BANO_PERROR();
    return -1;
  }

  for (i = 0; i != size; ++i)
  {
    arr[i] = strtoul((char*)s->data + i * 5, NULL, 16);
  }

  return 0;
}

int bano_parser_string_cmp
(const bano_parser_string_t* a, const char* b)
{
  size_t i;

  for (i = 0; i != a->size; ++i)
  {
    if (b[i] == 0) return -1;
    if (a->data[i] != b[i]) return -1;
  }

  return 0;
}
