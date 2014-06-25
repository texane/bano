/* use matrixssl for https and crypto support */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "bano_sms.h"


int bano_sms_encrypt_creds
(sms_handle_t* sms, uint8_t* x, const uint8_t* l, const uint8_t* p)
{
  /* x the encrypted buffer */
  /* l the login, of size SMS_LOGIN_SIZE */
  /* p the password, of size SMS_PASS_SIZE */

  size_t i = 0;

  memcpy(x + i, l, SMS_LOGIN_SIZE);
  i += SMS_LOGIN_SIZE;
  memcpy(x + i, p, SMS_PASS_SIZE);
  i += SMS_PASS_SIZE;
  memset(x + i, 0, SMS_CREDS_SIZE - i);

  psAesInit();

  return 0;
}

int bano_sms_decrypt_creds
(sms_handle_t* sms, char* l, char* p, const uint8_t* x)
{
  return 0;
}

int main(int ac, char** av)
{
  char l[SMS_LOGIN_SIZE + 1];
  char p[SMS_PASS_SIZE + 1];
  uint8_t creds[SMS_CREDS_SIZE];

  if (strlen(av[1]) != SMS_LOGIN_SIZE) goto on_error;
  if (strlen(av[2]) != SMS_PASS_SIZE) goto on_error;
  strcpy(l, av[1]);
  strcpy(p, av[2]);

  sms_encrypt_creds(creds, l, p);
  sms_decrypt_creds(l, p, creds);
  

  return 0;
}
