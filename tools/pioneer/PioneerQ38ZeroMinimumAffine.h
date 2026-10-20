/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_ZERO_MINIMUM_AFFINE_H
#define PIONEER_Q38_ZERO_MINIMUM_AFFINE_H
#include "PioneerQ38VectorAffine.h"
// OFFLINE arithmetic prerequisite ONLY;NO image/driver selects it.
// Q8/Q6 metadata minimum is EXACT +0. With the original finite,normal-
// or-zero domain,its correction can change ONLY the sign of a zero product.
// An accumulator initialized to +0 stays normal or +0 under round-to-nearest
// additions,so that sign cannot affect the final addition. Therefore retain
// the ORIGINAL coefficient*scale,then dot,then accumulator rounding points.
// PRECONDITION:old accumulator is NEVER -0,minimum is +0,canonical SM32 dot,
// finite bounded metadata/scale as in VectorAffine. A generic affine caller
// cannot assume this: -0 accumulator is an explicit counterexample in tests.
// Future integration owes the whole accumulator initialization/induction,
// physical lanes/source/drains/metadata/ranges. This helper grants NO IO.
template<class E> static void PioneerQ38ZeroMinimumAffine(E& e)
{
	e.Op(0x90000000); // canonical SM32 dot -> exact FP32 integer
	PioneerQ38VectorAffineMul(e,1,3,6);
	PioneerQ38VectorAffineMul(e,6,0,0);
	PioneerQ38VectorAffineAdd(e,5,0,0);
}
#endif
