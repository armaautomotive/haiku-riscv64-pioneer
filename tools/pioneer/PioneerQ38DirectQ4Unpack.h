/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DIRECT_Q4_UNPACK_H
#define PIONEER_Q38_DIRECT_Q4_UNPACK_H
// OFFLINE ONLY. A NEW predecoded4352B tile is already fetched/owned at
// 90000..91100. The256B metadata prefix is followed by16 signed-INT8
// panels;every encoded byte is nonnegative0..15,so native SM7 unpack is
// LOSSLESS (unlike Q8's -128). Native format14,ordinary Thread5=4 and
// source-bank publication. No SFPU decode/MOVD2A/operand staging.
// Existing matrix phases/final drain MUST complete before Restore below.
template<class E> static
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#else
__attribute__((noinline))
#endif
bool PioneerQ38DirectQ4Unpack(E& e,unsigned panel) {
	if(panel>=16)return false;
	e.Drain();
#ifndef PIONEER_Q38_PREDECODED_Q4_STICKY
	e.Config(0,0x423e);e.Config(1,0xa03c0000);
	e.Config(73,3);
#endif
	e.Config(76,(0x90100+panel*256-16)/16);
	// Configure ONLY while fully drained;RWC/ADC setup then precedes its
	// owned source consumer without any intervening CPU config access.
	e.Op(0x3700000f);e.Op(0x5e23fc00);e.Op(0x5120000a);e.Op(0x5420000f);
	e.Op(0x420000c1);e.Drain();return true;
}
#ifdef PIONEER_Q38_PREDECODED_Q4_STICKY
// OFFLINE ONLY. Called ONCE after the block's exact FP32 metadata staging,
// not each of its sixteen panels. Same format/ownership and original
// affine rounding. Restore MUST execute before the NEXT metadata block.
template<class E> static
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#else
__attribute__((noinline))
#endif
void PioneerQ38DirectQ4Setup(E& e) {
	e.Drain();e.Config(0,0x423e);e.Config(1,0xa03c0000);e.Config(73,3);
	e.Op(0x8a000002);e.Drain();
}
#endif
template<class E> static
#ifdef PIONEER_Q38_PREDECODED_Q4_STICKY
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#else
__attribute__((noinline))
#endif
#endif
void PioneerQ38DirectQ4Restore(E& e) {
	// Matrix's real completion wait has ALREADY released both banks.
	// Restore the original metadata/SFPU contract before its first load.
	e.Config(0,0x4231);e.Config(1,0xa03c8000);e.Config(73,0x13);e.Drain();
}
#endif
