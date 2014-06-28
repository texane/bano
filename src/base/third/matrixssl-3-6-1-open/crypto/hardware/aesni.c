/*
 *	aesni.c
 *	Release $Name: MATRIXSSL-3-6-1-OPEN $
 *
 *  Support for AES-NI Hardware Crypto Instructions (x86-64 platforms)
 */
/*
 *	Copyright (c) 2013 INSIDE Secure Corporation
 *	Copyright (c) PeerSec Networks, 2002-2011
 *	All Rights Reserved
 *
 *	The latest version of this code is available at http://www.matrixssl.org
 *
 *	This software is open source; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This General Public License does NOT permit incorporating this software 
 *	into proprietary programs.  If you are unable to comply with the GPL, a 
 *	commercial license for this software may be purchased from INSIDE at
 *	http://www.insidesecure.com/eng/Company/Locations
 *	
 *	This program is distributed in WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	See the GNU General Public License for more details.
 *	
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *	http://www.gnu.org/copyleft/gpl.html
 */
/******************************************************************************/

#include "aesni.h"

#ifdef USE_AESNI_CRYPTO
#ifndef __AES__
#error "'-maes' must be present in GCC compiler flags for AES-NI support"
#endif

#include <wmmintrin.h>
#ifndef __APPLE__
#include <cpuid.h>
#endif

#ifdef USE_AES_CBC_EXTERNAL

int32 psAesInit(psCipherContext_t *ctx, unsigned char *IV,
				unsigned char *key, uint32 keylen)
{
	int32		err;
	uint32		idx = 1, a, b, c, d;
	
	if (IV == NULL || key == NULL || ctx == NULL) {
		psTraceCrypto("psAesInit arg fail\n");
		return PS_ARG_FAIL;
	}
#ifndef __APPLE__
	__cpuid(idx, a, b, c, d);
#else
    asm volatile (
        "pushq %%rbx		\n\t"
        "cpuid				\n\t"
        "movl %%ebx ,%[ebx]	\n\t"
        "popq %%rbx			\n\t"
        : "=a"(a), [ebx] "=r"(b), "=c"(c), "=d"(d)
        : "a"(idx));
#endif
/*
	Check for     AESNI: CPUID.01H:ECX.AESNI[bit 25] = 1
*/
	if (!(c & 0x2000000)) {
		psTraceCrypto("psAesInit aes-ni unsupported\n");
		return PS_PLATFORM_FAIL;
	}
	ctx->aes.keylen = keylen;
    if ((err = psAesInitKey(key, keylen, &ctx->aes.key)) != PS_SUCCESS) {
        return err;
    }
	ctx->aes.blocklen = 16;
	for (a = 0; a < 16; a++) {
		ctx->aes.IV[a] = IV[a];
	}
	
	return PS_SUCCESS;
}

static __m128i psAesKeygenAssist(__m128i temp, int i)
{
	/*	Note that the second argument for _mm_aeskeygenassistant()
		is required to be a compile-time constant. */
	switch (i) {
		case 0: return _mm_aeskeygenassist_si128(temp, 0x01);
		case 1: return _mm_aeskeygenassist_si128(temp, 0x02);
		case 2: return _mm_aeskeygenassist_si128(temp, 0x04);
		case 3: return _mm_aeskeygenassist_si128(temp, 0x08);
		case 4: return _mm_aeskeygenassist_si128(temp, 0x10);
		case 5: return _mm_aeskeygenassist_si128(temp, 0x20);
		case 6: return _mm_aeskeygenassist_si128(temp, 0x40);
		case 7: return _mm_aeskeygenassist_si128(temp, 0x80);
		case 8: return _mm_aeskeygenassist_si128(temp, 0x1b);
		case 9: return _mm_aeskeygenassist_si128(temp, 0x36);
		case 10: return temp;
		default: break;
	}
	return temp;
}

int32 psAesInitKey(const unsigned char *ckey, uint32 keylen, psAesKey_t *key)
{
	__m128i		temp1, temp2, temp3, temp4;
	int			i, offset;
#ifdef AESNI_192 /* Not currently supported */
	int			kstemp[46];
#endif

	if (key == NULL || ckey == NULL) {
		psTraceCrypto("Bad args to psAesInitKey\n");
		return PS_ARG_FAIL;
	}
	key->type = AES_UNDEFINED;
	switch (keylen) {
	case 16:
		key->rounds = 10;
		temp1 = _mm_loadu_si128((__m128i *)ckey);
#ifdef PSTM_64BIT
		key->skey[0] = temp1;
#else
		_mm_storeu_si128(&key->skey[0], temp1);
#endif
		for (i = 0; i < 10; i++) {
			temp2 = psAesKeygenAssist(temp1, i);
			temp2 = _mm_shuffle_epi32(temp2, 0xff);
			temp3 = _mm_slli_si128(temp1, 0x4);
			temp1 = _mm_xor_si128(temp1, temp3);
			temp3 = _mm_slli_si128(temp3, 0x4);
			temp1 = _mm_xor_si128(temp1, temp3);
			temp3 = _mm_slli_si128(temp3, 0x4);
			temp1 = _mm_xor_si128(temp1, temp3);
			temp1 = _mm_xor_si128(temp1, temp2);
#ifdef PSTM_64BIT
			key->skey[i + 1] = temp1;
#else
			_mm_storeu_si128(&key->skey[i + 1], temp1);
#endif
		}
	break;

#ifdef AESNI_192 /* Not currently supported macro _set_epi32 */
	case 24:
		key->rounds = 12;
		temp1 = _mm_loadu_si128((__m128i *)ckey);
		temp3 = _mm_loadu_si128((__m128i *)(ckey + 16));
		key->skey[0] = temp1;
		offset = 0;
		for (i = 0; i < 8; i++) {
			temp2 = psAesKeygenAssist(temp3, i);
			temp2 = _mm_shuffle_epi32(temp2, 0x55);
			temp4 = temp1;
			temp4 = _mm_slli_si128(temp4, 0x4);
			temp1 = _mm_xor_si128(temp1, temp4);
			temp4 = _mm_slli_si128(temp4, 0x4);
			temp1 = _mm_xor_si128(temp1, temp4);
			temp4 = _mm_slli_si128(temp4, 0x4);
			temp1 = _mm_xor_si128(temp1, temp4);
			temp1 = _mm_xor_si128(temp1, temp2);
			temp2 = _mm_shuffle_epi32(temp1, 0xff);
			temp4 = temp3;
			temp4 = _mm_slli_si128(temp4, 0x4);
			temp3 = _mm_xor_si128(temp3, temp4);
			temp3 = _mm_xor_si128(temp3, temp2);

			kstemp[offset++] = _mm_extract_epi32(temp1, 0);
			kstemp[offset++] = _mm_extract_epi32(temp1, 1);
			kstemp[offset++] = _mm_extract_epi32(temp1, 2);
			kstemp[offset++] = _mm_extract_epi32(temp1, 3);
			kstemp[offset++] = _mm_extract_epi32(temp3, 0);
			kstemp[offset++] = _mm_extract_epi32(temp3, 1);
		}
		key->skey[1] = _mm_set_epi32(kstemp[1], kstemp[0],
			_mm_extract_epi32((ckey + 16), 1),
			_mm_extract_epi32((ckey + 16), 0));
		for (i = 2; i < offset - 4; i += 4) {
			key->skey[(i/4) + 2] = _mm_set_epi32(kstemp[i+3],
			kstemp[i+2], kstemp[i+1], kstemp[i]);
		}
	break;
#endif

	case 32:
		key->rounds = 14;
		temp1 = _mm_loadu_si128((__m128i *)ckey);
#ifdef PSTM_64BIT
		key->skey[0] = temp1;
#else
		_mm_storeu_si128(&key->skey[0], temp1);
#endif
		temp3 = _mm_loadu_si128((__m128i *)(ckey + 16));
#ifdef PSTM_64BIT
		key->skey[1] = temp3;
#else
		_mm_storeu_si128(&key->skey[1], temp3);
#endif
		offset = 2;
		for (i = 0; i < 7; i++) {
			temp2 = psAesKeygenAssist(temp3, i);
			temp2 = _mm_shuffle_epi32(temp2, 0xff);
			temp4 = temp1;
			temp4 = _mm_slli_si128(temp4, 0x4);
			temp1 = _mm_xor_si128(temp1, temp4);
			temp4 = _mm_slli_si128(temp4, 0x4);
			temp1 = _mm_xor_si128(temp1, temp4);
			temp4 = _mm_slli_si128(temp4, 0x4);
			temp1 = _mm_xor_si128(temp1, temp4);
			temp1 = _mm_xor_si128(temp1, temp2);
#ifdef PSTM_64BIT
			key->skey[offset] = temp1;
#else
			_mm_storeu_si128(&key->skey[offset], temp1);
#endif
			offset++;
			if (offset == 15) {
				return PS_SUCCESS;
			}
			temp4 = _mm_aeskeygenassist_si128(temp1, 0x0);
			temp2 = _mm_shuffle_epi32(temp4, 0xaa);
			temp4 = temp3;
			temp4 = _mm_slli_si128(temp4, 0x4);
			temp3 = _mm_xor_si128(temp3, temp4);
			temp4 = _mm_slli_si128(temp4, 0x4);
			temp3 = _mm_xor_si128(temp3, temp4);
			temp4 = _mm_slli_si128(temp4, 0x4);
			temp3 = _mm_xor_si128(temp3, temp4);
			temp3 = _mm_xor_si128(temp3, temp2);

#ifdef PSTM_64BIT
			key->skey[offset] = temp3;
#else
			_mm_storeu_si128(&key->skey[offset], temp3);
#endif
			offset++;
		}
	break;

	default:
		psTraceCrypto("Invalid AES key length\n");
		return CRYPT_INVALID_KEYSIZE;
	}
	return PS_SUCCESS;
}

/* psAesInitKey has set up the key for encryption, change it */
static void setDecryptKey(psAesKey_t *key)
{
	__m128i			temp;
  	int				i;
	
	/* No change to key[0] */
	for (i = 1; i < key->rounds; i++)
	{
#ifdef PSTM_64BIT
		temp = key->skey[i];
#else
		temp = _mm_loadu_si128(&key->skey[i]);
#endif
		temp = _mm_aesimc_si128(temp);

#ifdef PSTM_64BIT
		key->skey[i] = temp;
#else
		_mm_storeu_si128(&key->skey[i], temp);
#endif
	}
	/* No change to key[i] */
}

static void encryptBlock(__m128i *dst, __m128i *src, __m128i *round_keys,
							unsigned int rounds)
{
	unsigned int i;
	__m128i key_schedule[15];
	__m128i temp;

	for (i = 0; i <= rounds; i++) {
#ifdef PSTM_64BIT
		key_schedule[i] = round_keys[i];
#else
		key_schedule[i] = _mm_loadu_si128(&round_keys[i]);
#endif
	}
	/* First round */
	temp = _mm_xor_si128(*src, key_schedule[0]);
	/* Middle rounds */
	for (i = 1; i < rounds; i++) {
		temp = _mm_aesenc_si128(temp, key_schedule[i]);
	}
	/* Last round */
	*dst = _mm_aesenclast_si128(temp, key_schedule[rounds]);
}

static void decryptBlock(__m128i *dst, __m128i *src, __m128i *round_keys,
							unsigned int rounds)
{
	unsigned int i;
	__m128i key_schedule[15];
	__m128i temp;

	for (i = 0; i <= rounds; i++) {
#ifdef PSTM_64BIT
		key_schedule[i] = round_keys[i];
#else
		key_schedule[i] = _mm_loadu_si128(&round_keys[i]);
#endif
	}
	/* First round */
	temp = _mm_xor_si128(*src, key_schedule[rounds]);
	/* Middle rounds */
	for (i = 1; i < rounds; i++) {
		temp = _mm_aesdec_si128(temp, key_schedule[rounds - i]);
	}
	/* Last round */
	*dst = _mm_aesdeclast_si128(temp, key_schedule[0]);
}

void psAesEncryptBlock(const unsigned char *pt, unsigned char *ct,
                psAesKey_t *key)
{
	__m128i		src, dst;

	if (key->type != AES_ENCRYPT) {
		psAssert(key->type == AES_UNDEFINED);
		key->type = AES_ENCRYPT;
	}
	src = _mm_loadu_si128((__m128i *)(pt));
	encryptBlock(&dst, &src, key->skey, key->rounds);
	_mm_storeu_si128((void *)(ct), dst);
}

void psAesDecryptBlock(const unsigned char *ct, unsigned char *pt,
                psAesKey_t *key)
{
	__m128i		src, dst;

	if (key->type != AES_DECRYPT) {
		psAssert(key->type == AES_UNDEFINED);
		key->type = AES_DECRYPT;
		setDecryptKey(key);
	}
	src = _mm_loadu_si128((__m128i *)(ct));
	decryptBlock(&dst, &src, key->skey, key->rounds);
	_mm_storeu_si128((void *)(pt), dst);
}

/* Encrypt in CBC mode */
int32 psAesEncrypt(psCipherContext_t *ctx, unsigned char *pt,
							unsigned char *ct, uint32 len)
{
	unsigned int b;
	__m128i src_m128i, temp_m128i;

    if (ct == NULL || pt == NULL || ctx == NULL || (len & 0x7) != 0) {
        psTraceCrypto("Bad parameters to aesCbc\n");
        return PS_ARG_FAIL;
    }
	if (ctx->aes.key.type != AES_ENCRYPT) {
		psAssert(ctx->aes.key.type == AES_UNDEFINED);
		ctx->aes.key.type = AES_ENCRYPT;
	}
	temp_m128i = _mm_loadu_si128((__m128i *)(ctx->aes.IV));
	for (b = 0; b < len; b += 16) {
		src_m128i = _mm_loadu_si128((__m128i *)(pt + b));
		src_m128i = _mm_xor_si128(src_m128i, temp_m128i);
		encryptBlock(&temp_m128i, &src_m128i,
			ctx->aes.key.skey, ctx->aes.key.rounds);
		_mm_storeu_si128((void *)(ct + b), temp_m128i);
	}
	_mm_storeu_si128((void *)(ctx->aes.IV), temp_m128i);
	return b;
}

/* Decrypt in CBC mode */
int32 psAesDecrypt(psCipherContext_t *ctx, unsigned char *ct,
							unsigned char *pt, uint32 len)
{
	unsigned int b;
	__m128i src_m128i, temp_m128i, temp2_m128i, temp3_m128i;

    if (pt == NULL || ct == NULL || ctx == NULL || (len & 0x7) != 0) {
        psTraceCrypto("Bad parameters to aesCbc\n");
        return PS_ARG_FAIL;
    }
	if (ctx->aes.key.type != AES_DECRYPT) {
		psAssert(ctx->aes.key.type == AES_UNDEFINED);
		ctx->aes.key.type = AES_DECRYPT;
		setDecryptKey(&ctx->aes.key);
	}
	temp3_m128i = temp_m128i = _mm_loadu_si128((__m128i *)(ctx->aes.IV));
	for (b = 0; b < len; b += 16) {
		src_m128i = _mm_loadu_si128((__m128i *)(ct + b));
		decryptBlock(&temp2_m128i, &src_m128i,
			ctx->aes.key.skey, ctx->aes.key.rounds);
		temp_m128i = _mm_xor_si128(temp2_m128i, temp3_m128i);
		temp3_m128i = src_m128i;
		_mm_storeu_si128((void *)(pt + b), temp_m128i);
	}
	_mm_storeu_si128((void *)(ctx->aes.IV), src_m128i);
	return b;
}

#endif /* USE_AES_CBC_EXTERNAL */

/******************************************************************************/

#ifdef USE_AES_GCM_EXTERNAL

#include <smmintrin.h>

static __m128i flip_m128i(__m128i input_m128i);
static void galois_mul(__m128i a, __m128i b, __m128i *res);
static __m128i galois_hash(__m128i h_m128i, __m128i y_m128i,
                           unsigned char *buffer, size_t len);
static void galois_counter(psAesCipher_t *ctx, unsigned char *dst,
                           const unsigned char *src, size_t len);
static void gcm_update(psAesCipher_t *ctx, const unsigned char *buffer, 
						size_t len);
static void gcm_transform(psAesCipher_t *ctx, unsigned char *dest,
				  const unsigned char *src, size_t len, unsigned char *iv);
static void gcm_final(psAesCipher_t *ctx, unsigned char *digest);

/* Init cipher with key */
int32 psAesInitGCM(psCipherContext_t *ctx, unsigned char *key, int32 keylen)
{
	__m128i		zero_m128i, h_m128i;
	uint32		idx =1, a, b, c, d;
	int32		err;

	if (key == NULL || ctx == NULL) {
		psTraceCrypto("psAesInitGCM arg fail\n");
		return PS_ARG_FAIL;
	}
#ifndef __APPLE__
	__cpuid(idx, a, b, c, d);
#else
    asm volatile (
        "pushq %%rbx		\n\t"
        "cpuid				\n\t"
        "movl %%ebx ,%[ebx]	\n\t"
        "popq %%rbx			\n\t"
        : "=a"(a), [ebx] "=r"(b), "=c"(c), "=d"(d)
        : "a"(idx)
	);
#endif
/*
	Check for     AESNI: CPUID.01H:ECX.AESNI[bit 25] = 1
	and       PCLMULQDQ: CPUID.01H:ECX.PCLMULQDQ[bit 1] = 1
*/
    if (!(c & 0x2000001)) {
        psTraceCrypto("psAesInitGCM aes-ni and pclmulqdq unsupported\n");
        return PS_PLATFORM_FAIL;
    }
    ctx->aes.keylen = keylen;
    if ((err = psAesInitKey(key, keylen, &ctx->aes.key)) != PS_SUCCESS) {
        return err;
    }
    ctx->aes.blocklen = 16;

	/* Pre-calculate H */
	zero_m128i = _mm_setzero_si128();
	encryptBlock(&h_m128i, &zero_m128i, ctx->aes.key.skey, ctx->aes.key.rounds);
	ctx->aes.a_len = 0;
	ctx->aes.c_len = 0;
	ctx->aes.cipher_started = 0;  
#ifdef PSTM_64BIT
	ctx->aes.y_m128i = zero_m128i;
#else
	_mm_storeu_si128(&ctx->aes.y_m128i, zero_m128i);
#endif
	/* Pre-invert byte order in H */
#ifdef PSTM_64BIT
	ctx->aes.h_m128i = flip_m128i(h_m128i);
#else
	_mm_storeu_si128(&ctx->aes.h_m128i, flip_m128i(h_m128i));
#endif

	return PS_SUCCESS;
}

/*
	Assign the IV and init hash state with additional authenticated data (AAD)
	AEAD Ciphers and AAD:
	http://tools.ietf.org/html/rfc5116
	TLS 1.2 contents of AAD:
	http://tools.ietf.org/html/rfc5246#section-6.2.3.3
	GCM Spec:
	http://csrc.nist.gov/publications/nistpubs/800-38D/SP-800-38D.pdf
*/
int32 psAesReadyGCM(psCipherContext_t *ctx, unsigned char *IV,
                    unsigned char *aad, int32 aadLen)
{
#ifdef PSTM_64BIT
	ctx->aes.y_m128i = _mm_setzero_si128();
#else
	_mm_storeu_si128(&ctx->aes.y_m128i, _mm_setzero_si128());
#endif
	ctx->aes.c_len = 0;
	ctx->aes.cipher_started = 0;  

    memcpy(ctx->aes.IV, IV, 12);

	/* The AAD is TLS 1.2 specific */
	gcm_update(&ctx->aes, aad, aadLen);
	/* The a_len holds the number of bytes of AAD */
	ctx->aes.a_len = aadLen;

    ctx->aes.blocklen = 16;
    return 0;
}

/* Encrypt pt to ct and update the internal hash state */
int32 psAesEncryptGCM(psCipherContext_t *ctx, unsigned char *pt,
		unsigned char *ct, int32 len)
{
	if (ctx->aes.key.type != AES_ENCRYPT) {
		psAssert(ctx->aes.key.type == AES_UNDEFINED);
		ctx->aes.key.type = AES_ENCRYPT;
	}
	gcm_transform(&ctx->aes, ct, pt, len, ctx->aes.IV);
	return PS_SUCCESS;
}

/* Output TagBytes of the hash state (digest) to tag. */
int32 psAesGetGCMTag(psCipherContext_t *ctx, int32 TagBytes, unsigned char *tag)
{
	unsigned char digest[16];

	psAssert(TagBytes <= 16);
	psAssert(ctx->aes.key.type != AES_UNDEFINED);
	gcm_final(&ctx->aes, digest);
	memcpy(tag, digest, TagBytes);
	return PS_SUCCESS;
}

/* Decrypt ct to pt and verify hash in ct */
int32 psAesDecryptGCM(psCipherContext_t *ctx, unsigned char *ct, int32 ctLen,
            unsigned char *pt, int32 ptLen)
{
	int				tlen;
	unsigned char	digest[16];

	if (ctx->aes.key.type != AES_DECRYPT) {
		psAssert(ctx->aes.key.type == AES_UNDEFINED);
		ctx->aes.key.type = AES_DECRYPT;
	}
	tlen = ctLen - ptLen;
	gcm_transform(&ctx->aes, pt, ct, ptLen, ctx->aes.IV);
	gcm_final(&ctx->aes, digest);
	if (memcmp(digest, ct + ptLen, tlen) != 0) {
		return -1;
	}
	return PS_SUCCESS;
}

/******************************************************************************/

/* Flip byte endian in an _m128 */
static __m128i flip_m128i(__m128i input_m128i)
{
	__m128i output_m128i;

	output_m128i = _mm_set_epi8(_mm_extract_epi8(input_m128i, 0),
		_mm_extract_epi8(input_m128i, 1),
		_mm_extract_epi8(input_m128i, 2),
		_mm_extract_epi8(input_m128i, 3),
		_mm_extract_epi8(input_m128i, 4),
		_mm_extract_epi8(input_m128i, 5),
		_mm_extract_epi8(input_m128i, 6),
		_mm_extract_epi8(input_m128i, 7),
		_mm_extract_epi8(input_m128i, 8),
		_mm_extract_epi8(input_m128i, 9),
		_mm_extract_epi8(input_m128i, 10),
		_mm_extract_epi8(input_m128i, 11),
		_mm_extract_epi8(input_m128i, 12),
		_mm_extract_epi8(input_m128i, 13),
		_mm_extract_epi8(input_m128i, 14),
		_mm_extract_epi8(input_m128i, 15));

	return output_m128i;
}

/* NIST Special Publication 800-38D: 6.5 */
static void galois_counter(psAesCipher_t *ctx, unsigned char *dst,
                           const unsigned char *src, size_t len)
{
	unsigned int i, j, n, partial_len;
	__m128i key_schedule[15];
	__m128i temp_m128i, src_m128i, icb_m128i;
	__m128i ONE_m128i = _mm_set_epi8(1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);

	if (len == 0) {
		return;
	}
#ifdef PSTM_64BIT
	icb_m128i = ctx->icb_m128i;
#else
	icb_m128i = _mm_loadu_si128(&ctx->icb_m128i);
#endif

	for (i = 0; i <= ctx->key.rounds; i++) {
#ifdef PSTM_64BIT
		key_schedule[i] = ctx->key.skey[i];
#else
		key_schedule[i] = _mm_loadu_si128(&ctx->key.skey[i]);
#endif
	}

	n = len / 16;
	partial_len = len % 16;

	/* Handle multiple of blocksize */
	for (i = 0; i < n; i++) {
		/* First round */
		temp_m128i = _mm_xor_si128(icb_m128i, key_schedule[0]);
		/* Middle rounds */
		for (j = 1; j < ctx->key.rounds; j++) {
			temp_m128i = _mm_aesenc_si128(temp_m128i, key_schedule[j]);
		}
		/* Last round */
		temp_m128i = _mm_aesenclast_si128(temp_m128i, 
			key_schedule[ctx->key.rounds]);
		/* Fetch source and XOR to dest */
		src_m128i = _mm_loadu_si128((__m128i *)(src + i * 16));
		temp_m128i = _mm_xor_si128(src_m128i, temp_m128i);
		_mm_storeu_si128((void *)(dst + i * 16), temp_m128i);

		/* Increment and continue */
		icb_m128i = _mm_add_epi64(icb_m128i, ONE_m128i);
	}

	/* Handle remainder */
	if (partial_len != 0) {
		unsigned int partial[16];
		memset(partial, 0x00, 16);
		memcpy(partial, src + (n * 16), partial_len);

		/* First round */
		temp_m128i = _mm_xor_si128(icb_m128i, key_schedule[0]);
		/* Middle rounds */
		for (j = 1; j < ctx->key.rounds; j++) {
			temp_m128i = _mm_aesenc_si128(temp_m128i, key_schedule[j]);
		}
		/* Last round */
		temp_m128i = _mm_aesenclast_si128(temp_m128i, 
			key_schedule[ctx->key.rounds]);
		/* Fetch source and XOR to dest */
		src_m128i = _mm_loadu_si128((__m128i *)(partial));
		temp_m128i = _mm_xor_si128(src_m128i, temp_m128i);
		_mm_storeu_si128((void *)(partial), temp_m128i);

		memcpy(dst + (n * 16), partial, partial_len);
	}
#ifdef PSTM_64BIT
	ctx->icb_m128i = icb_m128i;
#else
	_mm_storeu_si128(&ctx->icb_m128i, icb_m128i);
#endif
}

/* NIST Special Publication 800-38D: 6.3 */
static void galois_mul(__m128i a, __m128i b, __m128i *res)
{
	__m128i tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9;

	/* Inputs and output in reverse byte order */

	tmp3 = _mm_clmulepi64_si128(a, b, 0x00);
	tmp4 = _mm_clmulepi64_si128(a, b, 0x10);
	tmp5 = _mm_clmulepi64_si128(a, b, 0x01);
	tmp6 = _mm_clmulepi64_si128(a, b, 0x11);

	tmp4 = _mm_xor_si128(tmp4, tmp5);
	tmp5 = _mm_slli_si128(tmp4, 8);
	tmp4 = _mm_srli_si128(tmp4, 8);
	tmp3 = _mm_xor_si128(tmp3, tmp5);
	tmp6 = _mm_xor_si128(tmp6, tmp4);

	tmp7 = _mm_srli_epi32(tmp3, 31);
	tmp8 = _mm_srli_epi32(tmp6, 31);
	tmp3 = _mm_slli_epi32(tmp3, 1);
	tmp6 = _mm_slli_epi32(tmp6, 1);

	tmp9 = _mm_srli_si128(tmp7, 12);
	tmp8 = _mm_slli_si128(tmp8, 4);
	tmp7 = _mm_slli_si128(tmp7, 4);
	tmp3 = _mm_or_si128(tmp3, tmp7);
	tmp6 = _mm_or_si128(tmp6, tmp8);
	tmp6 = _mm_or_si128(tmp6, tmp9);

	tmp7 = _mm_slli_epi32(tmp3, 31);
	tmp8 = _mm_slli_epi32(tmp3, 30);
	tmp9 = _mm_slli_epi32(tmp3, 25);
  
	tmp7 = _mm_xor_si128(tmp7, tmp8);
	tmp7 = _mm_xor_si128(tmp7, tmp9);
	tmp8 = _mm_srli_si128(tmp7, 4);
	tmp7 = _mm_slli_si128(tmp7, 12);
	tmp3 = _mm_xor_si128(tmp3, tmp7);
  
	tmp2 = _mm_srli_epi32(tmp3, 1);
	tmp4 = _mm_srli_epi32(tmp3, 2);
	tmp5 = _mm_srli_epi32(tmp3, 7);
	tmp2 = _mm_xor_si128(tmp2, tmp4);
	tmp2 = _mm_xor_si128(tmp2, tmp5);
	tmp2 = _mm_xor_si128(tmp2, tmp8);
	tmp3 = _mm_xor_si128(tmp3, tmp2);
	tmp6 = _mm_xor_si128(tmp6, tmp3);

	*res = tmp6;
}

/* NIST Special Publication 800-38D: 6.4 */
static __m128i galois_hash(__m128i h_m128i, __m128i y_m128i,
                           unsigned char *buffer, size_t len)
{
	__m128i x_m128i, temp_m128i, temp2_m128i;
	int i;

	psAssert(len % 16 == 0);

	temp2_m128i = _mm_setzero_si128();
	/* H is already stored in reversed byte order */
	temp_m128i = flip_m128i(y_m128i);

	for (i = 0; i < len; i += 16) {
		x_m128i = _mm_set_epi8(*(buffer + i),
			*(buffer + i + 1),
			*(buffer + i + 2),
			*(buffer + i + 3),
			*(buffer + i + 4),
			*(buffer + i + 5),
			*(buffer + i + 6),
			*(buffer + i + 7),
			*(buffer + i + 8),
			*(buffer + i + 9),
			*(buffer + i + 10),
			*(buffer + i + 11),
			*(buffer + i + 12),
			*(buffer + i + 13),
			*(buffer + i + 14),
			*(buffer + i + 15));

		temp_m128i = _mm_xor_si128(temp_m128i, x_m128i);
		galois_mul(h_m128i, temp_m128i, &temp2_m128i);
		temp_m128i = temp2_m128i;
	}

	return flip_m128i(temp2_m128i);
}

/*
	Update the GCM hash state (does not update a_len) 
	If just hashing data, but not encrypting, a_len should be incremented
		by caller.
*/
static void gcm_update(psAesCipher_t *ctx, const unsigned char *buffer, 
						size_t len)
{
#ifndef PSTM_64BIT
	__m128i  temp, temp2;
#endif
	unsigned char partial[16];
	uint32 partial_len;

	if (len == 0) {
		return;
	}
	partial_len = len % 16;
	/* First multiples of blocksize */
	if (len >= 16) {
#ifdef PSTM_64BIT
		ctx->y_m128i = galois_hash(ctx->h_m128i, ctx->y_m128i, 
			(unsigned char *)buffer, len - partial_len);
#else
		// need to allign galois_hash parameters before calling
		temp = _mm_loadu_si128(&ctx->h_m128i);
		temp2 = _mm_loadu_si128(&ctx->y_m128i);
		_mm_storeu_si128(&ctx->y_m128i, galois_hash(temp, temp2, 
			(unsigned char *)buffer, len - partial_len));
#endif
    }
	/* The last partial block */
	if (partial_len != 0) {
		memset(partial, 0x00, 16);
		memcpy(partial, ((buffer + len) - partial_len), partial_len);
#ifdef PSTM_64BIT
		ctx->y_m128i = galois_hash(ctx->h_m128i, ctx->y_m128i, partial, 16);
#else
		temp = _mm_loadu_si128(&ctx->h_m128i);
		temp2 = _mm_loadu_si128(&ctx->y_m128i);
		_mm_storeu_si128(&ctx->y_m128i, galois_hash(temp, temp2, partial, 16));
#endif
	}
}

#define GCM_PUT_32BIT(cp, value) do { \
  ((unsigned char *)(cp))[0] = (unsigned char)((value) >> 24); \
  ((unsigned char *)(cp))[1] = (unsigned char)((value) >> 16); \
  ((unsigned char *)(cp))[2] = (unsigned char)((value) >> 8); \
  ((unsigned char *)(cp))[3] = (unsigned char)(value); } while (0)

static void gcm_final(psAesCipher_t *ctx, unsigned char *digest)
{
#ifndef PSTM_64BIT
	__m128i	temp, temp2;
#endif
	unsigned char len_buffer[16];
	unsigned char iv_full[16];
	unsigned char final_y[16];

	memset(len_buffer, 0x00, 16);
	/* Store the number of bytes of AAD and AEAD */
	GCM_PUT_32BIT(len_buffer + 4, ctx->a_len * 8);
	GCM_PUT_32BIT(len_buffer + 12, ctx->c_len * 8);

	/* Create the final y */
#ifdef PSTM_64BIT
	ctx->y_m128i = galois_hash(ctx->h_m128i, ctx->y_m128i, 
								 (unsigned char *)len_buffer, 16);
	_mm_storeu_si128((void *)(final_y), ctx->y_m128i);
#else
	temp = _mm_loadu_si128(&ctx->h_m128i);
	temp2 = _mm_loadu_si128(&ctx->y_m128i);
	_mm_storeu_si128(&ctx->y_m128i, galois_hash(temp, temp2, 
		(unsigned char *)len_buffer, 16));
	temp2 = _mm_loadu_si128(&ctx->y_m128i);
	_mm_storeu_si128((void *)(final_y), temp2);
#endif

	/* Run through GCTR to get T, old icb is not needed anymore */
	memset(iv_full, 0x00, 16);
	memcpy(iv_full, ctx->IV, 12);
	iv_full[15] = 0x01;
#ifdef PSTM_64BIT
	ctx->icb_m128i = _mm_loadu_si128((__m128i *)iv_full);
#else
	temp2 = _mm_loadu_si128((__m128i *)iv_full);
	_mm_storeu_si128(&ctx->icb_m128i, temp2);
#endif

	/* Create last ciphertext */
	galois_counter(ctx, digest, final_y, 16);
}

static void gcm_transform(psAesCipher_t *ctx, unsigned char *dest,
				  const unsigned char *src, size_t len, unsigned char *iv)
{
	unsigned char iv_full[16];

	if (len == 0) {
		return;
	}
	memcpy(ctx->IV, iv, 12);
	if (!ctx->cipher_started) {
		/* Create IV */
		memset(iv_full, 0x00, 16);
		memcpy(iv_full, iv, 12);
		iv_full[15] = 0x02;
#ifdef PSTM_64BIT
		ctx->icb_m128i = _mm_loadu_si128((__m128i *)iv_full);
#else
		_mm_storeu_si128(&ctx->icb_m128i, _mm_loadu_si128((__m128i *)iv_full));
#endif
		
		ctx->cipher_started = 1;
	}

	if (ctx->key.type == AES_ENCRYPT) {
		/* Create ciphertext */
		galois_counter(ctx, dest, src, len);
		/* Update auth tag */
		gcm_update(ctx, dest, len);
    } else {
		/* Update auth tag */
		gcm_update(ctx, (unsigned char *)src, len);
		/* Create ciphertext */
		galois_counter(ctx, dest, src, len);    
	}
	/* Update authenticated and encrypted (AEAD) len */
	ctx->c_len += len;
}

#endif /* USE_AES_GCM_EXTERNAL */

/******************************************************************************/

#endif /* USE_AESNI_CRYPTO */

/******************************************************************************/

