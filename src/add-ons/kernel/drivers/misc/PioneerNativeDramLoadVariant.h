/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_DRAM_LOAD_VARIANT_H
#define PIONEER_NATIVE_DRAM_LOAD_VARIANT_H
// Compile-time placement control ONLY. Never a runtime fallback or proof
// that C64 fixes the observed C32 transfer failure. Packed GDDR is unchanged.
#if defined(PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64)
#if PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64 != 1 || defined(PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error C64 placement requires its separate LOAD-ONLY83 role,never math/Work/reference
#endif
#if defined(_KERNEL_MODE) && (!defined(PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC) || PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026110400010083ULL)
#error C64 kernel placement requires exact LOAD-ONLY83 reload role
#endif
#endif
struct PioneerNativeDramLoadVariant {
#if defined(PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64)
	static constexpr unsigned Role=83,Header=64,WeightStride=1152,InputStride=128,Input=0x12c000,Output=0x140000;
	static constexpr unsigned Marker=0x3436444e,Count=0xc0001400,LoadOnly=64,Opcode=0x7f00; // ND64
	static constexpr unsigned long long Tag=0x2026110400010083ULL;
#else
	static constexpr unsigned Role=82,Header=32,WeightStride=1120,InputStride=96,Input=0x126000,Output=0x134000;
	static constexpr unsigned Marker=0x314c444e,Count=0x80001400,LoadOnly=1,Opcode=0x7e00; // NDL1
	static constexpr unsigned long long Tag=0x2026110400010082ULL;
#endif
	static constexpr unsigned L1Words=1+2048+2048+8+160*(WeightStride+InputStride)/4+64+3;
	static constexpr unsigned HeaderWords=320*(Header/4);
};
#endif
