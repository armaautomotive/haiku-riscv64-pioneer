/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_DRAM_BAND_LOADER_H
#define PIONEER_NATIVE_DRAM_BAND_LOADER_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Unqualified card-side DRAM band loader is not driver/Work/validation
#endif
#include "PioneerNativeDramBandLayout.h"
#include "PioneerDramCohortReadSweep.h"
// Single-shot OFFBOARD image leaf:READ packed weights/BF16 vector from one
// firmware-bound bank into disjoint C32-aligned private L1 tiles,16 requests
// per batch. No scalar reads/copies of weights/activations,host/PCIe callback,
// GDDR writes,implicit fallback,reset,retry or cleanup. Source addresses are
// fixed diagnostic windows,NOT an arbitrary-address production interface.
// Caller owes SAME exclusive native PCI/NIU/L1/GDDR/source/clock/lifecycle
// lease and source payload integrity BEFORE entry. Shape/Self/counters are
// necessary checks,NOT that authority. No existing driver selects this leaf.
class PioneerNativeDramBandLoader {
	using L=PioneerNativeDramBandLayout;
	using Q=PioneerDramQueuedReadSweep;
	bool attempted=false;
	uint32_t baseline[9]={};unsigned issued=0,flits=0;
	static unsigned Counter(unsigned i){const unsigned indices[9]={1,2,3,4,5,8,10,12,14};return 0xffb20200+indices[i]*4;}
	unsigned Expected(unsigned i)const{return i==2?flits:(i==1 || i==3 || i==4 || i==8)?issued:0;}
	template<class IO> static bool Quiet(IO& io) {
		uint32_t available=0;
		return PioneerDramCohortReadSweep::Self(io,0) && Q::Fifo(io,available)
			&& available==0x10101010 && PioneerDramReadSweep::Idle(io);
	}
	template<class IO> bool Drain(IO& io) {
		// Counters do NOT count queued commands:observe empty FIFO FIRST.
		bool empty=false;
		for(unsigned poll=0;poll<Q::PollLimit;poll++) {
			uint32_t available=0;if(!Q::Fifo(io,available))return false;
			if(available==0x10101010){empty=true;break;}
		}
		if(!empty)return false;
		for(unsigned poll=0;poll<Q::PollLimit;poll++) {
			const unsigned busy=io.Read(0xffb20040);if(busy>1)return false;
			bool complete=!busy;
			for(unsigned i=0;i<9;i++) {
				const uint32_t delta=uint32_t(io.Read(Counter(i))-baseline[i]);
				if(delta>Expected(i))return false;
				complete&=delta==Expected(i);
			}
			for(unsigned i=0;i<32;i++) {
				const unsigned count=io.Read(0xffb20240+i*4)&255;
				if(count>(i==0?16U:0U))return false;
				complete&=!count;
			}
			if(complete){io.Fence();return Quiet(io);}
		}
		return false; // no subsequent packet,header write,math or cleanup
	}
public:
	PioneerNativeDramBandLoader()=default;
	PioneerNativeDramBandLoader(const PioneerNativeDramBandLoader&)=delete;
	PioneerNativeDramBandLoader& operator=(const PioneerNativeDramBandLoader&)=delete;
	static bool Route(unsigned route) {
		const unsigned x=route&63,y=route>>6;
		return (x==17 || x==18) && (y==12 || y==15 || y==18 || y==21);
	}
	template<class IO> bool Run(IO& io,unsigned k,unsigned firmwareBoundRoute) {
		if(attempted)return false;
		attempted=true;
		if(!L::Width(k) || !Route(firmwareBoundRoute))return false; // ZERO IO
		if(!Quiet(io))return false;
		for(unsigned i=0;i<9;i++)baseline[i]=io.Read(Counter(i));
		for(unsigned operand=0;operand<2;operand++) {
			// Complete owned headers retain the immediately preceding16B
			// unpacker header. Explicit83 uses64B,default82/81 uses32B.
			// No packed-GDDR/cache padding or source conversion change.
			for(unsigned tile=0;tile<L::Tiles(k);tile++) {
				L::Read packet={};if(!L::Packet(k,operand,tile,packet))return false;
				for(unsigned word=0;word<L::Header/4;word++) {
					const unsigned address=packet.destination-L::Header+word*4;
					io.Write(address,0);if(io.Read(address))return false;
				}
			}
			io.Fence();
			L::Read first={};if(!L::Packet(k,operand,0,first) || !Quiet(io))return false;
			const uint32_t command[10]={first.source,0,firmwareBoundRoute,first.destination,0,129,0,0,first.bytes,0};
			for(unsigned i=0;i<10;i++)io.Write(0xffb20000+i*4,command[i]);
			for(unsigned i=0;i<10;i++)if(io.Read(0xffb20000+i*4)!=command[i])return false;
			for(unsigned batch=0;batch<L::Tiles(k);batch+=16) {
				for(unsigned slot=0;slot<16;slot++) {
					L::Read packet={};if(!L::Packet(k,operand,batch+slot,packet) || !Q::Slot(io))return false;
					io.Write(0xffb20000,packet.source);io.Write(0xffb2000c,packet.destination);
					if(io.Read(0xffb20000)!=packet.source || io.Read(0xffb2000c)!=packet.destination)return false;
					io.Fence();io.Write(0xffb20040,1);if(io.Read(0xffb20040)>1)return false;
					io.Fence();issued++;flits+=packet.bytes/64;
				}
				if(!Drain(io))return false;
			}
		}
		return issued==2*L::Tiles(k) && flits==18*L::Tiles(k) && Quiet(io);
	}
};
#endif
