/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RUNTIME_READBACK_H
#define PIONEER_RUNTIME_READBACK_H
#include "PioneerRuntimeDual.h"
#include "PioneerOutputBuffer.h"
#include "BlackholeRuntimeQ8ReadbackImage.h"

// OFFLINE-ONLY controller prerequisite. No native adapter, ioctl or driver
// capability calls this. It compares two already populated L1 ranges; it does
// NOT establish DMA freshness/order, accept/copy output, or release ownership.
// A future transport controller must first prove both math stops, completed
// transfers, CPU-visible poison replacement and the frozen host snapshot.
// Its owner/team lock must span those operations AND this comparison.
enum {
	TT_READBACK_REFUSED=0,TT_READBACK_PASSED=1,TT_READBACK_MISMATCH=2,
	TT_READBACK_PREPARE_FAILED=3,TT_READBACK_COMPLETION_FAILED=4,TT_READBACK_COLD=5
};
struct PioneerRuntimeReadbackResult {
	uint32_t version=1,state=TT_READBACK_REFUSED,armed=0,started=0,completed=0,stopped=0,cold=0;
	uint32_t phase=0,words=0,marker=0,mismatch=0,pc[2]={},before[8]={},after[8]={};
};
template<class IO>
bool PioneerRuntimeReadbackGuard(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& noc,const tt_host_page_result& input,
	const PioneerOutputBufferLease& output,const PioneerResidentLease& store,unsigned core,uint32_t reset)
{
	if (core>1 || noc.version!=(core?2U:1U) || !store.ready || store.pending || store.recovery_required
		|| !output.ready || !output.memory_type_set || output.pending || output.quarantined || output.released
		|| io.ResetRead()!=reset || io.ResetRead()!=reset || !BlackholeResidentIdle(io)
		|| !PioneerRuntimeSources(io,baseline,noc,store) || !BlackholeHostReadMapStable(io,input)
		|| !PioneerOutputBufferMapStable(io,output,input)) return false;
	for (unsigned slot=0;slot<2;slot++) for (unsigned w=0;w<3;w++)
		if (io.DramTlbRead(slot,w)!=BlackholeDramFixedTlbWord(slot?0xffa00000:0,1,2+core,false,w)) return false;
	return true;
}
template<class IO>
bool PioneerRuntimeReadbackCode(IO& io,unsigned words,bool configured)
{
	if (io.RuntimeCodeRead(0)!=TT_Q8_READBACK_ENTRY) return false;
	// Only this fixed tail is executable in comparison mode. The qualified
	// math body is not entered, patched or used to compare a checksum.
	for (unsigned w=0xf54/4;w<1024;w++)
		if (io.RuntimeCodeRead(1+w)!=BlackholeRuntimeQ8ReadbackCodeWord(w)) return false;
	return !configured || (io.RuntimeCodeRead(1+0x508/4)==words/16
		&& io.RuntimeCodeRead(1+0x50c/4)==1);
}
template<class IO>
void PioneerRuntimeReadbackCounters(IO& io,uint32_t (&values)[8])
{
	for (unsigned w=0;w<4;w++) {
		values[w]=io.ResidentNocRead(0x208+w*4);
		values[4+w]=io.ResidentNocRead(BlackholeHostWriteCounter(w));
	}
}
template<class IO>
bool PioneerRuntimeReadbackCompletionStable(IO& io,const PioneerRuntimeReadbackResult& r)
{
	if (!r.completed || io.ResidentL1Read(0x1fc0/4)!=r.phase || io.ResidentL1Read(0x1fc4/4)!=r.words
		|| io.ResidentL1Read(0x1fc8/4)!=r.marker || io.ResidentL1Read(0x1fcc/4)!=r.mismatch) return false;
	io.Pause(); if (io.RuntimePcRead()!=TT_Q8_READBACK_PARK_PC) return false;
	io.Pause(); if (io.RuntimePcRead()!=TT_Q8_READBACK_PARK_PC) return false;
	uint32_t after[8]; PioneerRuntimeReadbackCounters(io,after);
	for (unsigned w=0;w<8;w++) if (after[w]!=r.before[w]) return false;
	return io.ResidentL1Read(0x1fc0/4)==r.phase && io.ResidentL1Read(0x1fc4/4)==r.words
		&& io.ResidentL1Read(0x1fc8/4)==r.marker && io.ResidentL1Read(0x1fcc/4)==r.mismatch;
}
template<class IO>
bool PioneerRuntimeVerifyReadback(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& firstNoc,const tt_compute_noc_result& secondNoc,
	const tt_host_page_result& input,PioneerOutputBufferLease& output,
	PioneerResidentLease (&store)[2],PioneerRuntimeMatvecLease (&compute)[2],
	const tt_runtime_matvec_result (&math)[2],unsigned inputs,unsigned outputs,
	PioneerRuntimeReadbackResult (&r)[2])
{
	for (auto& part:r) part=PioneerRuntimeReadbackResult{};
	if (!PioneerRuntimeDualProof(math,inputs,outputs) || firstNoc.version!=1 || secondNoc.version!=2
		|| !output.ready || !output.memory_type_set || output.pending || output.quarantined || output.released) return false;
	unsigned words=((outputs/2+1023)/1024)*1024; // compare poisoned padding, too
	if (!words || words>TT_Q8_READBACK_MAX_WORDS) return false;
	for (unsigned core=0;core<2;core++) if (!compute[core].saved || !compute[core].ready || !compute[core].bulk
		|| compute[core].running || compute[core].unsafe || !store[core].ready
		|| store[core].pending || store[core].recovery_required) return false;
	const tt_compute_noc_result* noc[2]={&firstNoc,&secondNoc};
	bool known=false,uncertain=false;
	do {
		// BOTH workers and BOTH host mappings must be proved before even
		// clearing metadata or arming a context. No host DMA is issued here.
		for (unsigned core=0;core<2;core++) if (!io.SelectRuntimeCore(core)
			|| !PioneerRuntimeReadbackGuard(io,baseline,*noc[core],input,output,store[core],core,0x47800)
			|| !PioneerRuntimeReadbackCode(io,words,false)) { uncertain=true; break; }
		if (uncertain) break;
		for (unsigned core=0;core<2;core++) {
			r[core].state=TT_READBACK_PREPARE_FAILED;
			if (!io.SelectRuntimeCore(core)
				|| !PioneerRuntimeReadbackGuard(io,baseline,*noc[core],input,output,store[core],core,0x47800)) { uncertain=true; break; }
			compute[core].code_dirty=true;
			store[core].l1_dirty=true; store[core].l1_restored=false;
			io.RuntimeCodeWrite(1+0x508/4,words/16); io.RuntimeCodeWrite(1+0x50c/4,1);
			for (unsigned w=0;w<4;w++) io.ResidentL1Write(0x1fc0/4+w,0);
			bool cleared=true;
			for (unsigned w=0;w<4;w++) cleared &= io.ResidentL1Read(0x1fc0/4+w)==0;
			if (!cleared || !PioneerRuntimeReadbackCode(io,words,true)
				|| !PioneerRuntimeReadbackGuard(io,baseline,*noc[core],input,output,store[core],core,0x47800)) { uncertain=true; break; }
			io.RuntimeResetWrite(0x87ffff); io.Pause();
			if (!PioneerRuntimeReadbackGuard(io,baseline,*noc[core],input,output,store[core],core,0x87ffff)) { uncertain=true; break; }
			PioneerRuntimeReadbackCounters(io,r[core].before); r[core].armed=1;
		}
		if (uncertain) break;
		for (unsigned core=0;core<2;core++) {
			if (!io.SelectRuntimeCore(core)
				|| !PioneerRuntimeReadbackGuard(io,baseline,*noc[core],input,output,store[core],core,0x87ffff)
				|| !PioneerRuntimeReadbackCode(io,words,true)) { uncertain=true; break; }
			r[core].started=1; r[core].state=TT_READBACK_COMPLETION_FAILED; compute[core].running=true;
			io.RuntimeResetWrite(0x46800);
			if (!PioneerRuntimeReadbackGuard(io,baseline,*noc[core],input,output,store[core],core,0x46800)) { uncertain=true; break; }
		}
		if (uncertain) break;
		for (unsigned attempt=0;attempt<256 && !(r[0].completed && r[1].completed);attempt++) {
			for (unsigned core=0;core<2;core++) {
				if (r[core].completed) continue;
				if (!io.SelectRuntimeCore(core)
					|| !PioneerRuntimeReadbackGuard(io,baseline,*noc[core],input,output,store[core],core,0x46800)) { uncertain=true; break; }
				auto& part=r[core];
				part.phase=io.ResidentL1Read(0x1fc0/4); part.words=io.ResidentL1Read(0x1fc4/4);
				part.marker=io.ResidentL1Read(0x1fc8/4); part.mismatch=io.ResidentL1Read(0x1fcc/4);
				if (part.phase!=TT_Q8_READBACK_PHASE || part.words!=words || part.marker!=TT_Q8_READBACK_MARKER) continue;
				io.Pause(); part.pc[0]=io.RuntimePcRead(); io.Pause(); part.pc[1]=io.RuntimePcRead();
				PioneerRuntimeReadbackCounters(io,part.after);
				bool quiet=true; for (unsigned w=0;w<8;w++) quiet &= part.before[w]==part.after[w];
				if (!quiet || part.pc[0]!=TT_Q8_READBACK_PARK_PC || part.pc[1]!=part.pc[0]
					|| !PioneerRuntimeReadbackGuard(io,baseline,*noc[core],input,output,store[core],core,0x46800)) { uncertain=true; break; }
				part.completed=1; // a known mismatch is still a completed comparison
			}
			if (uncertain) break;
			if (!(r[0].completed && r[1].completed)) io.Pause();
		}
		if (uncertain || !r[0].completed || !r[1].completed) { uncertain=true; break; }
		// Prove BOTH comparisons before stopping either. After an unknown
		// completion/route/map/counter/stop, never reset a peer or rewrite L1.
		for (unsigned core=0;core<2;core++) if (!io.SelectRuntimeCore(core)
			|| !PioneerRuntimeReadbackGuard(io,baseline,*noc[core],input,output,store[core],core,0x46800)
			|| !PioneerRuntimeReadbackCompletionStable(io,r[core])) { uncertain=true; break; }
		if (uncertain) break;
		for (unsigned core=0;core<2;core++) {
			if (!io.SelectRuntimeCore(core)
				|| !PioneerRuntimeReadbackGuard(io,baseline,*noc[core],input,output,store[core],core,0x46800)
				|| !PioneerRuntimeReadbackCompletionStable(io,r[core])) { uncertain=true; break; }
			io.RuntimeResetWrite(0x87ffff); io.Pause();
			if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) { uncertain=true; break; }
			io.RuntimeResetWrite(0x47800); io.Pause();
			if (!PioneerRuntimeReadbackGuard(io,baseline,*noc[core],input,output,store[core],core,0x47800)) { uncertain=true; break; }
			r[core].stopped=1; compute[core].running=false;
			r[core].state=r[core].mismatch?TT_READBACK_MISMATCH:TT_READBACK_PASSED;
		}
		known=!uncertain;
	} while (false);
	if (uncertain) {
		for (unsigned core=0;core<2;core++) {
			compute[core].unsafe=true; store[core].recovery_required=true;
			r[core].cold=1; r[core].state=TT_READBACK_COLD;
		}
		io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(io,output);
	}
	// No output acceptance/copy, buffer release, DMA, PCI/NIU write, or
	// payload restoration exists here. Selector remains1; a future math
	// controller MUST explicitly restore/read back selector0 before launch.
	return known && !r[0].mismatch && !r[1].mismatch;
}
#endif
