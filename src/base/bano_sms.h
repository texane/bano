#ifndef BANO_SMS_H_INCLUDED
#define BANO_SMS_H_INCLUDED


#include <stdint.h>


typedef struct bano_sms_info
{
  union
  {
    struct
    {
      const char* addr;
      const char* port;
#define SMS_LOGIN_SIZE 8
#define SMS_PASS_SIZE 14
#define SMS_CREDS_SIZE 32
      uint8_t creds[SMS_CREDS_SIZE];
    } https;
  } u;
} bano_sms_info_t;


typedef struct bano_sms_handle
{
  struct sockaddr_in addr;
} bano_sms_handle_t;


int bano_sms_open(sms_handle_t*, const sms_info_t*);
int bano_sms_close(sms_handle_t*);
int bano_sms_send(sms_handle_t*, const char*, unsigned int);

/* helper routines */
int bano_sms_encrypt_creds(uint8_t*, const char*, const char*);
int bano_sms_decrypt_creds(char*, char*, const uint8_t*);


#endif /* BANO_SMS_H_INCLUDED */
