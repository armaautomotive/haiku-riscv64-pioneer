/* SPDX-License-Identifier: MIT */
#define PIONEER_SECOND_CORE_MOCK_ONLY
#include "pioneer_runtime_second_core_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimeDual.h"
struct DualMock {
	RuntimeMock first;
	SecondRuntimeMock second;
	unsigned current=0,selects=0,failSelect=0;
	unsigned pcReads[2]={},allStarts=0;
	bool unsafe=false;
	PioneerRuntimeProfile profile;
	uint64_t now=100;
	bool SelectRuntimeCore(unsigned core) {
		assert(core<2); selects++;
		if (selects==failSelect) { unsafe=true; return false; }
		current=core; return true;
	}
#define DUAL_READ(type,name,args,call) type name args { return current?second.name call:first.name call; }
#define DUAL_WRITE(name,args,call) void name args { if (current) second.name call; else first.name call; }
	DUAL_READ(uint32_t,SourceRead,(unsigned w),(w))
	DUAL_READ(uint32_t,HostPciRead,(unsigned p,uint16_t o,uint8_t s),(p,o,s))
	DUAL_READ(uint32_t,HostPageRead,(unsigned w),(w))
	DUAL_WRITE(HostPageWrite,(unsigned w,uint32_t v),(w,v))
	DUAL_READ(int32_t,HostPageMap,(uint64_t& p,uint64_t& b),(p,b))
	DUAL_READ(uint32_t,ResetRead,(),())
	DUAL_READ(uint32_t,RuntimeCodeRead,(unsigned w),(w))
	DUAL_WRITE(RuntimeCodeWrite,(unsigned w,uint32_t v),(w,v))
	DUAL_READ(uint32_t,RuntimeStatusRead,(unsigned w),(w))
	DUAL_WRITE(RuntimeStatusWrite,(unsigned w,uint32_t v),(w,v))
	DUAL_READ(uint32_t,ResidentNocRead,(uint32_t o),(o))
	void ResidentNocWrite(uint32_t o,uint32_t v) {
		assert(!unsafe); if (o==0x40) assert(!allStarts); // NO host DMA after either launch
		if (current) second.ResidentNocWrite(o,v); else first.ResidentNocWrite(o,v);
	}
	void HostPciWrite(unsigned p,uint16_t o,uint8_t s,uint32_t v) {
		if (v==0x406) assert(!allStarts && !unsafe);
		if (current) { second.HostPciWrite(p,o,s,v); first.pciCommand[p]=second.pciCommand[p]; }
		else { first.HostPciWrite(p,o,s,v); second.pciCommand[p]=first.pciCommand[p]; }
	}
	uint32_t* RuntimeStageWords() { return first.stage; }
	void RuntimeProfileMark(unsigned slot) { profile.Mark(now+=50,slot); }
	uint32_t RuntimePcRead() { pcReads[current]++; return current?second.RuntimePcRead():first.RuntimePcRead(); }
	uint32_t RuntimeBufferRead(unsigned w) {
		assert(first.starts && second.starts && pcReads[0]>=2 && pcReads[1]>=2);
		assert(!unsafe && !first.executeTimeout && !second.executeTimeout && !first.badPc && !second.badPc);
		return current?second.RuntimeBufferRead(w):first.RuntimeBufferRead(w);
	}
	void RuntimeResetWrite(uint32_t v) {
		assert(!unsafe && !first.uncertain && !second.uncertain);
		if (v==0x46800) {
			assert(first.code[1+0x500/4]==0x04000000 && second.code[1+0x500/4]==0x04009000);
			if (!allStarts) assert(first.reset==0x87ffff && second.reset==0x87ffff);
			else assert(allStarts==1 && first.reset==0x46800 && second.reset==0x87ffff);
		}
		if (v==0x87ffff && allStarts) assert(pcReads[0]>=2 && pcReads[1]>=2 && first.starts && second.starts);
		if (current) second.RuntimeResetWrite(v); else first.RuntimeResetWrite(v);
		if (v==0x46800) allStarts++;
	}
	void Pause() { first.Pause(); }
	void QuarantineHostPage() { unsafe=true; first.QuarantineHostPage(); second.QuarantineHostPage(); }
#undef DUAL_READ
#undef DUAL_WRITE
};
struct Fixture {
	DualMock io;
	tt_host_page_result host[2];
	PioneerResidentLease store[2];
	PioneerRuntimeMatvecLease compute[2];
	tt_runtime_matvec_result result[2]={};
	Fixture() {
		io.first.bufferWords=12288;
		assert(Prepared(io.first,host[0],store[0]) && SecondPrepared(io.second,host[1],store[1]));
		assert(host[0].physical==host[1].physical && host[0].bytes==host[1].bytes);
		auto b=io.first.Baseline(); auto n=io.first.Compute(),n2=n; n2.version=2;
		assert(PioneerRuntimeMatvecPrepare(io.first,b,n,store[0],compute[0],false,true,true,true,true,true,true));
		assert(PioneerRuntimeMatvecPrepare(io.second,b,n2,store[1],compute[1],false,true,true,true,true,true,true));
		io.profile.Begin(io.now,10); io.RuntimeProfileMark(TT_PROFILE_PREPARED);
	}
	bool Run(unsigned n=64) {
		assert(io.first.reset==0x47800 && io.second.reset==0x47800);
		io.allStarts=0; io.pcReads[0]=io.pcReads[1]=0;
		auto b=io.first.Baseline(); auto first=io.first.Compute(),second=first; second.version=2;
		uint32_t input[512],scales[32],output[64]={};
		for (auto& v:input) v=0x40014001;
		for (auto& v:scales) v=0x3c800000;
		bool passed=PioneerRuntimeDualMatvec(io,b,first,second,host[0],store[0],store[1],compute[0],compute[1],73728,0,1024,n,input,scales,output,result);
		if (passed) {
			for (auto v:output) assert(v==0x40000000);
			assert(io.first.code[1+0x500/4]==0x04000000 && io.second.code[1+0x500/4]==0x04009000);
			for (const auto& r:result) assert(r.started && r.completed && r.stopped && r.tiles==128 && r.after[0]-r.before[0]==4);
		}
		return passed;
	}
	void Close() {
		auto b=io.first.Baseline(); auto n=io.first.Compute(),n2=n; n2.version=2;
		if (io.unsafe) {
			assert(result[0].recovery_required && result[1].recovery_required && compute[0].unsafe && compute[1].unsafe);
			assert(io.first.quarantined && io.second.quarantined && !io.first.releases && !io.second.releases);
			assert(!PioneerRuntimeMatvecClose(io.first,b,n,store[0],compute[0]));
			assert(!PioneerRuntimeMatvecClose(io.second,b,n2,store[1],compute[1])); return;
		}
		assert(PioneerRuntimeMatvecClose(io.first,b,n,store[0],compute[0]) && PioneerResidentLeaseEnd(io.first,n,store[0]));
		assert(PioneerRuntimeMatvecClose(io.second,b,n2,store[1],compute[1]) && PioneerResidentLeaseEnd(io.second,n2,store[1]));
		io.first.Restored(); io.second.Restored();
	}
};
int main()
{
	Fixture baseline; assert(baseline.Run());
	assert(PioneerRuntimeDualProof(baseline.result,1024,64));
	for (unsigned core=0;core<2;core++) for (unsigned field:{0U,1U,2U,3U,4U,5U,6U,7U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,23U}) {
		tt_runtime_matvec_result parts[2]={baseline.result[0],baseline.result[1]};
		uint32_t* words=(uint32_t*)&parts[core]; words[field]^=1;
		assert(!PioneerRuntimeDualProof(parts,1024,64));
	}
	unsigned writes[2]={baseline.io.first.Mock::writes,baseline.io.second.Mock::writes},selects=baseline.io.selects;
	assert(baseline.Run()); baseline.Close(); unsigned cases=0;
	for (unsigned core=0;core<2;core++) for (unsigned drop=0;drop<=writes[core];drop++) {
		Fixture f; unsigned start=core?f.io.second.Mock::writes:f.io.first.Mock::writes;
		if (drop<=start) continue;
		if (core) f.io.second.Mock::drop=drop; else f.io.first.Mock::drop=drop;
		cases++; f.Run(); f.Close();
	}
	for (unsigned select=1;select<=selects;select++) {
		Fixture f; f.io.failSelect=select; assert(!f.Run()); f.Close();
	}
	for (unsigned core=0;core<2;core++) for (unsigned fault=0;fault<7;fault++) {
		Fixture f; RuntimeMock& target=core?static_cast<RuntimeMock&>(f.io.second):f.io.first;
		target.executeTimeout=fault==0; target.badPc=fault==1; target.badMathFlits=fault==2; target.stopFails=fault==3;
		target.failRequest=fault==4?2:0; target.clearFails=fault==5; target.changed=fault==6;
		assert(!f.Run());
		if (fault<3) assert(!f.io.first.stops && !f.io.second.stops);
		if (fault==3 && core==0) assert(!f.io.second.stops); // no peer stop after uncertain stop
		f.Close();
	}
	{
		Fixture f; f.io.second.nonfinite=true; assert(!f.Run());
		assert(!f.io.unsafe && f.result[0].stopped && f.result[1].stopped && f.result[1].nonfinite==32); f.Close();
	}
	for (unsigned inputs:{1024U,2048U,3072U,5120U,6144U}) for (unsigned n=0;n<=4112;n+=16) {
		bool valid=n>=64 && n<=4096 && !(n&63);
		assert(PioneerRuntimeDualShape(inputs,n)==valid);
		if (valid) assert(PioneerQ8CompactWeightRange(PioneerQ8CompactWeightBytes(inputs,n),PioneerQ8CompactWeightBytes(inputs,n/2),inputs,n/2));
	}
	printf("PASS: two fixed workers, row partitions, both staged before launch, both proved before results/stop, %u lost writes and %u mapping faults; OFFLINE only\n",cases,selects);
}
