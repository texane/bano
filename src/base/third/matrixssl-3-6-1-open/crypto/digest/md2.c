/*
 *	md2.c
 *	Release $Name: MATRIXSSL-3-6-1-OPEN $
 *
 *	MD2 hash implementation
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

/******************************************************************************/
#ifdef USE_MD2

static const unsigned char PI_SUBST[256] = {
  41, 46, 67, 201, 162, 216, 124, 1, 61, 54, 84, 161, 236, 240, 6,
  19, 98, 167, 5, 243, 192, 199, 115, 140, 152, 147, 43, 217, 188,
  76, 130, 202, 30, 155, 87, 60, 253, 212, 224, 22, 103, 66, 111, 24,
  138, 23, 229, 18, 190, 78, 196, 214, 218, 158, 222, 73, 160, 251,
  245, 142, 187, 47, 238, 122, 169, 104, 121, 145, 21, 178, 7, 63,
  148, 194, 16, 137, 11, 34, 95, 33, 128, 127, 93, 154, 90, 144, 50,
  39, 53, 62, 204, 231, 191, 247, 151, 3, 255, 25, 48, 179, 72, 165,
  181, 209, 215, 94, 146, 42, 172, 86, 170, 198, 79, 184, 56, 210,
  150, 164, 125, 182, 118, 252, 107, 226, 156, 116, 4, 241, 69, 157,
  112, 89, 100, 113, 135, 32, 134, 91, 207, 101, 230, 45, 168, 2, 27,
  96, 37, 173, 174, 176, 185, 246, 28, 70, 97, 105, 52, 64, 126, 15,
  85, 71, 163, 35, 221, 81, 175, 58, 195, 92, 249, 206, 186, 197,
  234, 38, 44, 83, 13, 110, 133, 40, 132, 9, 211, 223, 205, 244, 65,
  129, 77, 82, 106, 220, 55, 200, 108, 193, 171, 250, 36, 225, 123,
  8, 12, 189, 177, 74, 120, 136, 149, 139, 227, 99, 232, 109, 233,
  203, 213, 254, 59, 0, 29, 57, 242, 239, 183, 14, 102, 88, 208, 228,
  166, 119, 114, 248, 235, 117, 75, 10, 49, 68, 80, 180, 143, 237,
  31, 26, 219, 153, 141, 51, 159, 17, 131, 20
};

/* adds 16 bytes to the checksum */
static void md2_update_chksum(psDigestContext_t *md)
{
	int32 j;
	unsigned char L;
	L = md->md2.chksum[15];
	for (j = 0; j < 16; j++) {

/*
		caution, the RFC says its "C[j] = S[M[i*16+j] xor L]" but the reference
		source code [and test vectors] say otherwise.
*/
		L = (md->md2.chksum[j] ^= PI_SUBST[(int32)(md->md2.buf[j] ^ L)] & 255);
	}
}

static void md2_compress(psDigestContext_t *md)
{
	int32 j, k;
	unsigned char t;
   
	/* copy block */
	for (j = 0; j < 16; j++) {
		md->md2.X[16+j] = md->md2.buf[j];
		md->md2.X[32+j] = md->md2.X[j] ^ md->md2.X[16+j];
	}

	t = (unsigned char)0;

	/* do 18 rounds */
	for (j = 0; j < 18; j++) {
		for (k = 0; k < 48; k++) {
			t = (md->md2.X[k] ^= PI_SUBST[(int32)(t & 255)]);
		}
		t = (t + (unsigned char)j) & 255;
	}
}

void psMd2Init(psDigestContext_t *md)
{
	psAssert(md != NULL);

	/* MD2 uses a zero'ed state... */
	memset(md->md2.X, 0x0, sizeof(md->md2.X));
	memset(md->md2.chksum, 0x0, sizeof(md->md2.chksum));
	memset(md->md2.buf, 0x0, sizeof(md->md2.buf));
	md->md2.curlen = 0;
}

int32 psMd2Update(psDigestContext_t *md, const unsigned char *buf, uint32 len)
{
	uint32 n;

    psAssert(md != NULL);
    psAssert(buf != NULL);
    if (md->md2.curlen > sizeof(md->md2.buf)) {
		psTraceCrypto("psMd2Update error\n");
		return PS_LIMIT_FAIL;
	}                                                                                       
	while (len > 0) {
        n = min(len, (16 - md->md2.curlen));
        memcpy(md->md2.buf + md->md2.curlen, buf, (size_t)n);
        md->md2.curlen += n;
        buf            += n;
        len            -= n;

        /* is 16 bytes full? */
        if (md->md2.curlen == 16) {
            md2_compress(md);
            md2_update_chksum(md);
            md->md2.curlen = 0;
        }
    }
    return PS_SUCCESS;
}

int32 psMd2Final(psDigestContext_t * md, unsigned char *hash)
{
    uint32 i, k;

    psAssert(md != NULL);
    psAssert(hash != NULL);

    if (md->md2.curlen >= sizeof(md->md2.buf)) {
		psTraceCrypto("psMd2Final error\n");
		return PS_LIMIT_FAIL;
    }


    /* pad the message */
    k = 16 - md->md2.curlen;
    for (i = md->md2.curlen; i < 16; i++) {
        md->md2.buf[i] = (unsigned char)k;
    }

    /* hash and update */
    md2_compress(md);
    md2_update_chksum(md);

    /* hash checksum */
    memcpy(md->md2.buf, md->md2.chksum, 16);
    md2_compress(md);

    /* output is lower 16 bytes of X */
    memcpy(hash, md->md2.X, 16);

    memset(md, 0x0, sizeof(psDigestContext_t));
    return PS_SUCCESS;
}

#endif /* USE_MD2 */
/******************************************************************************/

