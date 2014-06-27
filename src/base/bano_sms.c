/* use matrixssl for https and crypto support */


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "bano_sms.h"
#include "bano_perror.h"
#include "core/coreApi.h"
#include "matrixssl/matrixsslApi.h"


int bano_sms_open
(bano_sms_handle_t* sms, const bano_sms_info_t* info)
{
  const char* k;
  size_t len;
  size_t i;
  uint8_t buf[32];
  int32_t err;

  if (matrixSslOpen() != PS_SUCCESS)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  k = getenv("BANO_SMS_KEY");
  if (k == NULL)
  {
    BANO_PERROR();
    goto on_error_1;
  }

  len = strlen(k);
  for (i = 0; i != sizeof(buf); ++i) buf[i] = k[i % len];

  err = psAesInitKey(buf, sizeof(buf), &sms->aes_key);
  if (err != PS_SUCCESS)
  {
    BANO_PERROR();
    goto on_error_1;
  }

  return 0;
 on_error_1:
  matrixSslClose();
 on_error_0:
  return -1;
}

int bano_sms_close
(bano_sms_handle_t* sms)
{
  matrixSslClose();
  return 0;
}

int bano_sms_encrypt_creds
(bano_sms_handle_t* sms, uint8_t* x, const char* l, const char* p)
{
  /* x the encrypted buffer */
  /* l the login, of size BANO_SMS_LOGIN_SIZE */
  /* p the password, of size BANO_SMS_PASS_SIZE */

  size_t i = 0;

  memcpy(x + i, l, BANO_SMS_LOGIN_SIZE);
  i += BANO_SMS_LOGIN_SIZE;
  memcpy(x + i, p, BANO_SMS_PASS_SIZE);
  i += BANO_SMS_PASS_SIZE;
  memset(x + i, 0, BANO_SMS_CREDS_SIZE - i);

  psAesEncryptBlock(x, x, &sms->aes_key);
  psAesEncryptBlock(x + 16, x + 16, &sms->aes_key);

  return 0;
}

int bano_sms_decrypt_creds
(bano_sms_handle_t* sms, char* l, char* p, const uint8_t* x)
{
  uint8_t xx[BANO_SMS_CREDS_SIZE];

  psAesDecryptBlock(x, xx, &sms->aes_key);
  psAesDecryptBlock(x + 16, xx + 16, &sms->aes_key);

  memcpy(l, xx, BANO_SMS_LOGIN_SIZE);
  l[BANO_SMS_LOGIN_SIZE] = 0;

  memcpy(p, xx + BANO_SMS_LOGIN_SIZE, BANO_SMS_PASS_SIZE);
  p[BANO_SMS_PASS_SIZE] = 0;

  return 0;
}
