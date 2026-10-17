/* SPDX-License-Identifier: MIT */
// Bounded image; immutable dedicated and shared resident-session ioctls only.
// Fixed Qwen3-0.6B GQA tile: two query heads sharing one KV head, <=256 keys.
// QK products and PV accumulation execute on the 32-lane Tensix SFPU.
// Reductions/softmax execute on the card's TRISC0, not the host CPU.
// No NoC, PCIe, DRAM, firmware, arbitrary pointers, allocation or OS calls.
#include "PioneerAttentionMath.h"
#ifndef PIONEER_ATTENTION_EXACT_HALF_PRODUCT
#define PIONEER_ATTENTION_EXACT_HALF_PRODUCT 0
#endif
#ifndef PIONEER_ATTENTION_CACHED_QUERY
#define PIONEER_ATTENTION_CACHED_QUERY 0
#endif

static inline volatile uint32_t* Words(uint32_t address)
{
	return reinterpret_cast<volatile uint32_t*>(address);
}
static inline void Fence() { asm volatile("fence" ::: "memory"); }
static inline void Emit(uint32_t instruction) { *Words(0xffe40000) = instruction; }
extern "C" void attention_fault_park() __attribute__((noreturn));

static void Drain()
{
	volatile uint32_t* ack = Words(0xffe000fc);
	*ack = 0;
	(void)*ack;
	Fence();
	Emit(0xa2101816); // qualified STALLWAIT: CFGEXU/unpack/FPU/SFPU idle
	Emit(0x4500017e); // SETDMAREG: GPR63 low = 1, after wait
	for (unsigned i = 0; i < 1000000; i++) {
		if (*ack) { Fence(); return; }
	}
	attention_fault_park(); // no completion / no reset authorization
}

static void Config(unsigned index, uint32_t value)
{
	volatile uint32_t* reg = Words(0xffef0000 + 4 * index);
	*reg = value;
	if (*reg != value) attention_fault_park();
}

static void Setup()
{
	Emit(0xb2000000);
	Drain();
	for (unsigned i = 1; i < 68; i++) Emit(0xb2000000 | (i << 16));
	Emit(0x3700000f); // zero all address increments and reset RWCs
	Emit(0x8a000002); // all SFPU lanes enabled, no stale conditional state
	Drain();
	Config(0, 0x4231); Config(1, 0x20220000);
	Config(2, 0); Config(3, 0); // converted IEEE FP32 RISC Dst access
	Config(6, 0); Config(7, 0); Config(220, 0);
	Fence();
}

// Fixed SFPLOAD/SFPSTORE lane mapping: even columns of four rows.
static inline unsigned LaneOffset(unsigned lane)
{
	return (lane / 8) * 16 + (lane % 8) * 2;
}
static void WriteQuery(const float* query)
{
	volatile uint32_t* dst = Words(0xffbd8000);
	for (unsigned i = 0; i < 32; i++)
		dst[LaneOffset(i)] = PIONEER_ATTENTION_CACHED_QUERY?PioneerAttentionBits(query[i]):PioneerAttentionBits(PioneerAttentionHalf(PioneerAttentionToHalf(query[i])));
	(void)dst[LaneOffset(31)]; Fence();
}
static void WriteHalf(const uint16_t* input, unsigned row)
{
	volatile uint32_t* dst = Words(0xffbd8000 + row * 64);
	for (unsigned i = 0; i < 32; i++) dst[LaneOffset(i)] = PioneerAttentionBits(PioneerAttentionHalf(input[i]));
	(void)dst[LaneOffset(31)]; Fence();
}

// All arithmetic constants are trusted instructions, never supplied by host.
// This is the previously tested Q8 exact-FMA sequence, with a0,b1,c2.
// Blackhole SFPMAD is only partially fused; do not substitute a plain MAD.
extern "C" const uint32_t attention_exact_begin[], attention_exact_end[];
static void AccurateMad()
{
	for (const uint32_t* p = attention_exact_begin; p != attention_exact_end; p++) Emit(*p);
}

static float Dot(const float* query, const uint16_t* key)
{
	float high = 0, low = 0;
	for (unsigned block = 0; block < 4; block++) {
		WriteQuery(query + block * 32); WriteHalf(key + block * 32, 4);
		Emit(0x70030000); Emit(0x70130004); // q0, k1
		Emit(0x702b0000); // c2 = 0, no stale accumulator
		// Q and K have BOTH been converted to IEEE half. Their product has
		// at most22 significant bits and is exactly representable in FP32,
		// including half subnormals. The separate whole-decoder candidate
		// can use the already-qualified standalone SFPMAD product; general
		// Q8 dot*scale+accumulator still REQUIRES the exact-FMA sequence.
		if (PIONEER_ATTENTION_EXACT_HALF_PRODUCT) Emit(0x84001900);
		else AccurateMad(); // exact RN(q*k) -> 0
		Emit(0x72030008); Drain(); // result -> rows8..11
		volatile uint32_t* dst = Words(0xffbd8200);
		for (unsigned i = 0; i < 32; i++) PioneerAttentionAdd(PioneerAttentionFloat(dst[LaneOffset(i)]), high, low);
	}
	return (high + low) * 0.0883883461356163f;
}

static void WeightedValues(const uint16_t* values, const float* probabilities,
	unsigned tokens, float* output)
{
	for (unsigned block = 0; block < 4; block++) {
		Emit(0x702b0000); Emit(0x703b0000); Drain(); // independent hi2/lo3 for each 32 dims
		for (unsigned t = 0; t < tokens; t++) {
			WriteHalf(values + t * 128 + block * 32, 0);
			Emit(0x70030000);
			uint32_t p = PioneerAttentionBits(probabilities[t]);
			Emit(0x71100000 | (p >> 16)); Emit(0x711a0000 | (p & 0xffff));
			// Half operands have an exact FP32 product. Compensated FP32
			// additions retain the low part of the CPU's double reduction.
			Emit(0x84001940); // product4 = value0 * probability1 + zero9
			Emit(0x850a2450); // sum5 = high2 + product4
			Emit(0x850a5262); // virtual6 = sum5 - high2
			Emit(0x850a5672); // temporary7 = sum5 - virtual6
			Emit(0x850a2772); // error7 = high2 - temporary7
			Emit(0x850a4662); // temporary6 = product4 - virtual6
			Emit(0x850a7670); // error7 += temporary6
			Emit(0x850a3730); // low3 += error7
			Emit(0x7c000520); Drain(); // high2 = sum5
		}
		Emit(0x850a2300); Emit(0x72030008); Drain(); // result0 = high2 + low3
		volatile uint32_t* dst = Words(0xffbd8200);
		for (unsigned i = 0; i < 32; i++) output[block * 32 + i] = PioneerAttentionFloat(dst[LaneOffset(i)]);
	}
}

extern "C" unsigned attention_main()
{
	// Descriptor is data-only and has no addresses, code or shape parameters.
	volatile uint32_t* descriptor = Words(0x74000);
	unsigned tokens = descriptor[1];
	if (descriptor[0] != 0x314e5441 || !tokens || tokens > 256
		|| descriptor[2] != 1 || descriptor[3] != 0) return 0;
	const float* query = reinterpret_cast<const float*>(0x90000);
	const uint16_t* keys = reinterpret_cast<const uint16_t*>(0x92000);
	const uint16_t* values = reinterpret_cast<const uint16_t*>(0xa2000);
	float* output = reinterpret_cast<float*>(0xb2000);
	// Validate all inputs before any SFPU/configuration/output mutation.
	for (unsigned i = 0; i < 256; i++)
		if ((PioneerAttentionBits(query[i]) & 0x7fffffff) > 0x477fe000) return 0;
	for (unsigned i = 0; i < tokens * 128; i++)
		if ((keys[i] & 0x7c00) == 0x7c00 || (values[i] & 0x7c00) == 0x7c00) return 0;
#if PIONEER_ATTENTION_CACHED_QUERY
	// Query rounding is independent of the token/key. Prepare all256 once,
	// then reuse EXACT same FP32-expanded half bits for every dot product.
	// Existing separately backed8KiB query owner:90800..90c00 is disjoint
	// from query90000..90400,current KV90400..90600 and proof90600..90660.
	// GLU uses the same scratch only in a different,proved-idle job.
	float* preparedQuery=reinterpret_cast<float*>(0x90800);
	for (unsigned i=0;i<256;i++) preparedQuery[i]=PioneerAttentionHalf(PioneerAttentionToHalf(query[i]));
	query=preparedQuery;
#endif
	Setup();
	for (unsigned head = 0; head < 2; head++) {
		float scores[256], maximum = -3.4028234663852886e38f;
		for (unsigned t = 0; t < tokens; t++) {
			scores[t] = Dot(query + head * 128, keys + t * 128);
			if (scores[t] > maximum) maximum = scores[t];
		}
		float denominator = 0, denominatorLow = 0;
		for (unsigned t = 0; t < tokens; t++) {
			scores[t] = PioneerAttentionExp(scores[t] - maximum);
			PioneerAttentionAdd(scores[t], denominator, denominatorLow);
		}
		float inverse = PioneerAttentionReciprocal(denominator, denominatorLow);
		for (unsigned t = 0; t < tokens; t++) scores[t] = PioneerAttentionHalf(PioneerAttentionToHalf(scores[t] * inverse));
		WeightedValues(values, scores, tokens, output + head * 128);
	}
	Drain();
	Fence();
	return tokens;
}
