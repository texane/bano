#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "bano_string.h"
#include "bano_cipher.h"
#include "bano_perror.h"


/* exported */

int bano_string_init(bano_string_t* string)
{
  return bano_string_init_with_data(string, NULL, 0);
}

int bano_string_init_with_data
(bano_string_t* string, const void* data, size_t size)
{
  string->data = data;
  string->size = size;
  return 0;
}

int bano_string_to_bool(const bano_string_t* s, unsigned int* b)
{
  /* yes or no, true or false, 1 or 0 */

  if (bano_string_cmp_cstr(s, "yes") == 0) *b = 1;
  else if (bano_string_cmp_cstr(s, "true") == 0) *b = 1;
  else if (bano_string_cmp_cstr(s, "1") == 0) *b = 1;
  else if (bano_string_cmp_cstr(s, "no") == 0) *b = 0;
  else if (bano_string_cmp_cstr(s, "false") == 0) *b = 0;
  else if (bano_string_cmp_cstr(s, "0") == 0) *b = 0;
  else return -1;

  return 0;
}

static const int get_base(const char* s)
{
  if (!(s[0] && s[1])) return 10;
  if (!((s[0] == '0') && (s[1] == 'x'))) return 10;
  return 16;
}

int bano_string_to_uint16(const bano_string_t* s, uint16_t* x)
{
  uint32_t xx;
  if (bano_string_to_uint32(s, &xx)) return -1;
  *x = (uint16_t)xx;
  return 0;
}

int bano_string_to_uint32(const bano_string_t* s, uint32_t* x)
{
  char buf[32];

  if (s->size >= sizeof(buf)) return -1;
  memcpy(buf, s->data, s->size);
  buf[s->size] = 0;

  *x = (uint32_t)strtoul(buf, NULL, get_base(buf));

  return 0;
}

int bano_string_to_array(const bano_string_t* s, uint8_t* a, size_t n)
{
  /* format: 0x00,0x01.. */
  size_t i;

  if (s->size != (n * 5 - 1))
  {
    BANO_PERROR();
    return -1;
  }

  for (i = 0; i != n; ++i)
  {
    a[i] = strtoul((char*)s->data + i * 5, NULL, 16);
  }

  return 0;
}

int bano_string_to_cipher_key(const bano_string_t* s, uint8_t* k)
{
  return bano_string_to_array(s, k, BANO_CIPHER_KEY_SIZE);
}

int bano_string_cmp_cstr(const bano_string_t* a, const char* b)
{
  size_t i;

  for (i = 0; i != a->size; ++i)
  {
    if (b[i] == 0) return -1;
    if (a->data[i] != b[i]) return -1;
  }

  return 0;
}

int bano_string_to_cstr(const bano_string_t* string, const char** cstrp)
{
  char* cstr;

  cstr = malloc((string->size + 1) * sizeof(char));
  if (cstr == NULL)
  {
    BANO_PERROR();
    return -1;
  }

  memcpy(cstr, string->data, string->size);
  cstr[string->size] = 0;

  *cstrp = cstr;

  return 0;
}
