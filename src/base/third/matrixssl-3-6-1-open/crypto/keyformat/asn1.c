/*
 *	asn1.c
 *	Release $Name: MATRIXSSL-3-6-1-OPEN $
 *
 *	DER/BER coding
 */
/*
 *	Copyright (c) 2013-2014 INSIDE Secure Corporation
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

#include "../cryptoApi.h"

/* Define this here to avoid including circular limits.h on some platforms */
#ifndef INT_MAX
#define INT_MAX		2147483647
#endif

/******************************************************************************/
/*
	On success, p will be updated to point to first character of value and
	valLen will contain number of bytes in value.
	
	Indefinite length formats return ASN_UNKNOWN_LEN and *valLen will simply
	be updated with the overall remaining length
*/
int32 getAsnLength(unsigned char **p, uint32 size, uint32 *valLen)
{
	unsigned char	*c, *end;
	uint32			len, olen;

	c = *p;
	end = c + size;
	*valLen = 0;
	if (end - c < 1) {
		psTraceCrypto("getAsnLength called on empty buffer\n");
		return PS_LIMIT_FAIL;
	}
/*
	If the length byte has high bit only set, it's an indefinite length
*/
	if (*c == 0x80) {
		//psTraceCrypto("Danger: ASN indefinite length\n");
		c++;
		*p = c;
		*valLen = size - 1;
		return ASN_UNKNOWN_LEN;
	}
/*
	If the high bit is set, the lower 7 bits represent the number of 
	bytes that follow and represent length
	If the high bit is not set, the lower 7 represent the actual length
*/
	len = *c & 0x7F;
	if (*(c++) & 0x80) {
/*
		Make sure there aren't more than 4 bytes of length specifier,
		and that we have that many bytes left in the buffer
*/
		if (len > sizeof(int32) || len == 0x7f || (uint32)(end - c) < len) {
			psTraceCrypto("Malformed stream in getAsnLength\n");
			return PS_LIMIT_FAIL;
		}
		olen = 0;
		while (len-- > 0) {
			olen = (olen << 8) | *c;
			c++;
		}
		if (olen > INT_MAX) {
			psTraceCrypto("Malformed stream in getAsnLength\n");
			return PS_LIMIT_FAIL;
		}
		len = olen;
	}
	*p = c;
	*valLen = len;
	return PS_SUCCESS;
}

/******************************************************************************/
/*
	Callback to extract a big int (stream of bytes) from the DER stream
*/
int32 getAsnBig(psPool_t *pool, unsigned char **pp, uint32 len, pstm_int *big)
{
	unsigned char	*p = *pp;
	uint32			vlen;	

	if (len < 1 || *(p++) != ASN_INTEGER ||
			getAsnLength(&p, len - 1, &vlen) < 0 || (len - 1) < vlen)  {
		psTraceCrypto("ASN getBig failed\n");
		return PS_PARSE_FAIL;
	}
#ifndef DISABLE_PSTM
/*
	Make a smart size since we know the length
*/
	if (pstm_init_for_read_unsigned_bin(pool, big, vlen) != PSTM_OKAY) {
		return PS_MEM_FAIL;
	}
	if (pstm_read_unsigned_bin(big, p, vlen) != 0) {
		pstm_clear(big);
		psTraceCrypto("ASN getBig failed\n");
		return PS_PARSE_FAIL;
	}
#else
	return PS_UNSUPPORTED_FAIL;
#endif /* DISABLE_PSTM */
	p += vlen;
	*pp = p;
	return PS_SUCCESS;
}

/******************************************************************************/
/*
	Callback to extract a sequence length from the DER stream
	Verifies that 'len' bytes are >= 'seqlen'
	Move pp to the first character in the sequence
*/
/* #define DISABLE_STRICT_ASN_LENGTH_CHECK */
int32 getAsnSequence(unsigned char **pp, uint32 len, uint32 *seqlen)
{
	int32			rc;
	unsigned char	*p = *pp;

	rc = PS_SUCCESS;
	if (len < 1 || *(p++) != (ASN_SEQUENCE | ASN_CONSTRUCTED) || 
			((rc = getAsnLength(&p, len - 1, seqlen)) < 0)) {
		psTraceCrypto("ASN getSequence failed\n");	
		return PS_PARSE_FAIL;
	}
#ifndef DISABLE_STRICT_ASN_LENGTH_CHECK
	if (len < *seqlen) {
		/* It isn't cool but some encodings have an outer length layer that
			is smaller than the inner.  Normally you'll want this check but if
			you're hitting this case, you could try skipping it to see if there
			is an error in the encoding */
		psTraceCrypto("ASN_SEQUENCE parse found length greater than total\n");
		psTraceCrypto("Could try enabling DISABLE_STRICT_ASN_LENGTH_CHECK\n");
		return PS_LIMIT_FAIL;
	}
#endif	
	*pp = p;
	return rc;
}

/******************************************************************************/
/*
	Extract a set length from the DER stream
*/
int32 getAsnSet(unsigned char **pp, uint32 len, uint32 *setlen)
{
	int32			rc;
	unsigned char	*p = *pp;

	rc = PS_SUCCESS;
	if (len < 1 || *(p++) != (ASN_SET | ASN_CONSTRUCTED) || 
			((rc = getAsnLength(&p, len - 1, setlen)) < 0) || len < *setlen) {
		psTraceCrypto("ASN getSet failed\n");
		return PS_PARSE_FAIL;
	}
	*pp = p;
	return rc;
}

/******************************************************************************/
/*
	Get an integer
*/
int32 getAsnInteger(unsigned char **pp, uint32 len, int32 *val)
{
	unsigned char	*p = *pp, *end;
	uint32			ui,	vlen;
	int32			slen;

	end = p + len;
	if (len < 1 || *(p++) != ASN_INTEGER ||
			getAsnLength(&p, len - 1, &vlen) < 0) {
		psTraceCrypto("ASN getInteger failed from the start\n");
		return PS_PARSE_FAIL;
	}
/*
	This check prevents us from having a big positive integer where the 
	high bit is set because it will be encoded as 5 bytes (with leading 
	blank byte).  If that is required, a getUnsigned routine should be used
*/
	if (vlen > sizeof(int32) || (uint32)(end - p) < vlen) {
		psTraceCrypto("ASN getInteger had limit failure\n");
		return PS_LIMIT_FAIL;
	}
	ui = 0;
/*
	If high bit is set, it's a negative integer, so perform the two's compliment
	Otherwise do a standard big endian read (most likely case for RSA)
*/
	if (*p & 0x80) {
		while (vlen-- > 0) {
			ui = (ui << 8) | (*p ^ 0xFF);
			p++;
		}
		slen = (int32)ui;
		slen++;
		slen = -slen;
		*val = slen;
	} else {
		while (vlen-- > 0) {
			ui = (ui << 8) | *p;
			p++;
		}
		*val = (int32)ui;
	}
	*pp = p;
	return PS_SUCCESS;
}

/******************************************************************************/
/*
	Implementation specific OID parser
*/
int32 getAsnAlgorithmIdentifier(unsigned char **pp, uint32 len, int32 *oi,
				int32 *paramLen)
{
	unsigned char   *p = *pp, *end;
	int32           rc;
	uint32			llen;

	end = p + len;
	if (len < 1 || getAsnSequence(&p, len, &llen) < 0) {
		psTraceCrypto("getAsnAlgorithmIdentifier failed on inital parse\n");
		return PS_PARSE_FAIL;
	}
	/* Always checks for parameter length */
	rc = getAsnOID(&p, llen, oi, 1, paramLen);
	*pp = p;
	return rc;
}

int32 getAsnOID(unsigned char **pp, uint32 len, int32 *oi,
				int32 checkForParams, int32 *paramLen)
{
	unsigned char   *p = *pp, *end;
	int32           plen;
	uint32			arcLen;

	end = p + len;
	plen = end - p;
	if (*(p++) != ASN_OID || getAsnLength(&p, (uint32)(end - p), &arcLen) < 0
			|| len < arcLen) {
		psTraceCrypto("Malformed algorithmId 2\n");
		return PS_PARSE_FAIL;
	}
	if (end - p < 2) {
		psTraceCrypto("Malformed algorithmId 3\n");
		return PS_LIMIT_FAIL;
	}
	*oi = 0;
	while (arcLen-- > 0) {
		*oi += (int32)*p;
		p++;
	}

	if (checkForParams) {
		plen -= (end - p);
		*paramLen = len - plen;
		if (*p != ASN_NULL) {
			*pp = p;
			/* paramLen tells whether params exist or completely missing (0) */
			if (*paramLen > 0) {
				psTraceIntCrypto("OID %d has parameters to process\n", *oi);
			}
			return PS_SUCCESS;
		}
		/* NULL parameter case.  Somewhat common.  Skip it for the caller */
		if (end - p < 2) {
			psTraceCrypto("Malformed algorithmId 4\n");
			return PS_LIMIT_FAIL;
		}
		if (*paramLen < 2) {
			psTraceCrypto("Malformed algorithmId 5\n");
			return PS_LIMIT_FAIL;
		}
		*paramLen -= 2; /* 1 for the OID tag and 1 for the NULL */
		*pp = p + 2;
	} else {
		*paramLen = 0;
		*pp = p;
	}
	return PS_SUCCESS;
}


/* Only handling a constructed octet string type that is made up of
	primitive segments */
int32 getAsnConstructedOctetString(psPool_t *pool, unsigned char **pp,
		uint32 len, unsigned char **outString, int32 *outStringLen)
{
	uint32			llen, totalStrLen, index;
	unsigned char   *p = *pp, *end, *constructStart;
	
	end = p + len;
	
	if (getAsnLength(&p, (uint32)(end - p), &llen) < 0 || len < llen) {
		return PS_PARSE_FAIL;
	}
	
	totalStrLen = index = 0;
	constructStart = p;
	/* First pass is to get the length and test integrity */
	while (*p == ASN_OCTET_STRING) {
		p++;
		if (getAsnLength(&p, (uint32)(end - p), &llen) < 0 || len < llen) {
			return PS_PARSE_FAIL;
		}
		totalStrLen += llen;
		p += llen;
	}
	if ((uint32)(end - p) < 2) {
		return PS_PARSE_FAIL;
	}
	if (*p++ != 0x0 || *p != 0x0) {
		return PS_PARSE_FAIL;
	}
	
	/* Second pass will do the string creation */
	p = constructStart;
	
	if ((constructStart = psMalloc(pool, totalStrLen)) == NULL) {
		return PS_MEM_FAIL;
	}
	while (*p == ASN_OCTET_STRING) {
		p++;
		if (getAsnLength(&p, (uint32)(end - p), &llen) < 0 || len < llen) {
			return PS_PARSE_FAIL;
		}
		memcpy(constructStart + index, p, llen);
		index += llen;
		p += llen;
	}
	psAssert(index == totalStrLen);
	
	p += 2; /* Already tested for 0x0, 0x0 above */
	
	*pp = p;
	*outString = constructStart;
	*outStringLen = totalStrLen;
	return PS_SUCCESS;
}

#ifdef USE_RSA	
/******************************************************************************/
/*
	Get the BIT STRING key and plug into RSA structure.
*/
int32 getAsnRsaPubKey(psPool_t *pool, unsigned char **pp, uint32 len, 
					 psRsaKey_t *pubKey)
{
	unsigned char	*p = *pp;
	uint32			pubKeyLen, seqLen;
	int32			ignore_bits;
	memset(pubKey, 0x0, sizeof(psRsaKey_t));
	if (len < 1 || (*(p++) != ASN_BIT_STRING) ||
			getAsnLength(&p, len - 1, &pubKeyLen) < 0 ||
			(len - 1) < pubKeyLen) {
		psTraceCrypto("Initial parse error in getAsnRsaPubKey\n");
		return PS_PARSE_FAIL;
	}
	
	ignore_bits = *p++;
/*
	We assume this is always zero
*/
	psAssert(ignore_bits == 0);

	if (getAsnSequence(&p, pubKeyLen, &seqLen) < 0 ||
		getAsnBig(pool, &p, seqLen, &pubKey->N) < 0 ||
		getAsnBig(pool, &p, seqLen, &pubKey->e) < 0) {
		psTraceCrypto("Secondary parse error in getAsnRsaPubKey\n");
		return PS_PARSE_FAIL;
	}
	pubKey->size = pstm_unsigned_bin_size(&pubKey->N);
	*pp = p;
	return PS_SUCCESS;
}

#endif /* USE_RSA */
/******************************************************************************/
