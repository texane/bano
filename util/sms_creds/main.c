#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "bano_sms.h"
#include "bano_string.h"
#include "core/coreApi.h"
#include "matrixssl/matrixsslApi.h"


int main(int ac, char** av)
{
  bano_sms_handle_t sms;
  bano_sms_info_t info;
  bano_string_t s;
  char l[BANO_SMS_LOGIN_SIZE + 1];
  char p[BANO_SMS_PASS_SIZE + 1];
  uint8_t creds[BANO_SMS_CREDS_SIZE];
  size_t i;
  int err = -1;

  if (bano_sms_open(&sms, &info))
    goto on_error_0;

  if (strcmp(av[1], "-d") == 0)
  {
    /* decrypt */

    if (bano_string_init_with_data(&s, av[2], strlen(av[2])))
      goto on_error_1;

    if (bano_string_to_array(&s, creds, BANO_SMS_CREDS_SIZE))
      goto on_error_1;

    bano_sms_decrypt_creds(&sms, l, p, creds);

    printf("l=%s, p=%s\n", l, p);
  }
  else if (strcmp(av[1], "-e") == 0)
  {
    /* encrypt */

    if (strlen(av[2]) != BANO_SMS_LOGIN_SIZE)
      goto on_error_1;

    if (strlen(av[3]) != BANO_SMS_PASS_SIZE)
      goto on_error_1;

    strcpy(l, av[2]);
    strcpy(p, av[3]);

    bano_sms_encrypt_creds(&sms, creds, l, p);

    for (i = 0; i != BANO_SMS_CREDS_SIZE; ++i)
    {
      char c = (i == BANO_SMS_CREDS_SIZE - 1) ? '\n' : ',';
      printf("0x%02x%c", creds[i], c);
    }
  }

  err = 0;

 on_error_1:
  bano_sms_close(&sms);
 on_error_0:
  return err;
}
