/* use matrixssl for https and crypto support */


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "bano_sms.h"
#include "bano_perror.h"
#include "core/coreApi.h"
#include "matrixssl/matrixsslApi.h"


void bano_sms_init_info(bano_sms_info_t* info)
{
  info->u.https.addr = NULL;
  info->u.https.port = NULL;
}

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

  /* decrypt creds */

  bano_sms_decrypt_creds(sms, sms->login, sms->pass, info->u.https.creds);

  /* get server socket address */

  if (info->u.https.addr != NULL)
  {
    struct sockaddr_in* const p = (struct sockaddr_in*)&sms->addr;
    memset((void*)&sms->addr, 0, sizeof(sms->addr));
    p->sin_family = AF_INET;
    p->sin_port = htons(atoi(info->u.https.port));
    p->sin_addr.s_addr = inet_addr(info->u.https.addr);
    strcpy(sms->addr_string, info->u.https.addr);
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


/* https message sending */
/* from matrixssl-3-6-1-open/apps/client.c */

#include "./third/matrixssl-3-6-1-open/sampleCerts/RSA/ALL_RSA_CAS.h"
#include "./third/matrixssl-3-6-1-open/sampleCerts/RSA/2048_RSA_KEY.h"
#include "./third/matrixssl-3-6-1-open/sampleCerts/RSA/2048_RSA.h"

#define HTTPS_BUFFER_MAX 256	
#define HTTPS_COMPLETE 1
#define HTTPS_PARTIAL 0
#define HTTPS_ERROR MATRIXSSL_ERROR

typedef struct
{
  DLListEntry List;
  ssl_t* ssl;
  int fd;
  uint32 timeout;
  uint32 flags;
  unsigned char* parsebuf;
  uint32 parsebuflen;
  uint32 bytes_received;
  uint32 bytes_requested;
  uint32 bytes_sent;
} httpConn_t;

static const char g_httpRequestHdr[] =
  "GET %s HTTP/1.0\r\n"
  "User-Agent: MatrixSSL/" MATRIXSSL_VERSION "\r\n"
  "Accept: */*\r\n"
  "Host: smsapi.free-mobile.fr\r\n"
  "Connection: Keep-Alive\r\n"
  "\r\n";

static const char g_strver[][8] = 
  { "SSL 3.0", "TLS 1.0", "TLS 1.1", "TLS 1.2" };

static char g_path[256];
static char g_ip[] = "212.27.40.200";
static int32 g_port = 443;

static int32 httpBasicParse(httpConn_t *cp, unsigned char *buf, uint32 len)
{
  unsigned char	*c, *end, *tmp;
  int32	l;

  /*
    SSL/TLS can provide zero length records, which we just ignore here
    because the code below assumes we have at least one byte
  */
  if (len == 0) {
    return HTTPS_PARTIAL;
  }

  c = buf;
  end = c + len;
  /*
    If we have an existing partial HTTP buffer, append to it the data in buf
    up to the first newline, or 'len' data, if no newline is in buf.
  */
  if (cp->parsebuf != NULL) {
    for (tmp = c; c < end && *c != '\n'; c++);
    /* We want c to point to 'end' or to the byte after \r\n */
    if (*c == '\n') {
      c++;
    }
    l = (int32)(c - tmp);
    if (l > HTTPS_BUFFER_MAX) {
      return HTTPS_ERROR;
    }
    cp->parsebuf = realloc(cp->parsebuf, l + cp->parsebuflen);
    memcpy(cp->parsebuf + cp->parsebuflen, tmp, l);
    cp->parsebuflen += l;
    /* Parse the data out of the saved buffer first */
    c = cp->parsebuf;
    end = c + cp->parsebuflen;
    /* We've "moved" some data from buf into parsebuf, so account for it */
    buf += l;
    len -= l;
  }
	
 L_PARSE_LINE:
  for (tmp = c; c < end && *c != '\n'; c++);
  if (c < end) {
    if (*(c - 1) != '\r') {
      return HTTPS_ERROR;
    }
    /* If the \r\n started the line, we're done reading headers */
    if (*tmp == '\r' && (tmp + 1 == c)) {
      if (cp->parsebuf != NULL) {
	free(cp->parsebuf); cp->parsebuf = NULL;
	cp->parsebuflen = 0;
      }
      return HTTPS_COMPLETE;
    }
  } else {
    /* If parsebuf is non-null, we have already saved it */
    if (cp->parsebuf == NULL && (l = (int32)(end -tmp)) > 0) {
      cp->parsebuflen = l;
      cp->parsebuf = malloc(cp->parsebuflen);
      psAssert(cp->parsebuf != NULL);
      memcpy(cp->parsebuf, tmp, cp->parsebuflen);
    }
    return HTTPS_PARTIAL;
  }
  *(c - 1) = '\0';	/* Replace \r with \0 just for printing */
  /* Finished parsing the saved buffer, now start parsing from incoming buf */
  if (cp->parsebuf != NULL) {
    free(cp->parsebuf); cp->parsebuf = NULL;
    cp->parsebuflen = 0;
    c = buf;
    end = c + len;
  } else {
    c++;	/* point c to the next char after \r\n */
  }		
  goto L_PARSE_LINE;

  return HTTPS_ERROR;
}

static int httpWriteRequest(ssl_t* ssl)
{
  unsigned char* buf;
  uint32 requested;
  int32 available;

  requested = strlen((char *)g_httpRequestHdr) + strlen(g_path) + 1;

  if ((available = matrixSslGetWritebuf(ssl, &buf, requested)) < 0)
  {
    BANO_PERROR();
    return -1;
  }

  requested = min(requested, available);
  snprintf((char*)buf, requested, (char*)g_httpRequestHdr, g_path);

  if (matrixSslEncodeWritebuf(ssl, strlen((char *)buf)) < 0)
  {
    BANO_PERROR();
    return -1;
  }

  return 0;
}

static int socketConnect(const char *ip, int32 port, int32 *err)
{
  struct sockaddr_in	addr;
  int			fd;
  int32			rc;
	
  /* By default, this will produce a blocking socket */
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    *err = -1;
    return -1;
  }
  fcntl(fd, F_SETFD, FD_CLOEXEC);
	
  memset((char *) &addr, 0x0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons((short)port);
  addr.sin_addr.s_addr = inet_addr(ip);
  rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
  if (rc < 0) {
    *err = -1;
  } else {
    *err = 0;
  }
  return fd;
}

static void closeConn(ssl_t *ssl, int fd)
{
	unsigned char	*buf;
	int32		len;

	/* Set the socket to non-blocking to flush remaining data */
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

	/* Quick attempt to send a closure alert, don't worry about failure */
	if (matrixSslEncodeClosureAlert(ssl) >= 0) {
		if ((len = matrixSslGetOutdata(ssl, &buf)) > 0) {
			if ((len = send(fd, buf, len, MSG_DONTWAIT)) > 0) {
				matrixSslSentData(ssl, len);
			}
		}
	}

	matrixSslDeleteSession(ssl);
	if (fd != -1) {
		close(fd);
	}
}

static int32 certCb(ssl_t *ssl, psX509Cert_t *cert, int32 alert)
{
  /* optional, but has to be implemented */
  return 0;
}

static int httpsClientConnection
(sslKeys_t* keys, sslSessionId_t* sid)
{
  tlsExtension_t* extension;
  int32	rc;
  int32 transferred;
  int32 len;
  int32 SNIextLen;
  ssl_t* ssl;
  unsigned char* buf;
  unsigned char* SNIext;
  httpConn_t cp;
  int fd;

  /* AES128-SHA */
  uint32 cipher = 47;
	
  memset(&cp, 0x0, sizeof(httpConn_t));
  fd = socketConnect(g_ip, g_port, &rc);
  if (fd == -1 || rc != PS_SUCCESS)
  {
    BANO_PERROR();
    return -1;
  }

  matrixSslNewHelloExtension(&extension);
  matrixSslCreateSNIext(NULL, (unsigned char*)g_ip, (uint32)strlen(g_ip), &SNIext, &SNIextLen);
  matrixSslLoadHelloExtension(extension, SNIext, SNIextLen, 0);
  psFree(SNIext);

  rc = matrixSslNewClientSession(&ssl, keys, sid, &cipher, 1, certCb, g_ip, extension, NULL, SSL_FLAGS_TLS_1_2);
  matrixSslDeleteHelloExtension(extension);
  if (rc != MATRIXSSL_REQUEST_SEND)
  {
    BANO_PERROR();
    return -1;
  }

 WRITE_MORE:
  while ((len = matrixSslGetOutdata(ssl, &buf)) > 0) {
    transferred = send(fd, buf, len, 0);
    if (transferred <= 0) {
      goto L_CLOSE_ERR;
    } else {
      /* Indicate that we've written > 0 bytes of data */
      if ((rc = matrixSslSentData(ssl, transferred)) < 0) {
	goto L_CLOSE_ERR;
      }
      if (rc == MATRIXSSL_REQUEST_CLOSE) {
	closeConn(ssl, fd);
	return MATRIXSSL_SUCCESS;
      } 
      if (rc == MATRIXSSL_HANDSHAKE_COMPLETE) {
	/* If we sent the Finished SSL message, initiate the HTTP req */
	/* (This occurs on a resumption handshake) */
	if (httpWriteRequest(ssl) < 0) {
	  goto L_CLOSE_ERR;
	}
	goto WRITE_MORE;
      }
      /* SSL_REQUEST_SEND is handled by loop logic */
    }
  }

 READ_MORE:
  if ((len = matrixSslGetReadbuf(ssl, &buf)) <= 0) {
    goto L_CLOSE_ERR;
  }
  if ((transferred = recv(fd, buf, len, 0)) < 0) {
    goto L_CLOSE_ERR;
  }
  /*	If EOF, remote socket closed. But we haven't received the HTTP response 
	so we consider it an error in the case of an HTTP client */
  if (transferred == 0) {
    goto L_CLOSE_ERR;
  }
  if ((rc = matrixSslReceivedData(ssl, (int32)transferred, &buf, (uint32*)&len)) < 0)
  {
    goto L_CLOSE_ERR;
  }

 PROCESS_MORE:
  switch (rc) {
  case MATRIXSSL_HANDSHAKE_COMPLETE:
    /* We got the Finished SSL message, initiate the HTTP req */
    if (httpWriteRequest(ssl) < 0) {
      goto L_CLOSE_ERR;
    }
    goto WRITE_MORE;
  case MATRIXSSL_APP_DATA:
  case MATRIXSSL_APP_DATA_COMPRESSED:
    if (cp.flags != HTTPS_COMPLETE &&
	(rc = httpBasicParse(&cp, buf, len)) < 0) {
      closeConn(ssl, fd);
      if (cp.parsebuf) free(cp.parsebuf); cp.parsebuf = NULL;
      cp.parsebuflen = 0;
      return MATRIXSSL_ERROR;
    }
    if (rc == HTTPS_COMPLETE) {
      cp.flags = HTTPS_COMPLETE;
    }
    cp.bytes_received += len;
    rc = matrixSslProcessedData(ssl, &buf, (uint32*)&len);
    if (cp.bytes_received >= 0) {
      /* We've received all that was requested, so close */
      closeConn(ssl, fd);
      if (cp.parsebuf) free(cp.parsebuf); cp.parsebuf = NULL;
      cp.parsebuflen = 0;
      return MATRIXSSL_SUCCESS;
    }
    if (rc == 0) {
      /* We processed a partial HTTP message */
      goto READ_MORE;
    }
    goto PROCESS_MORE;
  case MATRIXSSL_REQUEST_SEND:
    goto WRITE_MORE;
  case MATRIXSSL_REQUEST_RECV:
    goto READ_MORE;
  case MATRIXSSL_RECEIVED_ALERT:
    /* The first byte of the buffer is the level */
    /* The second byte is the description */
    if (*buf == SSL_ALERT_LEVEL_FATAL) {
      goto L_CLOSE_ERR;
    }
    /* Closure alert is normal (and best) way to close */
    if (*(buf + 1) == SSL_ALERT_CLOSE_NOTIFY) {
      closeConn(ssl, fd);
      if (cp.parsebuf) free(cp.parsebuf); cp.parsebuf = NULL;
      cp.parsebuflen = 0;
      return MATRIXSSL_SUCCESS;
    }
    if ((rc = matrixSslProcessedData(ssl, &buf, (uint32*)&len)) == 0) {
      /* No more data in buffer. Might as well read for more. */
      goto READ_MORE;
    }
    goto PROCESS_MORE;
  default:
    /* If rc <= 0 we fall here */
    goto L_CLOSE_ERR;
  }
	
 L_CLOSE_ERR:
  if (cp.flags != HTTPS_COMPLETE) {
  }
  matrixSslDeleteSession(ssl);
  close(fd);
  if (cp.parsebuf) free(cp.parsebuf); cp.parsebuf = NULL;
  cp.parsebuflen = 0;
  return MATRIXSSL_ERROR;
}


int bano_sms_send
(bano_sms_handle_t* sms, const char* s)
{
  /* s the msg contents */

  int err = -1;
  sslKeys_t* keys;
  sslSessionId_t* sid;
  size_t i;

  /* make the request path */
  /* /sendmsg?user=$l&pass=$k&api_id=$k&msg=$m */

  i = 0;
  i += sprintf(g_path + i, "/sendmsg?");
  i += sprintf(g_path + i, "user=%s", sms->login);
  i += sprintf(g_path + i, "&pass=%s", sms->pass);
  i += sprintf(g_path + i, "&api_id=%s", sms->pass);
  i += sprintf(g_path + i, "&msg=%s", s);

  /* create keys */

  if (matrixSslNewKeys(&keys) < 0)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  err = matrixSslLoadRsaKeysMem
  (
   keys,
   RSA2048, sizeof(RSA2048),
   RSA2048KEY, sizeof(RSA2048KEY),
   RSACAS, sizeof(RSACAS)
  );
  if (err < 0)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  matrixSslNewSessionId(&sid);

  err = httpsClientConnection(keys, sid);
  if (err < 0)
  {
    BANO_PERROR();
    goto on_error_0;
  }

  matrixSslDeleteSessionId(sid);
  matrixSslDeleteKeys(keys);

  /* success */

  err = 0;

 on_error_0:
  return err;
}

int bano_sms_send_uint32
(bano_sms_handle_t* sms, uint32_t x)
{
  char buf[32];
  sprintf(buf, "0x%08x", x);
  return bano_sms_send(sms, buf);
}
