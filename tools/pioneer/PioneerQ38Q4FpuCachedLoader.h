/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_CACHED_LOADER_H
#define PIONEER_Q38_Q4_FPU_CACHED_LOADER_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Q4 streaming prototype is not a driver/Work/reference capability
#endif
#include "PioneerQ38Q4FpuCachedLayout.h"
#include "PioneerDramCohortReadSweep.h"
// Returning cached-weight leaf,NOT native authority. The caller supplies
// BF16 producer provenance;weight tiles remain existing16-row byte-major.
// Route/self/job copied ONCE;only this private validated job reaches fast
// packet geometry. No reinterpretation of104 or old integer activations.
// OFFBOARD leaf,NOT native authority. Exclusive NIU/FIFO/L1/GDDR/clock/source
// lease required externally. Two disjoint buffers,one pending prefetch,one
// reader. Two13-request slices can overlap two groups of current math.
// No scalar payload read/copy,DRAM write,host callback,retry/reset/cleanup.
class PioneerQ38Q4FpuCachedLoader {
	using L=PioneerQ38Q4FpuCachedLayout;using Q=PioneerDramQueuedReadSweep;
	enum {Free,Loading,Ready,Reading};
	L::Job job={};
	unsigned pair=0,width=0,next=0,pending=2,reader=2,packet=0,issued=0,flits=0;
	unsigned state[2]={Free,Free},block[2]={~0U,~0U};
	// Word flags keep scalar stores inside the already qualified RV32
	// word-only ABI;do not broaden the strict decoder to accept byte stores.
	uint32_t counters[9]={},command[10]={};unsigned attempted=0,failed=0,completed=0;
	bool Fail(){failed=true;return false;} // MEMORY ONLY;caller parks/retains
	static unsigned Counter(unsigned i){static const unsigned indices[]={1,2,3,4,5,8,10,12,14};return 0xffb20200+indices[i]*4;}
	unsigned Expected(unsigned i)const{return i==2?flits:(i==1 || i==3 || i==4 || i==8)?issued:0;}
	template<class IO> bool Quiet(IO& io) {
		uint32_t available=0;return L::Self(io,job.words[6]) && Q::Fifo(io,available)
			&& available==0x10101010 && PioneerDramReadSweep::Idle(io);
	}
	template<class IO> bool Commands(IO& io)const {
		for(unsigned i=0;i<10;i++)if(io.Read(0xffb20000+i*4)!=command[i])return false;
		return true;
	}
	template<class IO> bool Drain(IO& io) {
		unsigned empty=0;
		for(unsigned poll=0;poll<Q::PollLimit;poll++) {
			uint32_t available=0;if(!Q::Fifo(io,available))return false;
			if(available==0x10101010){empty=true;break;}
		}
		if(!empty)return false; // queued requests are NOT outstanding counters
		for(unsigned poll=0;poll<Q::PollLimit;poll++) {
			unsigned busy=io.Read(0xffb20040);if(busy>1)return false;unsigned complete=!busy;
			for(unsigned i=0;i<9;i++) {
				uint32_t delta=uint32_t(io.Read(Counter(i))-counters[i]);
				if(delta>Expected(i))return false;
				complete&=delta==Expected(i);
			}
			// Only TID0 is issued. Poll its outstanding credit;the terminal
			// Quiet proof STILL checks ALL32 TIDs and every other queue BEFORE
			// the visibility fence/consumer handoff. Do not spend31 redundant
			// register reads on every incomplete poll or widen the poll budget.
			unsigned count=io.Read(0xffb20240)&255;if(count>16)return false;complete&=!count;
			if(complete){if(!Quiet(io) || !Commands(io))return false;io.Fence();return true;}
		}
		return false;
	}
public:
	PioneerQ38Q4FpuCachedLoader()=default;
	PioneerQ38Q4FpuCachedLoader(const PioneerQ38Q4FpuCachedLoader&)=delete;
	PioneerQ38Q4FpuCachedLoader& operator=(const PioneerQ38Q4FpuCachedLoader&)=delete;
	template<class IO> bool Begin(IO& io,const L::Job& selected,unsigned selectedPair) {
		if(attempted || failed)return false;
		attempted=true;
		if(!L::Shape(selected) || selectedPair>=selected.words[4]/2)return Fail(); // ZERO IO
  volatile uint32_t* copied=job.words;for(unsigned i=0;i<16;i++)copied[i]=selected.words[i];
  pair=selectedPair;width=job.words[2];
		if(!Quiet(io))return Fail();
		for(unsigned i=0;i<9;i++)counters[i]=io.Read(Counter(i));
		for(unsigned buffer=0;buffer<2;buffer++)for(unsigned p=0;p<L::Packets;p++)for(unsigned byte=0;byte<L::Header;byte+=4) {
			unsigned a=L::HeaderAddress(buffer,p)+byte;io.Write(a,0);if(io.Read(a))return Fail();
		}
		io.Fence();
		L::Read first={};if(!L::SelectedPacket(job,pair,0,0,0,first))return Fail();
		const uint32_t initial[10]={first.source,0,first.route,first.destination,0,job.words[7],0,0,first.bytes,0};
		for(unsigned i=0;i<10;i++){command[i]=initial[i];io.Write(0xffb20000+i*4,command[i]);}
		return (Commands(io) && Quiet(io)) || Fail();
	}
	template<class IO> bool Open(IO& io,unsigned kblock,unsigned buffer) {
		if(failed || completed || !attempted)return false;
		if(pending!=2 || buffer>=2 || state[buffer]!=Free || kblock!=next || kblock>=width/256
			|| (reader<2 && (buffer==reader || kblock!=block[reader]+1)))return Fail();
		if(!Quiet(io) || !Commands(io))return Fail();
		pending=buffer;state[buffer]=Loading;block[buffer]=kblock;next++;packet=0;return true;
	}
	template<class IO> bool Slice(IO& io) {
		if(failed || completed || !attempted)return false;
		if(pending>=2 || state[pending]!=Loading || packet>=L::Packets)return Fail();
		if(!L::Self(io,job.words[6]) || !Commands(io))return Fail();
		for(unsigned n=0;n<L::Slice;n++) {
			L::Read p={};if(!L::SelectedPacket(job,pair,block[pending],pending,packet,p) || !Q::Slot(io))return Fail();
			command[0]=p.source;command[2]=p.route;command[3]=p.destination;command[8]=p.bytes;
			io.Write(0xffb20000,p.source);io.Write(0xffb20008,p.route);io.Write(0xffb2000c,p.destination);io.Write(0xffb20020,p.bytes);
			if(io.Read(0xffb20000)!=p.source || io.Read(0xffb20008)!=p.route || io.Read(0xffb2000c)!=p.destination || io.Read(0xffb20020)!=p.bytes)return Fail();
			io.Fence();io.Write(0xffb20040,1);if(io.Read(0xffb20040)>1)return Fail();io.Fence();
			packet++;issued++;flits+=p.bytes/64;
		}
		return true;
	}
	template<class IO> bool Wait(IO& io) {
		if(failed || completed || !attempted)return false;
		if(pending>=2 || packet!=L::Packets || !Drain(io))return Fail();
		state[pending]=Ready;pending=2;return true;
	}
	bool Acquire(unsigned kblock,unsigned buffer) {
		if(failed || completed || !attempted)return false;
		if(reader!=2 || buffer>=2 || state[buffer]!=Ready || block[buffer]!=kblock)return Fail();
		state[buffer]=Reading;reader=buffer;return true;
	}
	bool Release(unsigned kblock,unsigned buffer) {
		if(failed || completed || !attempted)return false;
		if(buffer>=2 || reader!=buffer || state[buffer]!=Reading || block[buffer]!=kblock)return Fail();
		state[buffer]=Free;reader=2;return true; // caller owes ALL eight math drains
	}
	template<class IO> bool Complete(IO& io) {
		if(failed || completed || !attempted)return false;
		if(reader!=2 || pending!=2 || state[0]!=Free || state[1]!=Free || next!=width/256
			|| issued!=L::Packets*(width/256) || flits!=L::Flits*(width/256))return Fail();
		if(!Drain(io))return Fail();
		completed=true;return true;
	}
};
#endif
