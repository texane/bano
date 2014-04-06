#include <stdint.h>
#include "bano_cipher.h"
#include "bano_perror.h"


static enum bano_cipher_alg cipher_alg;

int bano_cipher_init(enum bano_cipher_alg alg, const uint8_t* key)
{
  cipher_alg = alg;
  return 0;
}

int bano_cipher_fini(void)
{
  return 0;
}

int bano_cipher_enc(uint8_t* data)
{
  return 0;
}

int bano_cipher_dec(uint8_t* data)
{
  return 0;
}
