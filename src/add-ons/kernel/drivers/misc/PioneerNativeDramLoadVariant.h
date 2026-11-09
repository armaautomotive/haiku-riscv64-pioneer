/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_QGW8_HEAD_BAND_130) && (PIONEER_QGW8_HEAD_BAND_130 != 1 \
 || !defined(PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64) || PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64 != 1 \
 || defined(PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64) || defined(PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGW8 head-band role requires isolated C64 arithmetic,never Work or CPU reference
#endif
#ifndef PIONEER_NATIVE_DRAM_LOAD_VARIANT_H
#define PIONEER_NATIVE_DRAM_LOAD_VARIANT_H
// Compile-time placement DATA. The83 transfer passed one fixed hardware
// sample;84 arithmetic remains separate/unqualified. No runtime fallback.
#if defined(PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64)
#if PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64 != 1 || defined(PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64) || defined(PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error C64 arithmetic requires its separate diagnostic84 role,never LOAD-ONLY/Work/reference
#endif
#if defined(_KERNEL_MODE) && (!defined(PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC) || PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || (defined(PIONEER_QGW8_HEAD_BAND_130) ? PIONEER_DRIVER_BUILD_TAG != 0x2026110800010130ULL : PIONEER_DRIVER_BUILD_TAG != 0x2026110400010084ULL))
#error C64 kernel arithmetic placement requires exact diagnostic84 reload role
#endif
#endif
#if defined(PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64)
#if PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64 != 1 || defined(PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error C64 placement requires its separate LOAD-ONLY83 role,never math/Work/reference
#endif
#if defined(_KERNEL_MODE) && (!defined(PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC) || PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026110400010083ULL)
#error C64 kernel placement requires exact LOAD-ONLY83 reload role
#endif
#endif
struct PioneerNativeDramLoadVariant {
#if defined(PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64)
#if defined(PIONEER_QGW8_HEAD_BAND_130)
	static constexpr unsigned Role=130,Header=64,WeightStride=1152,InputStride=128,Input=0x12c000,Output=0x140000;
	static constexpr unsigned Marker=0x3436424e,Count=0x04001400,LoadOnly=0,Opcode=0x9500;
	static constexpr unsigned long long Tag=0x2026110800010130ULL;
#else
	static constexpr unsigned Role=84,Header=64,WeightStride=1152,InputStride=128,Input=0x12c000,Output=0x140000;
	static constexpr unsigned Marker=0x3436424e,Count=0x04001400,LoadOnly=0,Opcode=0x8000; // NB64,NOT ND64
	static constexpr unsigned long long Tag=0x2026110400010084ULL;
#endif
#elif defined(PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64)
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
#if defined(PIONEER_QGW8_HEAD_BAND_130)
static_assert(PioneerNativeDramLoadVariant::Role==130,"late head-band selector change");
#elif defined(PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64)
static_assert(PioneerNativeDramLoadVariant::Role==84,"late C64 arithmetic selector change");
#elif defined(PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64)
static_assert(PioneerNativeDramLoadVariant::Role==83,"late C64 load selector change");
#else
static_assert(PioneerNativeDramLoadVariant::Role==82,"late legacy load selector change");
#endif
