#include <stdint.h>
#include <string.h>
#include "bano_cipher.h"
#include "bano_perror.h"


/* cipher context */

static enum bano_cipher_alg cipher_alg = BANO_CIPHER_ALG_NONE;
static uint8_t cipher_key[BANO_CIPHER_KEY_SIZE];


/* xtea implementation */
/* adapted from http://en.wikipedia.org/wiki/XTEA */

static void xtea_enc(uint8_t* dest, const uint8_t* v)
{
  const void* const k = cipher_key;

  uint32_t v0 = ((uint32_t*)v)[0];
  uint32_t v1 = ((uint32_t*)v)[1];
  unsigned int i;
  uint32_t sum = 0;
  const uint32_t delta = 0x9E3779B9;

  for (i = 0; i != 32; ++i)
  {
    v0 += ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + ((uint32_t*)k)[sum & 3]);
    sum += delta;
    v1 += ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + ((uint32_t*)k)[sum>>11 & 3]);
  }

  ((uint32_t*)dest)[0] = v0;
  ((uint32_t*)dest)[1] = v1;
}

static void xtea_dec(uint8_t* dest, const void* v)
{
  const void* const k = cipher_key;

  unsigned int i;
  uint32_t v0 = ((uint32_t*)v)[0];
  uint32_t v1 = ((uint32_t*)v)[1];
  uint32_t sum = 0xC6EF3720;
  uint32_t delta = 0x9E3779B9;

  for (i = 0; i != 32; ++i)
  {
    v1 -= ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + ((uint32_t*)k)[sum>>11 & 3]);
    sum -= delta;
    v0 -= ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + ((uint32_t*)k)[sum & 3]);
  }

  ((uint32_t*)dest)[0] = v0;
  ((uint32_t*)dest)[1] = v1;
}


/* exported */

int bano_cipher_init(enum bano_cipher_alg alg, const uint8_t* key)
{
  switch (alg)
  {
  case BANO_CIPHER_ALG_NONE:
    break ;

  case BANO_CIPHER_ALG_XTEA:
    memcpy(cipher_key, key, BANO_CIPHER_KEY_SIZE);
    break ;

  default:
    BANO_PERROR();
    break ;
    return -1;
  }

  /* set only on success */
  cipher_alg = alg;

  return 0;
}

int bano_cipher_fini(void)
{
  return 0;
}

int bano_cipher_enc(uint8_t* data)
{
  switch (cipher_alg)
  {
  case BANO_CIPHER_ALG_NONE:
    break ;

  case BANO_CIPHER_ALG_XTEA:
    xtea_enc(data + 0, data + 0);
    xtea_enc(data + 8, data + 8);
    break ;

  default:
    return -1;
    break ;
  }

  return 0;
}

int bano_cipher_dec(uint8_t* data)
{
  switch (cipher_alg)
  {
  case BANO_CIPHER_ALG_NONE:
    break ;

  case BANO_CIPHER_ALG_XTEA:
    xtea_dec(data + 0, data + 0);
    xtea_dec(data + 8, data + 8);
    break ;

  default:
    return -1;
    break ;
  }

  return 0;
}
