/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_ATTENTION_MATH_H
#define PIONEER_ATTENTION_MATH_H
#if defined(PIONEER_ATTENTION_FREESTANDING)
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
#else
#include <stdint.h>
#endif

// Scalar numerical model and elementary math for an offline on-card candidate.
// Never registered as a host backend or used as an inference fallback.
enum { PIONEER_ATTN_WIDTH = 128, PIONEER_ATTN_HEADS = 16,
	PIONEER_ATTN_KV_HEADS = 8, PIONEER_ATTN_TOKENS = 256, PIONEER_ATTN_LAYERS = 28 };

static inline uint32_t PioneerAttentionBits(float value)
{
	uint32_t bits; __builtin_memcpy(&bits, &value, 4); return bits;
}
static inline float PioneerAttentionFloat(uint32_t bits)
{
	float value; __builtin_memcpy(&value, &bits, 4); return value;
}
static inline float PioneerAttentionHalf(uint16_t value)
{
	uint32_t sign = uint32_t(value & 0x8000) << 16;
	uint32_t mantissa = value & 1023, exponent = (value >> 10) & 31;
	if (!exponent) {
		if (!mantissa) return PioneerAttentionFloat(sign);
		int e = -14;
		while (!(mantissa & 1024)) { mantissa <<= 1; e--; }
		return PioneerAttentionFloat(sign | (uint32_t(e + 127) << 23) | ((mantissa & 1023) << 13));
	}
	return PioneerAttentionFloat(sign | ((exponent == 31 ? 255 : exponent + 112) << 23) | (mantissa << 13));
}

static inline float PioneerAttentionReciprocal(float value)
{
	// Positive softmax denominator is in [1,256]. No unsupported fdiv.s.
	uint32_t bits = PioneerAttentionBits(value);
	if (!(bits & 0x7fffff)) return PioneerAttentionFloat((254U - (bits >> 23)) << 23);
	float inverse = PioneerAttentionFloat(0x7ef311c3U - bits);
	for (unsigned i = 0; i < 4; i++) inverse = inverse * (2.0f - value * inverse);
	return inverse;
}

static inline float PioneerAttentionExp(float value)
{
	// Stable softmax supplies x<=0. Values below -80 contribute <2e-35.
	if (value <= -80.0f) return 0;
	float magnitude = -value * 1.4426950408889634f;
	uint32_t bits = PioneerAttentionBits(magnitude);
	int exponent = int((bits >> 23) & 255) - 127;
	unsigned rounded = 0;
	if (exponent >= -1) {
		unsigned mantissa = (bits & 0x7fffff) | 0x800000;
		unsigned shift = unsigned(23 - exponent);
		rounded = (mantissa + (1U << (shift - 1))) >> shift;
	}
	int n = -int(rounded);
	// Split ln(2) keeps the range-reduction products exact at this bound.
	float r = (value - float(n) * 0.693145751953125f) - float(n) * 0.000001428606765330187f;
	float result = 0.001388888888888889f;
	result = result * r + 0.008333333333333333f;
	result = result * r + 0.041666666666666667f;
	result = result * r + 0.166666666666666667f;
	result = result * r + 0.5f;
	result = result * r + 1.0f;
	result = result * r + 1.0f;
	return result * PioneerAttentionFloat(uint32_t(n + 127) << 23);
}

static inline bool PioneerAttentionHead(const float* query, const uint16_t* keys,
	const uint16_t* values, unsigned tokens, unsigned stride, float* output)
{
	if (!query || !keys || !values || !output || !tokens || tokens > PIONEER_ATTN_TOKENS
		|| (stride != PIONEER_ATTN_WIDTH && stride != PIONEER_ATTN_WIDTH * PIONEER_ATTN_KV_HEADS)) return false;
	for (unsigned d = 0; d < PIONEER_ATTN_WIDTH; d++)
		if ((PioneerAttentionBits(query[d]) & 0x7fffffff) > 0x477fe000U) return false;
	for (unsigned t = 0; t < tokens; t++) for (unsigned d = 0; d < PIONEER_ATTN_WIDTH; d++)
		if ((keys[t * stride + d] & 0x7c00) == 0x7c00 || (values[t * stride + d] & 0x7c00) == 0x7c00) return false;
	float scores[PIONEER_ATTN_TOKENS], maximum = -3.4028234663852886e38f;
	for (unsigned t = 0; t < tokens; t++) {
		float sum = 0;
		for (unsigned d = 0; d < PIONEER_ATTN_WIDTH; d++) sum += query[d] * PioneerAttentionHalf(keys[t * stride + d]);
		scores[t] = sum * 0.0883883461356163f;
		if (scores[t] > maximum) maximum = scores[t];
	}
	float denominator = 0;
	for (unsigned t = 0; t < tokens; t++) {
		scores[t] = PioneerAttentionExp(scores[t] - maximum); denominator += scores[t];
	}
	float inverse = PioneerAttentionReciprocal(denominator);
	for (unsigned d = 0; d < PIONEER_ATTN_WIDTH; d++) {
		float sum = 0;
		for (unsigned t = 0; t < tokens; t++) sum += (scores[t] * inverse) * PioneerAttentionHalf(values[t * stride + d]);
		output[d] = sum;
	}
	return true;
}
#endif
