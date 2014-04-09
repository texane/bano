#ifndef BANO_CIPHER_H_INCLUDED
#define BANO_CIPHER_H_INCLUDED


#include <stdint.h>


/* crypto cipher algorithms */
enum bano_cipher_alg
{
  BANO_CIPHER_ALG_NONE = 0,
  BANO_CIPHER_ALG_XTEA,
  BANO_CIPHER_ALG_AES
};

/* 128 bits key cipher */
#define BANO_CIPHER_KEY_SIZE 16

typedef struct bano_cipher
{
  enum bano_cipher_alg alg;
  uint8_t key[BANO_CIPHER_KEY_SIZE];
} bano_cipher_t;

typedef bano_cipher_t bano_cipher_info_t;

static const bano_cipher_info_t bano_cipher_info_none =
{
  .alg = BANO_CIPHER_ALG_NONE
};

int bano_cipher_init(bano_cipher_t*, const bano_cipher_info_t*);
int bano_cipher_fini(bano_cipher_t*);
int bano_cipher_enc(bano_cipher_t*, uint8_t*);
int bano_cipher_dec(bano_cipher_t*, uint8_t*);


#endif /* BANO_CIPHER_H_INCLUDED */
