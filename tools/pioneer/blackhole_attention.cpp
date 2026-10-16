/* SPDX-License-Identifier: MIT */
// Bounded qualification image. Explicit dedicated test ioctl only;
// not yet registered for Assistant/full-model inference.
// Fixed Qwen3-0.6B GQA tile: two query heads sharing one KV head, <=256 keys.
// QK products and PV accumulation execute on the 32-lane Tensix SFPU.
// Reductions/softmax execute on the card's TRISC0, not the host CPU.
// No NoC, PCIe, DRAM, firmware, arbitrary pointers, allocation or OS calls.
#include "PioneerAttentionMath.h"

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
	for (unsigned i = 0; i < 32; i++) dst[LaneOffset(i)] = PioneerAttentionBits(query[i]);
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
	float sum = 0;
	for (unsigned block = 0; block < 4; block++) {
		WriteQuery(query + block * 32); WriteHalf(key + block * 32, 4);
		Emit(0x70030000); Emit(0x70130004); // q0, k1
		Emit(0x702b0000); // c2 = 0, no stale accumulator
		AccurateMad(); // exact RN(q*k) -> 0
		Emit(0x72030008); Drain(); // result -> rows8..11
		volatile uint32_t* dst = Words(0xffbd8200);
		for (unsigned i = 0; i < 32; i++) sum += PioneerAttentionFloat(dst[LaneOffset(i)]);
	}
	return sum * 0.0883883461356163f;
}

static void WeightedValues(const uint16_t* values, const float* probabilities,
	unsigned tokens, float* output)
{
	for (unsigned block = 0; block < 4; block++) {
		Emit(0x702b0000); Drain(); // independent accumulator for each 32 dims
		for (unsigned t = 0; t < tokens; t++) {
			WriteHalf(values + t * 128 + block * 32, 0);
			Emit(0x70030000);
			uint32_t p = PioneerAttentionBits(probabilities[t]);
			Emit(0x71100000 | (p >> 16)); Emit(0x711a0000 | (p & 0xffff));
			AccurateMad(); // result0 = RN(value0 * p1 + accumulator2)
			Emit(0x7c000020); Drain(); // result0 -> accumulator2
		}
		Emit(0x72230008); Drain();
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
	Setup();
	for (unsigned head = 0; head < 2; head++) {
		float scores[256], maximum = -3.4028234663852886e38f;
		for (unsigned t = 0; t < tokens; t++) {
			scores[t] = Dot(query + head * 128, keys + t * 128);
			if (scores[t] > maximum) maximum = scores[t];
		}
		float denominator = 0;
		for (unsigned t = 0; t < tokens; t++) {
			scores[t] = PioneerAttentionExp(scores[t] - maximum); denominator += scores[t];
		}
		float inverse = PioneerAttentionReciprocal(denominator);
		for (unsigned t = 0; t < tokens; t++) scores[t] *= inverse;
		WeightedValues(values, scores, tokens, output + head * 128);
	}
	Drain();
	Fence();
	return tokens;
}
