#
#	Makefile for MatrixSSL Commercial product package
#   'make' builds optimized (-Os default).
#   'make debug' builds debug mode.
#
#	Copyright (c) INSIDE Secure, 2013. All Rights Reserved.
#

include common.mk

SSL_DYN= libmatrixssl$(SO)
SSL_STATIC = libmatrixssl$(A)

CORE_OBJECTS = \
	./core/corelib$(O) \
	./core/$(OSDEP)/osdep$(O)

CRYPTO_OBJECTS = \
	./crypto/keyformat/asn1$(O) \
	./crypto/keyformat/base64$(O) \
	./crypto/keyformat/x509$(O) \
	./crypto/digest/hmac$(O) \
	./crypto/digest/md2$(O) \
	./crypto/digest/md4$(O) \
	./crypto/digest/md5$(O) \
	./crypto/digest/sha1$(O) \
	./crypto/digest/sha224$(O) \
	./crypto/digest/sha256$(O) \
	./crypto/digest/sha384$(O) \
	./crypto/digest/sha512$(O) \
	./crypto/hardware/aesni$(O) \
	./crypto/math/pstm$(O) \
	./crypto/math/pstm_mul_comba$(O) \
	./crypto/math/pstm_sqr_comba$(O) \
	./crypto/math/pstm_montgomery_reduce$(O) \
	./crypto/pubkey/pubkey$(O) \
	./crypto/pubkey/pkcs$(O) \
	./crypto/pubkey/rsa$(O) \
	./crypto/pubkey/dh$(O) \
	./crypto/pubkey/ecc$(O) \
	./crypto/prng/prng$(O) \
	./crypto/prng/yarrow$(O) \
	./crypto/symmetric/aes$(O) \
	./crypto/symmetric/aesGCM$(O) \
	./crypto/symmetric/arc4$(O) \
	./crypto/symmetric/idea$(O) \
	./crypto/symmetric/rc2$(O) \
	./crypto/symmetric/seed$(O) \
	./crypto/symmetric/des3$(O)

SSL_OBJECTS = \
	./matrixssl/cipherSuite$(O) \
	./matrixssl/hsHash$(O) \
	./matrixssl/matrixssl$(O) \
	./matrixssl/matrixsslApi$(O) \
	./matrixssl/sslDecode$(O) \
	./matrixssl/sslEncode$(O) \
	./matrixssl/psk$(O) \
	./matrixssl/prf$(O) \
	./matrixssl/tls$(O) \
	./matrixssl/sslv3$(O)


#
# Compile options
#
CFLAGS  += -I./ -I../ 

compile: $(SSL_STATIC) $(SSL_DYN)
	@echo $(STROPTS)

$(CORE_OBJECTS): core/*.h core/*.c core/*/*.c

$(CRYPTO_OBJECTS): DFLAGS += $(NOPIC)
$(CRYPTO_OBJECTS): $(CORE_OBJECTS) crypto/*.h crypto/*/*.h crypto/*/*.c

$(SSL_OBJECTS): $(CORE_OBJECTS) $(CRYPTO_OBJECTS) matrixssl/*.c matrixssl/*.h

#
# Build the dynamic SSL library
#
$(SSL_DYN): $(CORE_OBJECTS) $(CRYPTO_OBJECTS) $(SSL_OBJECTS)
	$(CC) $(SHARED) -o $@ $^ $(LDFLAGS)
	$(STRIP) $(SSL_DYN)

#
# Build the static SSL library
# Direct stderr to null so we don't see the 'empty file' warnings
#
$(SSL_STATIC): $(CORE_OBJECTS) $(CRYPTO_OBJECTS) $(SSL_OBJECTS)
	$(AR) -rcuv $@ $^ 2>/dev/null

#
#	Clean up all generated files
#
clean:
	rm -f $(CORE_OBJECTS) $(CRYPTO_OBJECTS) $(SSL_OBJECTS) $(SSL_DYN) $(SSL_STATIC)

