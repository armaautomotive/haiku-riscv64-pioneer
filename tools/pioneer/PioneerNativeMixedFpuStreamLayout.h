/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_MIXED_FPU_STREAM_LAYOUT_H
#define PIONEER_NATIVE_MIXED_FPU_STREAM_LAYOUT_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeMixedFpuStreamVariant.h"
#include "PioneerQ38Q6FpuStreamLayout.h"
#include "PioneerQ38Q8FpuStreamLayout.h"
// NEW87 DATA ranges only. Complete BOTH buffers/headers/gaps/shared zero and
// outputs are journaled. GDDR grants ONLY the exact selected two ranges.
struct PioneerNativeMixedFpuStreamLayout {
	using V=PioneerNativeMixedFpuStreamVariant;
	static constexpr unsigned Raw=0x90000,High=0x92000,Metadata=0x94000,Input=0x96000,Zero=0x97000,Output=0x98000,Frame=0x74000;
	static constexpr unsigned BufferStride=0x10000,DataBegin=Raw-64,DataEnd=0xa6400,DataWords=(DataEnd-DataBegin)/4;
	static constexpr unsigned L1Words=1+2048+2048+8+DataWords+3,MaxDramWords=58368;
	static constexpr unsigned DramWeight=0x04000000,DramInput=0x04100000;
	struct Read {unsigned source,destination,bytes;};
	static constexpr unsigned WeightBytes(unsigned s){return V::PreparedBytes(s)*(V::Width(s)/256);}
	static constexpr unsigned InputBytes(unsigned s){return V::Width(s)*2;}
	static constexpr unsigned DramWords(unsigned s){return (WeightBytes(s)+InputBytes(s))/4;}
	static unsigned Address(unsigned i) {
		if(i>=L1Words)return UINT32_MAX;
		if(!i)return 0x6000;
		i--;
		if(i<2048)return 0x70000+i*4;
		i-=2048;
		if(i<2048)return 0x6e000+i*4;
		i-=2048;
		if(i<8)return Frame+i*4;
		i-=8;
		if(i<DataWords)return DataBegin+i*4;
		return 0x17ffc0+(i-DataWords)*4;
	}
	static bool Contains(unsigned a) {
		return !(a&3) && (a==0x6000 || (a>=0x6e000 && a<0x72000) || (a>=Frame && a<Frame+32)
			|| (a>=DataBegin && a<DataEnd) || (a>=0x17ffc0 && a<0x17ffcc));
	}
	static unsigned DramAddress(unsigned s,unsigned i) {
		if(s>=V::Samples || i>=DramWords(s))return UINT32_MAX;
		return i<WeightBytes(s)/4?DramWeight+i*4:DramInput+(i-WeightBytes(s)/4)*4;
	}
	static bool DramContains(unsigned s,unsigned a) {
		return s<V::Samples && !(a&3) && ((a>=DramWeight && a-DramWeight<WeightBytes(s))
			|| (a>=DramInput && a-DramInput<InputBytes(s)));
	}
	static bool Packet(unsigned s,unsigned block,unsigned buffer,unsigned packet,Read& output) {
		if(s>=V::Samples)return false;
		if(V::Type(s)==14){PioneerQ38Q6FpuStreamLayout::Read p={};if(!PioneerQ38Q6FpuStreamLayout::Packet(V::Width(s),block,buffer,packet,p))return false;output={p.source,p.destination,p.bytes};}
		else {PioneerQ38Q8FpuStreamLayout::Read p={};if(!PioneerQ38Q8FpuStreamLayout::Packet(V::Width(s),block,buffer,packet,p))return false;output={p.source,p.destination,p.bytes};}
		return true;
	}
	// Independent inverse mapping,NOT implemented by calling Packet. Headers,
	// gaps,zero/output/stack/status and unaligned bytes are NEVER payloads.
	static unsigned PreparedIndex(unsigned s,unsigned a) {
		if(s>=V::Samples || (a&3))return UINT32_MAX;
		const bool q6=V::Type(s)==14;const unsigned slots=q6?16:32;
		if(a>=Raw-64 && a<Raw-64+slots*320) {
			unsigned rel=a-(Raw-64),slot=rel/320,within=rel%320;if(within<64)return UINT32_MAX;
			unsigned group=slot/4,face=slot%4;
			return (face/2)*(q6?2048:4096)+group*512+(face%2)*256+within-64;
		}
		if(q6 && a>=High-64 && a<High-64+8*320) {
			unsigned rel=a-(High-64),slot=rel/320,within=rel%320;if(within<64)return UINT32_MAX;
			unsigned half=slot/4,face=slot%4;return 4096+(face/2)*1024+half*512+(face%2)*256+within-64;
		}
		const unsigned metas=q6?32:16;
		if(a>=Metadata-64 && a<Metadata-64+metas*128) {
			unsigned rel=a-(Metadata-64),slot=rel/128,within=rel%128;
			return within<64?UINT32_MAX:(q6?6144:8192)+slot*64+within-64;
		}
		return UINT32_MAX;
	}
	static unsigned InputIndex(unsigned a) {
		if((a&3) || a<Input-64 || a>=Input-64+8*128)return UINT32_MAX;
		unsigned rel=a-(Input-64),within=rel%128;return within<64?UINT32_MAX:(rel/128)*64+within-64;
	}
	static unsigned FinalPayloadIndex(unsigned s,unsigned a) {
		if(s>=V::Samples || (a&3))return UINT32_MAX;
		for(unsigned b=0;b<2;b++) {
			if(a<b*BufferStride)continue;
			unsigned local=a-b*BufferStride,block=V::Width(s)/256-2+b;
			unsigned at=PreparedIndex(s,local);if(at!=UINT32_MAX)return block*(V::PreparedBytes(s)/4)+at/4;
			at=InputIndex(local);if(at!=UINT32_MAX)return WeightBytes(s)/4+block*128+at/4;
		}
		return UINT32_MAX;
	}
	static bool Immutable(unsigned a) {
		return Contains(a) && (a==0x6000 || (a>=0x70000 && a<0x72000) || (a>=Frame && a<Frame+32)
			|| (a>=DataBegin && a<DataEnd && !(a>=Output && a<Output+256)));
	}
};
static_assert(PioneerNativeMixedFpuStreamLayout::L1Words*4==107632 && PioneerNativeMixedFpuStreamLayout::MaxDramWords*4==233472,"bounded independent mixed heap journals");
#endif
