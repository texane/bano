#ifndef BANO_STRING_H_INCLUDED
#define BANO_STRING_H_INCLUDED


#include <stdint.h>
#include <sys/types.h>


typedef struct bano_string
{
  const uint8_t* data;
  size_t size;
} bano_string_t;


int bano_string_init(bano_string_t*);
int bano_string_init_with_data(bano_string_t*, const void*, size_t);
int bano_string_to_bool(const bano_string_t*, unsigned int*);
int bano_string_to_uint16(const bano_string_t*, uint16_t*);
int bano_string_to_uint32(const bano_string_t*, uint32_t*);
int bano_string_to_array(const bano_string_t*, uint8_t*, size_t);
int bano_string_to_cipher_key(const bano_string_t*, uint8_t*);
int bano_string_to_cstr(const bano_string_t*, const char**);
int bano_string_cmp_cstr(const bano_string_t*, const char*);


#endif /* ! BANO_STRING_H_INCLUDED */
