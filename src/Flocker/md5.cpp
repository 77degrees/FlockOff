// MD5 (Message-Digest Algorithm 5)
// Copyright Victor Breder 2024
// SPDX-License-Identifier: MIT

// Implemented from reference RFC 1321 available at
// <https://www.ietf.org/rfc/rfc1321.txt>

#include <Arduino.h>
#include "md5.h"

// from RFC 1321, Section 3.4:
#define MD5_F(X, Y, Z) (((X) & (Y)) | ((~(X)) & (Z)))
#define MD5_G(X, Y, Z) (((X) & (Z)) | (Y & (~(Z))))
#define MD5_H(X, Y, Z) ((X) ^ (Y) ^ (Z))
#define MD5_I(X, Y, Z) ((Y) ^ ((X) | (~(Z))))

static uint32_t rotl(uint32_t x, int s) 
{ 
  return (x << s) | (x >> (32 - s)); 
}

#define STEP(OP, a, b, c, d, k, s, i) do{ \
	a = b + rotl(a + OP(b, c, d) + X[k] + i, s); \
	}while(0)

#define TO_I32(x,i) ((uint32_t)(x[i]) | (uint32_t)(x[i+1]<<8) | (uint32_t)(x[i+2]<<16) | (uint32_t)(x[i+3]<<24))

static void md5_block(md5_context* ctx, const uint8_t m[64]) 
{
	assert(ctx != NULL);

	uint32_t X[16] = {
		TO_I32(m,0), TO_I32(m,4), TO_I32(m,8), TO_I32(m,12),
		TO_I32(m,16), TO_I32(m,20), TO_I32(m,24), TO_I32(m,28),
		TO_I32(m,32), TO_I32(m,36), TO_I32(m,40), TO_I32(m,44),
		TO_I32(m,48), TO_I32(m,52), TO_I32(m,56), TO_I32(m,60)
	};

	uint32_t a = ctx->a;
	uint32_t b = ctx->b;
	uint32_t c = ctx->c;
	uint32_t d = ctx->d;

	STEP(MD5_F, a, b, c, d, 0,  7, 0xd76aa478);
	STEP(MD5_F, d, a, b, c, 1, 12, 0xe8c7b756);
	STEP(MD5_F, c, d, a, b, 2, 17, 0x242070db);
	STEP(MD5_F, b, c, d, a, 3, 22, 0xc1bdceee);
	STEP(MD5_F, a, b, c, d, 4,  7, 0xf57c0faf);
	STEP(MD5_F, d, a, b, c, 5, 12, 0x4787c62a);
	STEP(MD5_F, c, d, a, b, 6, 17, 0xa8304613);
	STEP(MD5_F, b, c, d, a, 7, 22, 0xfd469501);
	STEP(MD5_F, a, b, c, d,  8,  7, 0x698098d8);
	STEP(MD5_F, d, a, b, c,  9, 12, 0x8b44f7af);
	STEP(MD5_F, c, d, a, b, 10, 17, 0xffff5bb1);
	STEP(MD5_F, b, c, d, a, 11, 22, 0x895cd7be);
	STEP(MD5_F, a, b, c, d, 12,  7, 0x6b901122);
	STEP(MD5_F, d, a, b, c, 13, 12, 0xfd987193);
	STEP(MD5_F, c, d, a, b, 14, 17, 0xa679438e);
	STEP(MD5_F, b, c, d, a, 15, 22, 0x49b40821);
	STEP(MD5_G, a, b, c, d,  1,  5, 0xf61e2562);
	STEP(MD5_G, d, a, b, c,  6,  9, 0xc040b340);
	STEP(MD5_G, c, d, a, b, 11, 14, 0x265e5a51);
	STEP(MD5_G, b, c, d, a,  0, 20, 0xe9b6c7aa);
	STEP(MD5_G, a, b, c, d,  5,  5, 0xd62f105d);
	STEP(MD5_G, d, a, b, c, 10,  9, 0x02441453);
	STEP(MD5_G, c, d, a, b, 15, 14, 0xd8a1e681);
	STEP(MD5_G, b, c, d, a,  4, 20, 0xe7d3fbc8);
	STEP(MD5_G, a, b, c, d,  9,  5, 0x21e1cde6);
	STEP(MD5_G, d, a, b, c, 14,  9, 0xc33707d6);
	STEP(MD5_G, c, d, a, b,  3, 14, 0xf4d50d87);
	STEP(MD5_G, b, c, d, a,  8, 20, 0x455a14ed);
	STEP(MD5_G, a, b, c, d, 13,  5, 0xa9e3e905);
	STEP(MD5_G, d, a, b, c,  2,  9, 0xfcefa3f8);
	STEP(MD5_G, c, d, a, b,  7, 14, 0x676f02d9);
	STEP(MD5_G, b, c, d, a, 12, 20, 0x8d2a4c8a);
	STEP(MD5_H, a, b, c, d,  5,  4, 0xfffa3942);
	STEP(MD5_H, d, a, b, c,  8, 11, 0x8771f681);
	STEP(MD5_H, c, d, a, b, 11, 16, 0x6d9d6122);
	STEP(MD5_H, b, c, d, a, 14, 23, 0xfde5380c);
	STEP(MD5_H, a, b, c, d,  1,  4, 0xa4beea44);
	STEP(MD5_H, d, a, b, c,  4, 11, 0x4bdecfa9);
	STEP(MD5_H, c, d, a, b,  7, 16, 0xf6bb4b60);
	STEP(MD5_H, b, c, d, a, 10, 23, 0xbebfbc70);
	STEP(MD5_H, a, b, c, d, 13,  4, 0x289b7ec6);
	STEP(MD5_H, d, a, b, c,  0, 11, 0xeaa127fa);
	STEP(MD5_H, c, d, a, b,  3, 16, 0xd4ef3085);
	STEP(MD5_H, b, c, d, a,  6, 23, 0x04881d05);
	STEP(MD5_H, a, b, c, d,  9,  4, 0xd9d4d039);
	STEP(MD5_H, d, a, b, c, 12, 11, 0xe6db99e5);
	STEP(MD5_H, c, d, a, b, 15, 16, 0x1fa27cf8);
	STEP(MD5_H, b, c, d, a,  2, 23, 0xc4ac5665);
	STEP(MD5_I, a, b, c, d,  0,  6, 0xf4292244);
	STEP(MD5_I, d, a, b, c,  7, 10, 0x432aff97);
	STEP(MD5_I, c, d, a, b, 14, 15, 0xab9423a7);
	STEP(MD5_I, b, c, d, a,  5, 21, 0xfc93a039);
	STEP(MD5_I, a, b, c, d, 12,  6, 0x655b59c3);
	STEP(MD5_I, d, a, b, c,  3, 10, 0x8f0ccc92);
	STEP(MD5_I, c, d, a, b, 10, 15, 0xffeff47d);
	STEP(MD5_I, b, c, d, a,  1, 21, 0x85845dd1);
	STEP(MD5_I, a, b, c, d,  8,  6, 0x6fa87e4f);
	STEP(MD5_I, d, a, b, c, 15, 10, 0xfe2ce6e0);
	STEP(MD5_I, c, d, a, b,  6, 15, 0xa3014314);
	STEP(MD5_I, b, c, d, a, 13, 21, 0x4e0811a1);
	STEP(MD5_I, a, b, c, d,  4,  6, 0xf7537e82);
	STEP(MD5_I, d, a, b, c, 11, 10, 0xbd3af235);
	STEP(MD5_I, c, d, a, b,  2, 15, 0x2ad7d2bb);
	STEP(MD5_I, b, c, d, a,  9, 21, 0xeb86d391);

	ctx->a += a;
	ctx->b += b;
	ctx->c += c;
	ctx->d += d;

	memset(X, 0, sizeof(X));
}

void md5_init(md5_context* ctx) {
	assert(ctx != NULL);
	memset(ctx, 0, sizeof(md5_context));
	// initialization values from RFC 1321, Section 3.3:
	ctx->a = 0x67452301;
	ctx->b = 0xEFCDAB89;
	ctx->c = 0x98BADCFE;
	ctx->d = 0x10325476;
}

#define TO_U8(x,o,i) do{ \
	o[i] = (x) & 0xFF; \
	o[i+1] = ((x) >> 8) & 0xFF; \
	o[i+2] = ((x) >> 16) & 0xFF; \
	o[i+3] = ((x) >> 24) & 0xFF; }while(0)

void md5_digest(md5_context* ctx, void* buffer, size_t size) {
	uint8_t* bytes = (uint8_t*)buffer;
	uint64_t message_bits = size * 8;
	ssize_t rem_size = size;
	while (rem_size > 64) {
		md5_block(ctx, bytes);
		bytes += 64;
		rem_size -= 64;
	}
	uint8_t scratch[64];
	memset(scratch, 0, 64);
	memcpy(scratch, bytes, rem_size);
	if (rem_size == 64) {
		md5_block(ctx, scratch);
		memset(scratch, 0, 64);
		scratch[0] = 0x80;
	} else {
		scratch[rem_size] = 0x80;
		if (64 - (rem_size + 1) < 8) {
			md5_block(ctx, scratch);
			memset(scratch, 0, 64);
		}
	}
	TO_U8(message_bits, scratch, 56);
	TO_U8(message_bits>>32, scratch, 60);
	md5_block(ctx, scratch);
	memset(scratch, 0x00, 64);
}

void md5_output(md5_context* ctx, uint8_t out[16]) {
	TO_U8(ctx->a, out, 0);
	TO_U8(ctx->b, out, 4);
	TO_U8(ctx->c, out, 8);
	TO_U8(ctx->d, out, 12);
}