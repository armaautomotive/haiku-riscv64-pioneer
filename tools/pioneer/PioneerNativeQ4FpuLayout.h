/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_Q4_FPU_LAYOUT_H
#define PIONEER_NATIVE_Q4_FPU_LAYOUT_H
#include <stdint.h>
// Plain fixed DATA geometry,not instructions or authority. Entire staged
// L1 span includes every payload/header/gap/zero buffer/output byte. Source
// metadata preparation recipe and actual hardware owner stay separate.
struct PioneerNativeQ4FpuLayout {
	static constexpr unsigned Raw=0x90000,RawStride=320,Metadata=0x94000,MetadataStride=128;
	static constexpr unsigned Input=0x96000,InputStride=128,Zero=0x97000,Output=0x98000;
	static constexpr unsigned DataBegin=Raw-64,DataEnd=Output+256,DataWords=(DataEnd-DataBegin)/4;
	static constexpr unsigned L1Words=1+2048+2048+DataWords+3,PreparedBytes=6144,InputBytes=512;
	static unsigned Address(unsigned index) {
		if(index>=L1Words)return UINT32_MAX;
		if(!index)return 0x6000;
		index--;if(index<2048)return 0x70000+index*4;
		index-=2048;if(index<2048)return 0x6e000+index*4;
		index-=2048;if(index<DataWords)return DataBegin+index*4;
		return 0x17ffc0+(index-DataWords)*4;
	}
	static bool Contains(unsigned a) {
		return !(a&3) && (a==0x6000 || (a>=0x70000 && a<0x72000) || (a>=0x6e000 && a<0x70000)
			|| (a>=DataBegin && a<DataEnd) || (a>=0x17ffc0 && a<0x17ffcc));
	}
	static bool Immutable(unsigned a) {
		return Contains(a) && (a==0x6000 || (a>=0x70000 && a<0x72000) || (a>=DataBegin && a<Output));
	}
	// Return a byte index in the NEW prepared data or input vector. Headers/
	// stack/status/gaps/zero buffers return UINT32_MAX,never alias a payload.
	static unsigned PreparedIndex(unsigned a) {
		if(a>=Raw-64 && a<Raw-64+16*RawStride) {
			unsigned rel=a-(Raw-64),slot=rel/RawStride,within=rel%RawStride;
			if(within<64)return UINT32_MAX;
			unsigned pair=slot/4,face=slot%4;
			return (face/2)*2048+pair*512+(face%2)*256+within-64;
		}
		if(a>=Metadata-64 && a<Metadata-64+32*MetadataStride) {
			unsigned rel=a-(Metadata-64),slot=rel/MetadataStride,within=rel%MetadataStride;
			return within<64?UINT32_MAX:4096+slot*64+within-64;
		}
		return UINT32_MAX;
	}
	static unsigned InputIndex(unsigned a) {
		if(a<Input-64 || a>=Input-64+8*InputStride)return UINT32_MAX;
		unsigned rel=a-(Input-64),within=rel%InputStride;
		return within<64?UINT32_MAX:(rel/InputStride)*64+within-64;
	}
};
static_assert(PioneerNativeQ4FpuLayout::DataWords==8272 && PioneerNativeQ4FpuLayout::L1Words==12372,"exact49488B heap snapshot");
#endif
