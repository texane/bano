#include <stdio.h>
#include <stdint.h>
#include "bano_sms.h"


int main(int ac, char** av)
{
  static const uint8_t sms_creds[BANO_SMS_CREDS_SIZE] =
  {
    0xea,0xac,0x69,0xa4,0x01,0x25,0x34,0x49,
    0x88,0x14,0x8d,0xc3,0x2f,0x33,0xb2,0xc6,
    0x1d,0xe3,0x6c,0x9a,0xf1,0xea,0xa5,0xa8,
    0x15,0x58,0x01,0xce,0x2d,0x5f,0xc2,0x48
  };

  bano_sms_handle_t sms;
  bano_sms_info_t info;
  int err = -1;

  info.u.https.addr = "212.27.40.200";
  info.u.https.port = "443";
  memcpy(info.u.https.creds, sms_creds, BANO_SMS_CREDS_SIZE);
  if (bano_sms_open(&sms, &info)) goto on_error_0;

  if (bano_sms_send(&sms, "00")) goto on_error_1;
  if (bano_sms_send(&sms, "01")) goto on_error_1;
  if (bano_sms_send(&sms, "02")) goto on_error_1;

  printf("success\n");

  err = 0;

 on_error_1:
  bano_sms_close(&sms);
 on_error_0:
  return err;
}
