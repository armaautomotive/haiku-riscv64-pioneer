/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_MIXED_FPU_STREAM_VARIANT_H
#define PIONEER_NATIVE_MIXED_FPU_STREAM_VARIANT_H
#include <stdint.h>
// NEW87 DATA identity only. Seventeen fixed original Q6/Q8 bands,NOT a
// caller format/row-count/code/address/route/reset or Work capability.
struct PioneerNativeMixedFpuStreamVariant {
	static constexpr unsigned Role=87,Opcode=0x8300,Samples=17;
	static constexpr uint64_t Tag=UINT64_C(0x2026110400010087);
	static constexpr unsigned Type(unsigned s){return s<6?14:s<17?8:0;}
	static constexpr unsigned Width(unsigned s){return s>=17?0:(s<3 || (s>=9 && s<12))?6144:5120;}
	static constexpr unsigned RealRows(unsigned s){return s>=17?0:s==16?16:32;}
	static constexpr unsigned Tensor(unsigned s){return s<3?48:s<6?0:s<9?5:s<12?16:s<15?49:s<17?11:UINT32_MAX;}
	static constexpr unsigned Outputs(unsigned s){return s>=17?0:s<3?5120:s<6?248320:s<9?10240:s<12?5120:s<15?12288:48;}
	static constexpr unsigned First(unsigned s) {
		if(s>=17)return UINT32_MAX;
		if(s>=15)return (s-15)*32;
		const unsigned group=s/3,position=s%3;
		return !position?0:(group==0 || group==3)?(position==1?2560:5088):group==1?(position==1?124160:248288)
			:group==2?(position==1?5120:10208):(position==1?6144:12256);
	}
	static constexpr unsigned Marker(unsigned s){return Type(s)==14?0x31533651:Type(s)==8?0x31533851:0;}
	static constexpr unsigned Fault(unsigned s){return Type(s)==14?0x45533651:Type(s)==8?0x45533851:0;}
	static constexpr unsigned Count(unsigned s){return s>=17?0:(Type(s)==14?0x06000000:0x08000000)|Width(s);}
	static constexpr unsigned CodeBytes(unsigned s){return Type(s)==14?7848:Type(s)==8?6812:0;}
	static constexpr unsigned PreparedBytes(unsigned s){return Type(s)==14?8192:Type(s)==8?9216:0;}
	static constexpr unsigned Packets(unsigned s){return Type(s)==14?64:Type(s)==8?56:0;}
	static constexpr unsigned Flits(unsigned s){return Type(s)==14?136:Type(s)==8?152:0;}
};
#endif
