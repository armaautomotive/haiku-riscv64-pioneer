/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_STREAM_LAYOUT_H
#define PIONEER_Q38_Q4_FPU_STREAM_LAYOUT_H
// Plain DATA proposal only. No owner/route/GDDR allocation/range authority.
// Packed source:one6144B prepared Q4 block,one512B BF16 vector per K256.
// L1 headers are PRIVATE zero64B,not duplicated in GDDR/cache weights.
struct PioneerQ38Q4FpuStreamLayout {
	static constexpr unsigned Raw=0x90000,Metadata=0x94000,Input=0x96000,BufferStride=0x10000;
	static constexpr unsigned RawStride=320,MetadataStride=128,InputStride=128,Header=64;
	static constexpr unsigned Zero=0x97000,Output=0x98000,Frame=0x74000;
	static constexpr unsigned DramWeight=0x04000000,DramInput=0x04100000,Packets=56,Flits=104,Slice=14;
	struct Read {unsigned source,destination,bytes;};
	static constexpr bool Width(unsigned k){return k==5120 || k==17408;}
	static bool Route(unsigned xy){unsigned x=xy&63,y=xy>>6;return (x==17 || x==18) && (y==12 || y==15 || y==18 || y==21);}
	static bool Packet(unsigned k,unsigned block,unsigned buffer,unsigned packet,Read& result) {
		if(!Width(k) || block>=k/256 || buffer>=2 || packet>=Packets)return false;
		Read p={};unsigned base=buffer*BufferStride;
		if(packet<16) {
			unsigned pair=packet/4,face=packet%4;
			p={DramWeight+block*6144+(face/2)*2048+pair*512+(face%2)*256,base+Raw+packet*RawStride,256};
		} else if(packet<48)p={DramWeight+block*6144+4096+(packet-16)*64,base+Metadata+(packet-16)*MetadataStride,64};
		else p={DramInput+block*512+(packet-48)*64,base+Input+(packet-48)*InputStride,64};
		result=p;return true;
	}
	static unsigned HeaderAddress(unsigned buffer,unsigned packet) {
		Read p={};return Packet(5120,0,buffer,packet,p)?p.destination-Header:~0U;
	}
	static bool HeaderWord(unsigned a) {
		if(a&3)return false;
		for(unsigned b=0;b<2;b++)for(unsigned p=0;p<Packets;p++) {
			unsigned h=HeaderAddress(b,p);if(a>=h && a<h+Header)return true;
		}
		return false;
	}
};
#endif
