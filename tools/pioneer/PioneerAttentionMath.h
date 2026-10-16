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

// IEEE binary16 RNE, including subnormals. The older matrix operand helper
// intentionally flushes subnormals and must not be used for probabilities.
static inline uint16_t PioneerAttentionToHalf(float value)
{
	uint32_t bits = PioneerAttentionBits(value), magnitude = bits & 0x7fffffff;
	uint16_t sign = uint16_t((bits >> 16) & 0x8000);
	if (magnitude >= 0x7f800000) return sign | uint16_t(magnitude == 0x7f800000 ? 0x7c00 : 0x7e00);
	if (magnitude >= 0x477ff000) return sign | 0x7c00;
	if (magnitude < 0x33000000) return sign;
	if (magnitude < 0x38800000) {
		unsigned shift = 126 - (magnitude >> 23);
		uint32_t mantissa = (magnitude & 0x7fffff) | 0x800000;
		uint32_t rounded = mantissa >> shift;
		uint32_t remainder = mantissa & ((1U << shift) - 1), halfway = 1U << (shift - 1);
		return sign | uint16_t(rounded + (remainder > halfway || (remainder == halfway && (rounded & 1))));
	}
	uint32_t rounded = magnitude + 0xfff + ((magnitude >> 13) & 1);
	return sign | uint16_t((rounded - 0x38000000) >> 13);
}

// Half*half products are exact binary32. A two-float expansion retains the
// low part of bounded reductions, matching the pinned CPU's double sum
// without requiring unsupported double instructions on the card.
static inline void PioneerAttentionAdd(float value, float& high, float& low)
{
	float sum = high + value, virtualValue = sum - high;
	low += (high - (sum - virtualValue)) + (value - virtualValue);
	high = sum;
}

struct PioneerAttentionWide { float high, low; };
static inline PioneerAttentionWide PioneerAttentionWideAdd(PioneerAttentionWide a, PioneerAttentionWide b)
{
	float high = a.high + b.high, v = high - a.high;
	float low = ((a.high - (high - v)) + (b.high - v)) + (a.low + b.low);
	float normalized = high + low, w = normalized - high;
	return {normalized, (high - (normalized - w)) + (low - w)};
}
static inline PioneerAttentionWide PioneerAttentionWideMultiply(PioneerAttentionWide a, PioneerAttentionWide b)
{
	float as = a.high * 4097.0f, ah = as - (as - a.high), al = a.high - ah;
	float bs = b.high * 4097.0f, bh = bs - (bs - b.high), bl = b.high - bh;
	float high = a.high * b.high;
	float low = (((ah * bh - high) + ah * bl) + al * bh) + al * bl;
	low += (a.high * b.low + a.low * b.high) + a.low * b.low;
	return PioneerAttentionWideAdd({high, 0}, {low, 0});
}

static inline float PioneerAttentionReciprocal(float value, float low = 0)
{
	// Positive softmax denominator is in [1,256]. No unsupported fdiv.s.
	uint32_t bits = PioneerAttentionBits(value);
	if (!(bits & 0x7fffff) && !low) return PioneerAttentionFloat((254U - (bits >> 23)) << 23);
	float inverse = PioneerAttentionFloat(0x7ef311c3U - bits);
	for (unsigned i = 0; i < 4; i++) inverse = inverse * (2.0f - value * inverse);
	// A float-only Newton step can oscillate between adjacent floats.
	// Retain the denominator's low part and round one wide correction.
	PioneerAttentionWide product = PioneerAttentionWideMultiply({value, low}, {inverse, 0});
	PioneerAttentionWide error = PioneerAttentionWideAdd({1, 0}, {-product.high, -product.low});
	PioneerAttentionWide result = PioneerAttentionWideAdd({inverse, 0}, PioneerAttentionWideMultiply({inverse, 0}, error));
	return result.high + result.low;
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
	// Degree-10 exp on [-ln(2)/2,ln(2)/2], evaluated in two floats.
	// Single-float exp error can cross a subsequent binary16 midpoint.
	// Constants are split offline, not computed using card double ops.
	PioneerAttentionWide shift = PioneerAttentionWideMultiply({float(n), 0}, {0.6931471824645996f, -1.904654323148236e-9f});
	PioneerAttentionWide r = PioneerAttentionWideAdd({value, 0}, {-shift.high, -shift.low});
	static const uint32_t coefficients[11][2] = {
		{0x3f800000,0}, {0x3f800000,0}, {0x3f000000,0},
		{0x3e2aaaab,0xb1aaaaab}, {0x3d2aaaab,0xb0aaaaab}, {0x3c088889,0xafeeeeef},
		{0x3ab60b61,0xae13e93f}, {0x39500d01,0xac3fcbfd}, {0x37d00d01,0xaabfcbfd},
		{0x3638ef1d,0x292ad8e6}, {0x3493f27e,0xa808760a}
	};
	PioneerAttentionWide result = {PioneerAttentionFloat(coefficients[10][0]), PioneerAttentionFloat(coefficients[10][1])};
	for (int i = 9; i >= 0; i--) result = PioneerAttentionWideAdd(PioneerAttentionWideMultiply(result, r),
		{PioneerAttentionFloat(coefficients[i][0]), PioneerAttentionFloat(coefficients[i][1])});
	return (result.high + result.low) * PioneerAttentionFloat(uint32_t(n + 127) << 23);
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
		float high = 0, low = 0;
		for (unsigned d = 0; d < PIONEER_ATTN_WIDTH; d++) {
			float q = PioneerAttentionHalf(PioneerAttentionToHalf(query[d]));
			PioneerAttentionAdd(q * PioneerAttentionHalf(keys[t * stride + d]), high, low);
		}
		scores[t] = (high + low) * 0.0883883461356163f;
		if (scores[t] > maximum) maximum = scores[t];
	}
	float denominator = 0, denominatorLow = 0;
	for (unsigned t = 0; t < tokens; t++) {
		scores[t] = PioneerAttentionExp(scores[t] - maximum);
		PioneerAttentionAdd(scores[t], denominator, denominatorLow);
	}
	float inverse = PioneerAttentionReciprocal(denominator, denominatorLow);
	for (unsigned t = 0; t < tokens; t++) scores[t] = PioneerAttentionHalf(PioneerAttentionToHalf(scores[t] * inverse));
	for (unsigned d = 0; d < PIONEER_ATTN_WIDTH; d++) {
		float high = 0, low = 0;
		for (unsigned t = 0; t < tokens; t++) PioneerAttentionAdd(scores[t] * PioneerAttentionHalf(values[t * stride + d]), high, low);
		output[d] = high + low;
	}
	return true;
}
#endif
