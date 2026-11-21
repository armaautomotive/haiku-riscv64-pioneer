/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_SILU_H
#define PIONEER_QWEN35_DATAFLOW_SILU_H

#include <stdint.h>

namespace PioneerQwen35 {

union DataflowSiluFloatBits {
	float value;
	uint32_t bits;
};

inline bool DataflowSiluFinite(float value)
{
	DataflowSiluFloatBits encoded{};
	encoded.value=value;
	return (encoded.bits&0x7f800000U)!=0x7f800000U;
}

// Range-reduced FP32 exp for non-positive values. The reduced interval is
// [-ln(2)/2, ln(2)/2], where the degree-10 Taylor polynomial is well bounded.
// Scaling by an exact power of two keeps this suitable for the freestanding
// RV32IMF worker without libm or host-side arithmetic.
inline bool DataflowExpNonPositive(float value,float& result)
{
	if (!DataflowSiluFinite(value) || value>0) return false;
	if (value<-104.0f) { result=0; return true; }
	constexpr float OneOverLn2=1.4426950408889634f;
	constexpr float Ln2=0.6931471805599453f;
	const float scaled=value*OneOverLn2;
	const int exponent=int(scaled-0.5f); // RV32 conversion truncates toward zero.
	const float reduced=value-float(exponent)*Ln2;
	float polynomial=1.0f/3628800.0f;
	polynomial=1.0f/362880.0f+reduced*polynomial;
	polynomial=1.0f/40320.0f+reduced*polynomial;
	polynomial=1.0f/5040.0f+reduced*polynomial;
	polynomial=1.0f/720.0f+reduced*polynomial;
	polynomial=1.0f/120.0f+reduced*polynomial;
	polynomial=1.0f/24.0f+reduced*polynomial;
	polynomial=1.0f/6.0f+reduced*polynomial;
	polynomial=0.5f+reduced*polynomial;
	polynomial=1.0f+reduced*polynomial;
	polynomial=1.0f+reduced*polynomial;
	if (!DataflowSiluFinite(polynomial) || !(polynomial>0)) return false;

	DataflowSiluFloatBits scale{};
	if (exponent>=-126) {
		if (exponent>0) return false;
		scale.bits=uint32_t(exponent+127)<<23;
	} else if (exponent>=-149) {
		scale.bits=1U<<unsigned(exponent+149);
	} else {
		result=0;
		return true;
	}
	result=polynomial*scale.value;
	return DataflowSiluFinite(result);
}

inline bool DataflowSiluProduct(float gate,float up,float& result)
{
	if (!DataflowSiluFinite(gate) || !DataflowSiluFinite(up)) return false;
	float sigmoid=0;
	if (gate>=0) {
		float exponential=0;
		if (!DataflowExpNonPositive(-gate,exponential)) return false;
		sigmoid=1.0f/(1.0f+exponential);
	} else {
		float exponential=0;
		if (!DataflowExpNonPositive(gate,exponential)) return false;
		sigmoid=exponential/(1.0f+exponential);
	}
	result=(gate*sigmoid)*up;
	return DataflowSiluFinite(result);
}

} // namespace PioneerQwen35

#endif
