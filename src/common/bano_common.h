#ifndef BANO_COMMON_H_INCLUDED
#define BANO_COMMON_H_INCLUDED


#include <stdint.h>

#define BANO_OP_SET 0
#define BANO_OP_GET 1

#define BANO_FLAG_ACK (1 << 0)
#define BANO_FLAG_REPLY (1 << 1)
#define BANO_FLAG_ERR (1 << 2)

typedef struct bano_msg_hdr
{
  uint8_t op: 2;
  uint8_t flags: 6;
  uint32_t saddr: 32;
} __attribute__((packed)) bano_msg_hdr_t;

typedef struct bano_msg_get
{
  uint16_t key: 16;
} __attribute__((packed)) bano_msg_get_t;

typedef struct bano_msg_set
{
  uint16_t key: 16;
  uint32_t val: 32;
} __attribute__((packed)) bano_msg_set_t;

typedef struct bano_msg
{
#define BANO_MSG_SIZE 16
  bano_msg_hdr_t hdr;
  union
  {
    bano_msg_set_t set;
    bano_msg_get_t get;
    uint8_t pad[BANO_MSG_SIZE - sizeof(bano_msg_hdr_t)];
  } __attribute__((packed)) u;
} __attribute__((packed)) bano_msg_t;

/* reserved keys */
#define BANO_KEY_RESERVED(__x) (__x | (1 << 15))
#define BANO_KEY_NODL_ID BANO_KEY_RESERVED(0)
#define BANO_KEY_ADDR BANO_KEY_RESERVED(1)
#define BANO_KEY_ACK BANO_KEY_RESERVED(2)
#define BANO_KEY_NEXT_MSG BANO_KEY_RESERVED(3)

/* broadcast address */
#define BANO_ANY_ADDR ((uint16_t)-1)

/* default node address */
#define BANO_DEFAULT_NODE_ADDR ((uint32_t)0xa5a5a5a5)

/* default base address */
#define BANO_DEFAULT_BASE_ADDR ((uint32_t)0x5a5a5a5a)


#endif /* BANO_COMMON_H_INCLUDED */
