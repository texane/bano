#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "bano_parser.h"
#include "bano_cipher.h"
#include "bano_list.h"
#include "bano_perror.h"


/* allocation and init routines */

static void init_string(bano_parser_string_t* s)
{
  s->data = NULL;
  s->size = 0;
}

static bano_parser_pair_t* alloc_pair(void)
{
  bano_parser_pair_t* const pair = malloc(sizeof(bano_parser_pair_t));
  if (pair == NULL)
  {
    BANO_PERROR();
    return NULL;
  }

  init_string(&pair->key);
  init_string(&pair->val);

  return pair;
}

static int fini_pair_item(bano_list_item_t* it, void* p)
{
  free(it->data);
  return 0;
}

static bano_parser_struct_t* alloc_struct(void)
{
  bano_parser_struct_t* const strukt = malloc(sizeof(bano_parser_struct_t));
  if (strukt == NULL)
  {
    BANO_PERROR();
    return NULL;
  }

  init_string(&strukt->name);
  bano_list_init(&strukt->pairs);

  return strukt;
}

static void free_struct(bano_parser_struct_t* strukt)
{
  bano_list_fini(&strukt->pairs, fini_pair_item, NULL);
  free(strukt);
}

static int fini_struct_item(bano_list_item_t* it, void* p)
{
  free_struct(it->data);
  return 0;
}

static bano_parser_buf_t* alloc_mmap_buf(const char* path)
{
  int err = -1;
  int fd;
  struct stat st;
  bano_parser_buf_t* buf = NULL;

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

  buf = malloc(sizeof(bano_parser_buf_t));
  if (buf == NULL)
  {
    BANO_PERROR();
    goto on_error_1;
  }

  buf->size = st.st_size;
  buf->data = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (buf->data == (void*)MAP_FAILED)
  {
    BANO_PERROR();
    goto on_error_2;
  }

  err = 0;

 on_error_2:
  if (err == -1)
  {
    free(buf);
    buf = NULL;
  }
 on_error_1:
  close(fd);
 on_error_0:
  return buf;
}

static void free_buf(bano_parser_buf_t* buf)
{
  if (buf->flags & BANO_PARSER_BUF_FLAG_MMAP)
    munmap((void*)buf->data, buf->size);
  free(buf);
}

static int fini_buf_item(bano_list_item_t* it, void* p)
{
  free_buf(it->data);
  return 0;
}

static int fini_cstr_item(bano_list_item_t* it, void* p)
{
  free(it->data);
  return 0;
}


/* exported */

int bano_parser_fini(bano_parser_t* parser)
{
  bano_list_fini(&parser->structs, fini_struct_item, NULL);
  bano_list_fini(&parser->bufs, fini_buf_item, NULL);
  bano_list_fini(&parser->cstrs, fini_cstr_item, NULL);
  return 0;
}

static unsigned int is_space(uint8_t c)
{
  return ((c == ' ') || (c == '\t'));
}

static unsigned int is_nl(uint8_t c)
{
  return ((c == '\r') || (c == '\n'));
}

static unsigned int is_white(uint8_t c)
{
  return (is_nl(c) || is_space(c));
}

static unsigned int is_comment(uint8_t c)
{
  return c == '#';
}

static unsigned int is_directive(uint8_t c)
{
  return c == '.';
}

static unsigned int is_include_directive(const uint8_t* s, size_t n)
{
#define INCLUDE_STRING ".include"
#define INCLUDE_SIZE (sizeof(INCLUDE_STRING) - 1)

  if (n != INCLUDE_SIZE) return 0;
  if (memcmp(s, INCLUDE_STRING, INCLUDE_SIZE)) return 0;
  return 1;
}

static size_t skip_whites(const uint8_t* s, size_t n)
{
  size_t i;
  for (i = 0; (i != n) && (is_white(s[i])); ++i) ;
  return i;
}

static size_t skip_comment(const uint8_t* s, size_t n)
{
  /* parser one line */

  size_t i;

  for (i = 0; i != n; ++i)
  {
    if (is_nl(s[i])) break ;
  }

  if (i != n) ++i;

  return i;
}

static size_t skip_comments_and_whites(const uint8_t* s, size_t n)
{
  size_t i = 0;

  /* printf("%s\n", __FUNCTION__); */

  while (i != n)
  {
    if (is_white(s[i]))
    {
      i += skip_whites(s + i, n - i);
      if (i == n) break ;
    }

    if (is_comment(s[i]) == 0) break ;

    i += skip_comment(s + i, n - i);
  }

  return i;
}

static void next_token(size_t tok[2], const uint8_t* s, size_t n)
{
  /* retrieve next token skipping spaces and comments */
  /* tok[0]: token offset */
  /* tok[1]: token size */

  size_t i;

  /* printf("%s\n", __FUNCTION__); */

  tok[0] = skip_comments_and_whites(s, n);

  for (i = tok[0]; i != n; ++i)
  {
    if (is_space(s[i]) || is_nl(s[i])) break ;
  }

  tok[1] = i - tok[0];
}

static size_t parse_pair(bano_parser_pair_t* pair, const uint8_t* s, size_t n)
{
  size_t i = 0;
  size_t tok[2];

  /* printf("%s\n", __FUNCTION__); */

  /* key */
  next_token(tok, s + i, n - i);
  if (tok[1] == 0)
  {
    BANO_PERROR();
    return (size_t)-1;
  }
  i += tok[0];
  pair->key.data = s + i;
  pair->key.size = tok[1];
  i += tok[1];

  /* equal */
  next_token(tok, s + i, n - i);
  if (tok[1] == 0)
  {
    BANO_PERROR();
    return (size_t)-1;
  }
  i += tok[0] + tok[1];

  /* value */
  next_token(tok, s + i, n - i);
  if (tok[1] == 0)
  {
    BANO_PERROR();
    return (size_t)-1;
  }
  i += tok[0];
  pair->val.data = s + i;
  pair->val.size = tok[1];
  i += tok[1];

  return i;
}

static size_t parse_struct
(bano_parser_struct_t* strukt, const uint8_t* s, size_t n)
{
  /* return size or ((size_t)-1) on error */

  bano_parser_pair_t* pair;
  size_t tok[2];
  size_t i = 0;
  size_t j;

  /* printf("%s %u\n", __FUNCTION__, __LINE__); */

  /* name */
  next_token(tok, s + i, n - i);
  if (tok[1] == 0)
  {
    BANO_PERROR();
    return (size_t)-1;
  }
  i += tok[0];
  strukt->name.data = s + i;
  strukt->name.size = tok[1];
  i += tok[1];

  /* opening accolade */
  next_token(tok, s + i, n - i);
  if ((tok[1] != 1) || (s[i + tok[0]] != '{'))
  {
    BANO_PERROR();
    return (size_t)-1;
  }
  i += tok[0] + 1;

  /* pairs */
  while (1)
  {
    /* one token look ahead */
    next_token(tok, s + i, n - i);
    if (tok[1] == 0)
    {
      BANO_PERROR();
      return (size_t)-1;
    }

    /* closing accolade */
    if ((tok[1] == 1) && (s[i + tok[0]] == '}'))
    {
      return tok[0] + i + 1;
    }

    /* new pair */
    pair = alloc_pair();
    if (pair == NULL)
    {
      BANO_PERROR();
      return (size_t)-1;
    }

    if (bano_list_add_tail(&strukt->pairs, pair))
    {
      BANO_PERROR();
      free(pair);
      return (size_t)-1;
    }

    j = parse_pair(pair, s + i, n - i);
    if (j == (size_t)-1)
    {
      BANO_PERROR();
      /* pair freed by caller during list_fini */
      return (size_t)-1;
    }

    i += j;
  }

  /* not reached */
  return (size_t)-1;
}

static size_t parse_buf(bano_parser_t*, const uint8_t*, size_t);

static size_t parse_directive
(bano_parser_t* parser, const uint8_t* s, size_t n)
{
  char path[BANO_PARSER_PATHLEN];
  bano_parser_buf_t* buf;
  size_t tok[2];
  size_t i = 0;

  next_token(tok, s, n);

  if ((tok[1] == 0) || (is_directive(s[tok[0]]) == 0))
  {
    BANO_PERROR();
    return (size_t)-1;
  }

  if (is_include_directive(s + tok[0], tok[1]))
  {
    i += tok[0] + tok[1];

    next_token(tok, s + i, n - i);
    if ((tok[1] == 0) || ((parser->top_len + tok[1]) >= sizeof(path)))
    {
      BANO_PERROR();
      return (size_t)-1;
    }

    memcpy(path, parser->top_dir, parser->top_len);
    memcpy(path + parser->top_len, s + i + tok[0], tok[1]);
    path[parser->top_len + tok[1]] = 0;

    buf = alloc_mmap_buf(path);
    if (buf == NULL)
    {
      BANO_PERROR();
      return (size_t)-1;
    }

    if (parse_buf(parser, buf->data, buf->size) == (size_t)-1)
    {
      BANO_PERROR();
      free_buf(buf);
      return (size_t)-1;
    }

    if (bano_list_add_tail(&parser->bufs, buf))
    {
      BANO_PERROR();
      free_buf(buf);
      return (size_t)-1;
    }

    i += tok[0] + tok[1];
  }

  return i;
}

static size_t parse_directive_or_struct
(bano_parser_t* parser, const uint8_t* s, size_t n)
{
  bano_parser_struct_t* strukt;
  size_t tok[2];
  size_t i;

  /* printf("%s\n", __FUNCTION__); */

  next_token(tok, s, n);
  if (tok[1] == 0) return tok[0];

  if (is_directive(s[tok[0]]))
  {
    return parse_directive(parser, s, n);
  }

  strukt = alloc_struct();
  if (strukt == NULL)
  {
    BANO_PERROR();
    return (size_t)-1;
  }

  if (bano_list_add_tail(&parser->structs, strukt))
  {
    BANO_PERROR();
    free_struct(strukt);
    return (size_t)-1;
  }

  i = parse_struct(strukt, s, n);
  if (i == (size_t)-1)
  {
    BANO_PERROR();
    /* struct freed by caller during list_fini */
    return (size_t)-1;
  }

  /* success */

  return i;
}

static size_t parse_buf(bano_parser_t* parser, const uint8_t* s, size_t n)
{
  /* return ((size_t)-1) on error */

  size_t i;
  size_t j;

  for (i = 0; i != n; )
  {
    j = parse_directive_or_struct(parser, s + i, n - i);
    if (j == (size_t)-1)
    {
      BANO_PERROR();
      return (size_t)-1;
    }

    i += j;
  }

  return i;
}

static int get_dirname(char* dirname, size_t* dirlen, const char* path)
{
  size_t i;
  size_t j = 0;

  for (i = 0; path[i]; ++i)
  {
    if (i == BANO_PARSER_PATHLEN)
    {
      BANO_PERROR();
      return -1;
    }

    if (path[i] == '/') j = i;
  }

  memcpy(dirname, path, j + 1);
  *dirlen = j + 1;

  return 0;
}

int bano_parser_load_file(bano_parser_t* parser, const char* path)
{
  bano_parser_buf_t* buf;

  bano_list_init(&parser->structs);
  bano_list_init(&parser->bufs);
  bano_list_init(&parser->cstrs);

  if (get_dirname(parser->top_dir, &parser->top_len, path))
  {
    BANO_PERROR();
    goto on_error_0;
  }

  buf = alloc_mmap_buf(path);
  if (buf == NULL)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  if (bano_list_add_tail(&parser->bufs, buf))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  if (parse_buf(parser, buf->data, buf->size) == (size_t)-1)
  {
    BANO_PERROR();
    goto on_error_2;
  }

  return 0;

 on_error_2:
  bano_list_fini(&parser->structs, fini_struct_item, NULL);
  bano_list_fini(&parser->bufs, fini_buf_item, NULL);
  bano_list_fini(&parser->cstrs, fini_cstr_item, NULL);
 on_error_1:
  free_buf(buf);
 on_error_0:
  return -1;
}


int bano_parser_foreach_struct
(bano_parser_t* parser, bano_list_fn_t fn, void* p)
{
  return bano_list_foreach(&parser->structs, fn, p);
}

int bano_parser_foreach_pair
(bano_parser_struct_t* strukt, bano_list_fn_t fn, void* p)
{
  return bano_list_foreach(&strukt->pairs, fn, p);
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

int bano_parser_string_to_cipher_key(const bano_parser_string_t* s, uint8_t* k)
{
  static const size_t size = BANO_CIPHER_KEY_SIZE;

  size_t i;

  if (s->size != (size * 5 - 1))
  {
    BANO_PERROR();
    return -1;
  }

  for (i = 0; i != size; ++i)
  {
    k[i] = strtoul((char*)s->data + i * 5, NULL, 16);
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

int bano_parser_string_to_cstr
(bano_parser_t* parser, const bano_parser_string_t* string, const char** cstrp)
{
  char* cstr;

  cstr = malloc((string->size + 1) * sizeof(char));
  if (cstr == NULL)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  if (bano_list_add_tail(&parser->cstrs, cstr))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  memcpy(cstr, string->data, string->size);
  cstr[string->size] = 0;

  return 0;

 on_error_1:
  free(cstr);
 on_error_0:
  return -1;
}


#ifdef BANO_CONFIG_UNIT /* unit */

#include <stdio.h>

static void print_string(const bano_parser_string_t* s)
{
  size_t i;
  for (i = 0; i != s->size; ++i) printf("%c", s->data[i]);
}

static int print_pair_item(bano_list_item_t* it, void* p)
{
  bano_parser_pair_t* const pair = it->data;

  print_string(&pair->key);
  printf(" = ");
  print_string(&pair->val);
  printf("\n");

  return 0;
}

static int print_struct_item(bano_list_item_t* it, void* p)
{
  bano_parser_struct_t* const strukt = it->data;

  print_string(&strukt->name);
  printf("\n{\n");
  bano_list_foreach(&strukt->pairs, print_pair_item, NULL);
  printf("}\n");

  return 0;
}

int main(int ac, char** av)
{
  bano_parser_t parser;

  if (bano_parser_load_file(&parser, "../base_cli/conf/base_cli.conf"))
  {
    BANO_PERROR();
    return -1;
  }

  bano_list_foreach(&parser.structs, print_struct_item, NULL);

  bano_parser_fini(&parser);

  return 0;
}

#endif  /* unit */
