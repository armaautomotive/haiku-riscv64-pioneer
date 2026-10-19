/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DECODED_PROBE_H
#define PIONEER_Q38_DECODED_PROBE_H
// Isolated59 DIAGNOSTIC ONLY. Independent scalar Q8 byte -> expected
// opaque Dst32 -> existing converted signed32 RISC view. No dot product,
// approximation,route authority,host result or production fallback.
// ISA8a34e25e Dst.md/SFPSTORE.md/MOVD2A.md;Config3 remains0x10000.
static inline uint32_t PioneerQ38DecodedQ8Risc(uint8_t code,bool sourceRoundTrip=false)
{
	int value=int(code)-int((code&128)*2);
	unsigned magnitude=unsigned(value<0?-value:value);
	// Algebraic form of the opaque high16 store + Dst permutation. The
	// exhaustive test independently derives it from scalar operand bits.
	uint32_t risc=0x08000000|((magnitude&7)<<28)|((magnitude>>3)<<16);
	// Blackhole MOVA2D with FP32 accumulation reinterprets SrcA as TF32.
	// SrcA's exact FP16 integer overlay has exponent16 and magnitude10b;
	// thus its converted TF32 view is sign|16<<23|magnitude<<13.
	if (sourceRoundTrip) risc=0x08000000|(magnitude<<13);
	return value<0?0U-risc:risc;
}
#endif
