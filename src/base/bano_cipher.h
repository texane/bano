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

/* 128 bits block cipher */
#define BANO_CIPHER_KEY_SIZE 16


int bano_cipher_init(enum bano_cipher_alg, const uint8_t*);
int bano_cipher_fini(void);
int bano_cipher_enc(uint8_t*);
int bano_cipher_dec(uint8_t*);


#endif /* BANO_CIPHER_H_INCLUDED */
