#ifndef BANO_SMS_H_INCLUDED
#define BANO_SMS_H_INCLUDED


#include <stdint.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "core/coreApi.h"
#include "matrixssl/matrixsslApi.h"


typedef struct bano_sms_info
{
  union
  {
    struct
    {
      const char* addr;
      const char* port;
#define BANO_SMS_LOGIN_SIZE 8
#define BANO_SMS_PASS_SIZE 14
#define BANO_SMS_CREDS_SIZE 32
      uint8_t creds[BANO_SMS_CREDS_SIZE];
    } https;
  } u;
} bano_sms_info_t;


typedef struct bano_sms_handle
{
  /* used for creds ciphering */
  psAesKey_t aes_key;

  /* plain creds */
  char login[BANO_SMS_LOGIN_SIZE + 1];
  char pass[BANO_SMS_PASS_SIZE + 1];

  /* server address */
  struct sockaddr addr;

  /* toremove: ipv4 address string */
  char addr_string[16];

} bano_sms_handle_t;


void bano_sms_init_info(bano_sms_info_t*);
int bano_sms_open(bano_sms_handle_t*, const bano_sms_info_t*);
int bano_sms_close(bano_sms_handle_t*);
int bano_sms_send(bano_sms_handle_t*, const char*);
int bano_sms_send_uint32(bano_sms_handle_t*, uint32_t);

/* helper routines */
int bano_sms_encrypt_creds
(bano_sms_handle_t*, uint8_t*, const char*, const char*);
int bano_sms_decrypt_creds
(bano_sms_handle_t*, char*, char*, const uint8_t*);


#endif /* BANO_SMS_H_INCLUDED */
