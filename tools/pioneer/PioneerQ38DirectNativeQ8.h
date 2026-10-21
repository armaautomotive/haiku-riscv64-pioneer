/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DIRECT_NATIVE_Q8_H
#define PIONEER_Q38_DIRECT_NATIVE_Q8_H
// OFFLINE ONLY. NEW SM7 Q8 format,NOT original two's-complement Q8_0.
// ALL values[-127,127] are encoded once by authenticated host loading.
// Keep native SrcA format for the whole Work-Q8 job. Metadata helpers
// must restore this SAME format before its next source-bank publication.
template<class E> static
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#else
__attribute__((noinline))
#endif
void PioneerQ38DirectNativeQ8Setup(E& e) {
	e.Config(0,0x423e);e.Config(1,0xa03c0000);e.Config(73,3);
	e.Op(0x8a000002);e.Drain(); // ALL lanes enabled before ANY first affine clear/load
}
template<class E> static bool PioneerQ38DirectNativeQ8(E& e,unsigned panel) {
	if(panel>=2)return false;
	e.Drain();e.Config(76,(0x90020+panel*256-16)/16);
	e.Op(0x3700000f);e.Op(0x5e23fc00);e.Op(0x5120000a);e.Op(0x5420000f);
	e.Op(0x420000c1);e.Drain();return true;
}
#endif
