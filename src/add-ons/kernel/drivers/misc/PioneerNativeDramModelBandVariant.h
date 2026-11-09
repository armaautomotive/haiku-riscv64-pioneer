/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_DRAM_MODEL_BAND_VARIANT_H
#define PIONEER_NATIVE_DRAM_MODEL_BAND_VARIANT_H
#include "PioneerNativeDramLoadVariant.h"
// Distinct compile-time math identity. Geometry alone never grants math IO.
struct PioneerNativeDramModelBandVariant {
#if defined(PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64)
#if defined(PIONEER_QGW8_HEAD_BAND_130)
	static constexpr unsigned Role=130,Marker=0x3436424e,Fault=0x4546424e,Count=0x04001400,Opcode=0x9500;
	static constexpr unsigned long long Tag=0x2026110800010130ULL;
	static_assert(PioneerNativeDramLoadVariant::Role==130,"exact isolated head-band placement policy");
#else
	static constexpr unsigned Role=84,Marker=0x3436424e,Fault=0x4546424e,Count=0x04001400,Opcode=0x8000;
	static constexpr unsigned long long Tag=0x2026110400010084ULL;
	static_assert(PioneerNativeDramLoadVariant::Role==84,"exact C64 math placement policy");
#endif
	static_assert(PioneerNativeDramLoadVariant::Tag==Tag && PioneerNativeDramLoadVariant::Marker==Marker
		&& !PioneerNativeDramLoadVariant::LoadOnly,"exact math identity,not a loader role");
#else
	static constexpr unsigned Role=81,Marker=0x3142444e,Fault=0x4542444e,Count=0x04001400,Opcode=0x7d00;
	static constexpr unsigned long long Tag=0x2026110400010081ULL;
	static_assert(PioneerNativeDramLoadVariant::Role==82,"legacy math requires exact original C32 placement,not LOAD-ONLY83");
#endif
};
#endif
#if defined(PIONEER_QGW8_HEAD_BAND_130)
static_assert(PioneerNativeDramModelBandVariant::Role==130,"late head-band selector change");
#elif defined(PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64)
static_assert(PioneerNativeDramModelBandVariant::Role==84,"late C64 arithmetic selector change");
#else
static_assert(PioneerNativeDramModelBandVariant::Role==81,"late legacy math selector change");
#endif
