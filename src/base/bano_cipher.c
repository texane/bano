#include <stdint.h>
#include <string.h>
#include "bano_cipher.h"
#include "bano_perror.h"


/* xtea implementation */
/* adapted from http://en.wikipedia.org/wiki/XTEA */

static void xtea_enc(uint8_t* dest, const uint8_t* v, const void* k)
{
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

static void xtea_dec(uint8_t* dest, const void* v, const void* k)
{
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

int bano_cipher_init(bano_cipher_t* cipher, const bano_cipher_info_t* info)
{
  /* currently a raw copy */
  memcpy(cipher, info, sizeof(bano_cipher_t));
  return 0;
}

int bano_cipher_fini(bano_cipher_t* cipher)
{
  return 0;
}

int bano_cipher_enc(bano_cipher_t* cipher, uint8_t* data)
{
  switch (cipher->alg)
  {
  case BANO_CIPHER_ALG_NONE:
    break ;

  case BANO_CIPHER_ALG_XTEA:
    xtea_enc(data + 0, data + 0, cipher->key);
    xtea_enc(data + 8, data + 8, cipher->key);
    break ;

  default:
    return -1;
    break ;
  }

  return 0;
}

int bano_cipher_dec(bano_cipher_t* cipher, uint8_t* data)
{
  switch (cipher->alg)
  {
  case BANO_CIPHER_ALG_NONE:
    break ;

  case BANO_CIPHER_ALG_XTEA:
    xtea_dec(data + 0, data + 0, cipher->key);
    xtea_dec(data + 8, data + 8, cipher->key);
    break ;

  default:
    return -1;
    break ;
  }

  return 0;
}
