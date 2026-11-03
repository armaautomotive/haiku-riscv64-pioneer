/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_FIDELITY_VIEW_H
#define PIONEER_Q38_NATIVE_FIDELITY_VIEW_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Unqualified native fidelity VIEW is CPU diagnostic data only
#endif
#include <cstdint>
#include <cstring>
#include <limits>
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,"diagnostic requires IEEE binary32");
// Operand-view diagnostic ONLY, not a Blackhole dot/accumulator simulator.
// MVMUL phase0 consumes5 significant SrcA bits and7 significant SrcB bits.
// This views that truncation for finite normal/zero FP32 inputs; it does NOT
// model unpacking, intermediate alignment/rounding,hardware NaN or denormals.
// No converter/cache recipe,Work comparator or running model selects it.
static inline bool PioneerQ38NativePhase0View(float input,unsigned significant,float& output) {
	if(significant!=5 && significant!=7)return false;
	uint32_t word;std::memcpy(&word,&input,4);
	const unsigned exponent=(word>>23)&255;
	if(exponent==255 || (!exponent && (word&0x7fffff)))return false;
	word&=~((uint32_t(1)<<(24-significant))-1);std::memcpy(&output,&word,4);return true;
}
#endif
