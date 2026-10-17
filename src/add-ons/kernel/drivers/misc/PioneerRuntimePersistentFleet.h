/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RUNTIME_PERSISTENT_FLEET_H
#define PIONEER_RUNTIME_PERSISTENT_FLEET_H
#include "PioneerRuntimeFleet.h"
#include "BlackholeFleetPersistentImage.h"
#include "BlackholeRuntimeQ8InlineImage.h"
#include "BlackholeRuntimeQ8PartialImage.h"
#include "BlackholeRuntimeQ8PhaseImage.h"
#include "BlackholeRuntimeQ8AccumImage.h"
#include "BlackholeCoordinatedWorkerImage.h"
#include "BlackholeAttentionWorkerImage.h"
#include "BlackholeGluWorkerImage.h"
#include "BlackholeTableWorkerImage.h"
#include "BlackholeTableAttentionWorkerImage.h"
#include "BlackholeTableGluWorkerImage.h"
#include "PioneerDecoderDescriptorTable.h"

static inline uint32_t PioneerPersistentPreludeWord(unsigned word,bool coordinated,bool attention,bool glu,bool table)
{
	return table?(glu?BlackholeTableGluWorkerCodeWord(word):attention?BlackholeTableAttentionWorkerCodeWord(word):BlackholeTableWorkerCodeWord(word))
		:glu?BlackholeGluWorkerCodeWord(word):attention?BlackholeAttentionWorkerCodeWord(word):coordinated?BlackholeCoordinatedWorkerCodeWord(word):BlackholeFleetPersistentCodeWord(word);
}

// A completed math job is NOT a stopped RISC. Keep that distinction in the
// ABI, lease and teardown. An unknown command completion forbids ANY reset.
struct tt_persistent_core_result {
	tt_runtime_matvec_result math;
	uint32_t sequence,issued,finished,idle,launched;
};
struct tt_runtime_persistent_fleet_result {
	uint32_t version,state,workers,started,completed,idle,recovery_required,staged_pages;
	tt_persistent_core_result core[TT_PIONEER_WORKERS];
};
static inline bool PioneerPersistentWaitPc(uint32_t pc) {
	return pc>=TT_PERSISTENT_WAIT && pc<=TT_PERSISTENT_WAIT_LAST && !(pc&3);
}
static inline bool PioneerPersistentWorkerWaitPc(const PioneerFleetWorker& f,uint32_t pc) {
	return f.coordinated?(pc>=TT_COORDINATED_WAIT && pc<=TT_COORDINATED_WAIT_LAST && !(pc&3)):PioneerPersistentWaitPc(pc);
}
static inline bool PioneerPersistentProof(const tt_runtime_persistent_fleet_result& r,unsigned k,unsigned n,unsigned limit) {
	PioneerFleetPlan p;
	if (!PioneerFleetPartition(k,n,limit,p) || r.version!=1 || r.state!=TT_RUNTIME_PASSED
		|| r.workers!=p.workers || r.started!=p.workers || r.completed!=p.workers || r.idle!=p.workers
		|| r.recovery_required || r.staged_pages!=(k*2+4095)/4096+1) return false;
	for (unsigned w=0;w<p.workers;w++) {
		const auto& c=r.core[w]; const auto& m=c.math;
		unsigned tiles=(k/16)*(p.rows[w]/16),reads=tiles/32,peer=w!=0;
		if (!c.sequence || c.sequence>=0x7fffffff || c.issued!=c.sequence || c.finished!=c.sequence || c.idle!=1
			|| c.launched>1 || m.version!=2 || m.state!=TT_RUNTIME_PASSED || m.prepared!=1 || m.started!=1
			|| m.completed!=1 || m.stopped || m.context_reset!=c.launched || m.recovery_required || m.nonfinite
			|| m.phase!=TT_RUNTIME_Q8_FINAL_PHASE || m.tiles!=tiles || m.marker!=0x4d415431
			|| !PioneerPersistentWaitPc(m.pc[0]) || !PioneerPersistentWaitPc(m.pc[1])
			|| uint32_t(m.after[0]-m.before[0])!=reads+peer
			|| uint32_t(m.after[1]-m.before[1])!=reads*TT_Q8_BULK_FLITS+256*peer
			|| uint32_t(m.after[2]-m.before[2])!=reads+peer || uint32_t(m.after[3]-m.before[3])!=reads+peer) return false;
	}
	return true;
}
template<class IO>
void PioneerPersistentQuarantine(IO& io,PioneerFleetWorker* fleet) {
	for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) if (fleet[w].Store().ready || fleet[w].Compute().saved) {
		fleet[w].Compute().unsafe=true; fleet[w].Store().recovery_required=true;
	}
	io.QuarantineHostPage();
}
template<class IO>
bool PioneerPersistentIdle(IO& io,PioneerFleetWorker& f,uint32_t* pc=nullptr) {
	if (!f.persistentActive || f.Compute().unsafe || f.Store().recovery_required || f.Store().pending
		|| io.ResetRead()!=0x46800 || io.FleetCodeRead(TT_PERSISTENT_MAILBOX)!=f.persistentSequence
		|| io.FleetCodeRead(TT_PERSISTENT_MAILBOX+1)!=f.persistentSequence
		|| io.FleetCodeRead(TT_PERSISTENT_MAILBOX+2)!=0
		|| io.FleetCodeRead(TT_PERSISTENT_MAILBOX+3)!=(f.coordinated?uint32_t(TT_COORDINATED_READY):uint32_t(TT_PERSISTENT_READY))) return false;
	uint32_t first=io.RuntimePcRead(); io.Pause(); uint32_t second=io.RuntimePcRead();
	if (pc) { pc[0]=first; pc[1]=second; }
	return PioneerPersistentWorkerWaitPc(f,first) && PioneerPersistentWorkerWaitPc(f,second)
		&& io.FleetCodeRead(TT_PERSISTENT_MAILBOX)==f.persistentSequence
		&& io.FleetCodeRead(TT_PERSISTENT_MAILBOX+1)==f.persistentSequence
		&& BlackholeResidentIdle(io) && PioneerFleetLocalSources(io,f.noc,f.Store());
}
template<class IO>
bool PioneerPersistentAllIdle(IO& io,const tt_dram_samples_result& baseline,PioneerFleetWorker* fleet,unsigned workers=TT_PIONEER_WORKERS) {
	if (!workers || workers>TT_PIONEER_WORKERS) return false;
	if (!PioneerFleetGlobalSources(io,baseline)) return false;
	for (unsigned w=0;w<workers;w++) {
		auto& f=fleet[w]; if (!f.Store().ready) continue;
		if (!io.SelectRuntimeCore(w)) return false;
		if (f.persistentActive) {
			if (!f.persistentIdle || f.Compute().running || !PioneerPersistentIdle(io,f)) return false;
		} else if (io.ResetRead()!=0x47800 || f.Compute().running || f.Compute().unsafe
			|| !BlackholeResidentIdle(io) || !PioneerFleetLocalSources(io,f.noc,f.Store())) return false;
	}
	return io.SelectRuntimeCore(0) && PioneerFleetGlobalSources(io,baseline);
}
template<class IO>
bool PioneerPersistentPrepare(IO& io,PioneerFleetWorker& f,bool inlineMath=false,bool coordinated=false,bool attentionDispatch=false,bool gluDispatch=false,bool coordinatedInline=false,bool partialMath=false,bool phasePipeline=false,bool descriptorTable=false,bool accumPipeline=false) {
	if (accumPipeline && (!descriptorTable || !phasePipeline)) return false;
	if (descriptorTable && (!phasePipeline || !coordinatedInline || partialMath)) return false;
	if (phasePipeline && (!coordinatedInline || partialMath)) return false;
	if (partialMath && !coordinatedInline) return false;
	if ((coordinatedInline && (!coordinated || !inlineMath)) || (coordinated && inlineMath && !coordinatedInline)) return false;
	if (gluDispatch && !attentionDispatch) return false;
	if (attentionDispatch && (!coordinated || (inlineMath && !coordinatedInline) || f.noc.version<0x101 || f.noc.version>0x108)) return false;
	if (coordinated && (f.noc.version<0x101 || f.noc.version>0x177)) return false; // peers1..119, never master0
	if (f.persistentPrepared) return f.Compute().ready && !f.Compute().unsafe && f.inlineMath==inlineMath && f.coordinatedInline==coordinatedInline && f.partialMath==partialMath && f.phasePipeline==phasePipeline && f.descriptorTable==descriptorTable && f.accumPipeline==accumPipeline && f.coordinated==coordinated && f.attentionDispatch==attentionDispatch && f.gluDispatch==gluDispatch;
	if (!f.preludeSaved || !f.preludeDirty || !f.Compute().ready || f.Compute().unsafe
		|| f.Compute().running || io.ResetRead()!=0x47800
		|| io.RuntimeCodeRead(1+(TT_RUNTIME_Q8_PARK_PC-0x70000)/4)!=0x0000006f) return false;
	for (unsigned w=0;w<1024;w++) io.FleetCodeWrite(w,PioneerPersistentPreludeWord(w,coordinated,attentionDispatch,gluDispatch,descriptorTable));
	bool okay=true;
	for (unsigned w=0;w<1024;w++) okay &= io.FleetCodeRead(w)==PioneerPersistentPreludeWord(w,coordinated,attentionDispatch,gluDispatch,descriptorTable);
	if (coordinated) {
		for (unsigned w=0;w<16;w++) io.FleetCodeWrite(TT_COORDINATED_SOURCE_WORD+w,w?f.noc.source[0][w]:f.Store().config|0x10000);
		for (unsigned w=0;w<16;w++) okay &= io.FleetCodeRead(TT_COORDINATED_SOURCE_WORD+w)==(w?f.noc.source[0][w]:f.Store().config|0x10000);
	}
	if (inlineMath) {
		for (unsigned w=0;w<1024;w++) io.RuntimeCodeWrite(1+w,accumPipeline?BlackholeRuntimeQ8AccumCodeWord(w):phasePipeline?BlackholeRuntimeQ8PhaseCodeWord(w):partialMath?BlackholeRuntimeQ8PartialCodeWord(w):BlackholeRuntimeQ8InlineCodeWord(w));
		for (unsigned w=0;w<1024;w++) okay &= io.RuntimeCodeRead(1+w)==(accumPipeline?BlackholeRuntimeQ8AccumCodeWord(w):phasePipeline?BlackholeRuntimeQ8PhaseCodeWord(w):partialMath?BlackholeRuntimeQ8PartialCodeWord(w):BlackholeRuntimeQ8InlineCodeWord(w));
	}
	io.RuntimeCodeWrite(1+(TT_RUNTIME_Q8_PARK_PC-0x70000)/4,TT_PERSISTENT_MATH_RETURN);
	okay &= io.RuntimeCodeRead(1+(TT_RUNTIME_Q8_PARK_PC-0x70000)/4)==TT_PERSISTENT_MATH_RETURN;
	f.persistentPrepared=okay; f.inlineMath=inlineMath; f.coordinatedInline=coordinatedInline; f.partialMath=partialMath; f.phasePipeline=phasePipeline; f.descriptorTable=descriptorTable; f.accumPipeline=accumPipeline; f.coordinated=coordinated; f.attentionDispatch=attentionDispatch; f.gluDispatch=gluDispatch; return okay;
}
template<class IO>
bool PioneerPersistentTablePrepare(IO& io,const tt_dram_samples_result& baseline,PioneerFleetWorker& f,const PioneerDecoderJob& model,unsigned worker,bool smallRows=false)
{
	unsigned count=PioneerDecoderDescriptorCount(worker,smallRows);
	if (smallRows && !f.accumPipeline) return false;
	if (!count || !f.descriptorTable || f.descriptorTablePrepared || !f.persistentPrepared || f.persistentActive || f.persistentSequence
		|| !f.Compute().ready || !f.Compute().saved || !f.Compute().bulk || f.Compute().running || f.Compute().unsafe
		|| f.Compute().buffer_words!=TT_Q8_BULK_SAVED_BUFFER_WORDS || io.RuntimeBufferWords()!=TT_Q8_BULK_SAVED_BUFFER_WORDS
		|| io.ResetRead()!=0x47800 || !PioneerRuntimeSources(io,baseline,f.noc,f.Store()) || !BlackholeResidentIdle(io)) return false;
	f.Compute().buffers_dirty=true;
	const unsigned first=(TT_DECODER_TABLE_BASE-0x80000)/4;
	for (unsigned index=0;index<count;index++) { PioneerDecoderDescriptor entry;
		if (!PioneerDecoderDescriptorEntry(model,worker,index,entry,smallRows)) return false;
		for (unsigned word=0;word<8;word++) io.RuntimeBufferWrite(first+index*8+word,entry.words[word]);
	}
	for (unsigned index=0;index<count;index++) { PioneerDecoderDescriptor entry;
		if (!PioneerDecoderDescriptorEntry(model,worker,index,entry,smallRows)) return false;
		for (unsigned word=0;word<8;word++) if (io.RuntimeBufferRead(first+index*8+word)!=entry.words[word]) return false;
	}
	io.FleetCodeWrite((TT_DECODER_TABLE_COUNT-0x71000)/4,count); io.FleetCodeWrite((TT_DECODER_TABLE_CURSOR-0x71000)/4,0);
	if (io.FleetCodeRead((TT_DECODER_TABLE_COUNT-0x71000)/4)!=count || io.FleetCodeRead((TT_DECODER_TABLE_CURSOR-0x71000)/4)
		|| io.ResetRead()!=0x47800 || !PioneerRuntimeSources(io,baseline,f.noc,f.Store()) || !BlackholeResidentIdle(io)) return false;
	f.smallRows=smallRows; f.descriptorTablePrepared=true; return true;
}
template<class IO>
bool PioneerPersistentBoot(IO& io,PioneerFleetWorker& f) {
	if (!f.persistentPrepared || (f.descriptorTable && !f.descriptorTablePrepared) || f.persistentActive || f.persistentSequence || io.ResetRead()!=0x47800
		|| !PioneerFleetLocalSources(io,f.noc,f.Store()) || !BlackholeResidentIdle(io)) return false;
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) return false;
	f.persistentActive=true; f.persistentIdle=false; f.Compute().running=true;
	io.RuntimeResetWrite(0x46800);
	for (unsigned a=0;a<50000;a++) {
		if (PioneerPersistentIdle(io,f)) { f.persistentIdle=true; f.Compute().running=false; return true; }
		io.Pause();
	}
	return false;
}
// STOP is a real command, not a forced reset of a waiting/running worker.
// Every RISC must prove STOP before even the first reset or restoration.
template<class IO>
bool PioneerPersistentStop(IO& io,const tt_dram_samples_result& baseline,PioneerFleetWorker* fleet) {
	if (!PioneerPersistentAllIdle(io,baseline,fleet)) { PioneerPersistentQuarantine(io,fleet); return false; }
	bool okay=true;
	for (unsigned w=0;w<TT_PIONEER_WORKERS && okay;w++) {
		auto& f=fleet[w]; if (!f.persistentActive) continue;
		okay=io.SelectRuntimeCore(w) && f.persistentSequence<0x7fffffff;
		if (!okay) break;
		io.FleetCodeWrite(TT_PERSISTENT_MAILBOX+2,1);
		if (io.FleetCodeRead(TT_PERSISTENT_MAILBOX+2)!=1) { okay=false; break; }
		f.persistentIdle=false;
		io.FleetCodeWrite(TT_PERSISTENT_MAILBOX,++f.persistentSequence);
	}
	for (unsigned w=0;w<TT_PIONEER_WORKERS && okay;w++) {
		auto& f=fleet[w]; if (!f.persistentActive) continue;
		okay=io.SelectRuntimeCore(w); if (!okay) break;
		bool stopped=false;
		for (unsigned a=0;a<50000;a++) {
			if (io.ResetRead()!=0x46800) break;
			if (io.FleetCodeRead(TT_PERSISTENT_MAILBOX)==f.persistentSequence
				&& io.FleetCodeRead(TT_PERSISTENT_MAILBOX+1)==f.persistentSequence
				&& io.FleetCodeRead(TT_PERSISTENT_MAILBOX+3)==(f.coordinated?uint32_t(TT_COORDINATED_STOPPED):uint32_t(TT_PERSISTENT_STOPPED))) {
				uint32_t first=io.RuntimePcRead(); io.Pause(); uint32_t second=io.RuntimePcRead();
				stopped=first==(f.coordinated?uint32_t(TT_COORDINATED_STOP_PC):uint32_t(TT_PERSISTENT_STOP_PC)) && second==first && BlackholeResidentIdle(io)
					&& PioneerFleetLocalSources(io,f.noc,f.Store());
				if (stopped) break;
			}
			io.Pause();
		}
		okay=stopped;
	}
	okay=okay && PioneerFleetGlobalSources(io,baseline);
	if (!okay) { PioneerPersistentQuarantine(io,fleet); return false; }
	for (unsigned w=0;w<TT_PIONEER_WORKERS && okay;w++) {
		auto& f=fleet[w]; if (!f.persistentActive) continue;
		okay=io.SelectRuntimeCore(w); if (!okay) break;
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		okay=io.ResetRead()==0x87ffff && io.ResetRead()==0x87ffff;
		if (!okay) break;
		io.RuntimeResetWrite(0x47800); io.Pause();
		okay=io.ResetRead()==0x47800 && io.ResetRead()==0x47800 && BlackholeResidentIdle(io);
		if (okay) { f.persistentActive=false; f.persistentIdle=false; f.Compute().running=false; }
	}
	okay=okay && io.SelectRuntimeCore(0) && PioneerFleetGlobalSources(io,baseline);
	if (!okay) PioneerPersistentQuarantine(io,fleet);
	return okay;
}
template<class IO>
bool PioneerPersistentMatvec(IO& io,const tt_dram_samples_result& baseline,const tt_host_page_result& host,
	PioneerFleetWorker* fleet,uint64_t capacity,uint64_t offset,unsigned k,unsigned n,unsigned limit,
	const uint32_t* input,const uint32_t* scales,uint32_t* output,tt_runtime_persistent_fleet_result& r)
{
	r={}; r.version=1; PioneerFleetPlan p; uint64_t bytes=PioneerQ8CompactWeightBytes(k,n);
	if (!fleet || !input || !scales || !output || !PioneerFleetPartition(k,n,limit,p)
		|| !PioneerResidentOffset(capacity,offset) || !bytes || bytes>capacity-offset) return false;
	for (unsigned i=0;i<k/2;i++) if (!PioneerInt8OperandValid(uint16_t(input[i])) || !PioneerInt8OperandValid(uint16_t(input[i]>>16))) return false;
	for (unsigned i=0;i<k/32;i++) if ((scales[i]&0x7f800000)==0x7f800000) return false;
	for (unsigned w=0;w<p.workers;w++) if (!fleet[w].persistentPrepared || !fleet[w].Compute().ready
		|| fleet[w].persistentSequence>=0x7ffffffe) return false;
	r.workers=p.workers; r.state=TT_RUNTIME_CONFIG_FAILED; bool okay=false,uncertain=false;
	do {
		// Other workers retain their last proved idle mailbox. No command or
		// code write targets them; full-fleet audits are reserved for STOP.
		if (!PioneerPersistentAllIdle(io,baseline,fleet,p.workers) || !BlackholeHostReadMapStable(io,host)) { uncertain=true; break; }
		uint32_t* stage=io.RuntimeStageWords(); if (!stage) break;
		fleet[0].Compute().buffers_dirty=true; tt_runtime_matvec_result staged={}; bool transferred=true;
		for (unsigned page=0;page<(k*2+4095)/4096 && transferred;page++) {
			for (unsigned i=0;i<1024;i++) stage[i]=page*1024+i<k/2?input[page*1024+i]:0;
			transferred=PioneerRuntimeStage(io,baseline,fleet[0].noc,host,fleet[0].Store(),false,page,stage,staged);
		}
		if (transferred) {
			for (unsigned i=0;i<1024;i++) stage[i]=i<k/32?scales[i]:0;
			transferred=PioneerRuntimeStage(io,baseline,fleet[0].noc,host,fleet[0].Store(),false,3,stage,staged,true);
		}
		r.staged_pages=staged.staged_pages; if (!transferred) break;
		io.RuntimeProfileMark(TT_PROFILE_STAGED);
		for (unsigned w=0;w<p.workers;w++) {
			auto& f=fleet[w]; auto& c=r.core[w]; auto& m=c.math; m.version=2; m.prepared=1;
			if (!io.SelectRuntimeCore(w)) { uncertain=true; break; }
			c.launched=!f.persistentActive; m.context_reset=c.launched;
			if (!f.persistentActive && !PioneerPersistentBoot(io,f)) { uncertain=true; break; }
			if (!PioneerPersistentIdle(io,f)) { uncertain=true; break; }
			uint32_t address=PioneerResidentAddress(capacity,offset+(w?PioneerQ8CompactWeightBytes(k,p.first[w]):0));
			const uint32_t descriptor[]={address,k/16,p.rows[w]/16,uint32_t(w!=0),PioneerWorkerX(w)|(PioneerWorkerY(w)<<6)};
			for (unsigned i=0;i<5;i++) io.RuntimeCodeWrite(1+0x500/4+i,descriptor[i]);
			bool valid=true;
			for (unsigned i=0;i<5;i++) valid &= io.RuntimeCodeRead(1+0x500/4+i)==descriptor[i];
			if (!valid) break;
			for (unsigned i=0;i<3;i++) io.RuntimeStatusWrite(i,0);
			for (unsigned i=0;i<4;i++) m.before[i]=io.ResidentNocRead(0x208+i*4);
			f.Compute().buffers_dirty=true; f.Store().l1_dirty=true; f.Store().l1_restored=false;
			f.persistentIdle=false; f.Compute().running=true; c.sequence=++f.persistentSequence;
			m.started=1; r.started++; io.FleetCodeWrite(TT_PERSISTENT_MAILBOX,c.sequence);
		}
		if (uncertain || r.started!=p.workers) break;
		io.RuntimeProfileMark(TT_PROFILE_LAUNCHED);
		for (unsigned attempt=0;attempt<50000 && r.completed<p.workers;attempt++) {
			for (unsigned w=0;w<p.workers;w++) {
				auto& f=fleet[w]; auto& c=r.core[w]; auto& m=c.math; if (m.completed) continue;
				if (!io.SelectRuntimeCore(w) || io.ResetRead()!=0x46800) { uncertain=true; break; }
				m.polls++; m.marker=io.RuntimeStatusRead(2);
				if (m.marker==0x4e4f4346) { uncertain=true; break; }
				if (!PioneerPersistentIdle(io,f,m.pc)) continue;
				// The initial marker read is only an early fault check. It can
				// precede completion even when the subsequent mailbox/PC proof
				// succeeds. Snapshot ALL math metadata after that proof.
				m.phase=io.RuntimeStatusRead(0); m.tiles=io.RuntimeStatusRead(1); m.marker=io.RuntimeStatusRead(2);
				c.issued=io.FleetCodeRead(TT_PERSISTENT_MAILBOX); c.finished=io.FleetCodeRead(TT_PERSISTENT_MAILBOX+1);
				for (unsigned i=0;i<4;i++) m.after[i]=io.ResidentNocRead(0x208+i*4);
				unsigned tiles=(k/16)*(p.rows[w]/16),reads=tiles/32,peer=w!=0;
				if (m.phase!=TT_RUNTIME_Q8_FINAL_PHASE || m.tiles!=tiles || m.marker!=0x4d415431
					|| uint32_t(m.after[0]-m.before[0])!=reads+peer || uint32_t(m.after[1]-m.before[1])!=reads*144+peer*256
					|| uint32_t(m.after[2]-m.before[2])!=reads+peer || uint32_t(m.after[3]-m.before[3])!=reads+peer) { uncertain=true; break; }
				m.completed=1; r.completed++; c.idle=1; r.idle++; f.persistentIdle=true; f.Compute().running=false;
			}
			if (uncertain) break;
			if (r.completed<p.workers) io.Pause();
		}
		if (uncertain || r.completed!=p.workers) break;
		if (!PioneerPersistentAllIdle(io,baseline,fleet,p.workers)) { uncertain=true; break; }
		io.RuntimeProfileMark(TT_PROFILE_COMPLETED);
		for (unsigned w=0;w<p.workers;w++) {
			if (!io.SelectRuntimeCore(w)) { uncertain=true; break; }
			auto& m=r.core[w].math;
			for (unsigned i=0;i<p.rows[w];i++) { uint32_t v=io.RuntimeBufferRead(4096+i); output[p.first[w]+i]=v; m.nonfinite+=(v&0x7f800000)==0x7f800000; }
			m.state=m.nonfinite?TT_RUNTIME_NONFINITE:TT_RUNTIME_PASSED;
		}
		if (uncertain || !PioneerFleetGlobalSources(io,baseline)) { uncertain=true; break; }
		io.RuntimeProfileMark(TT_PROFILE_PIO); r.state=TT_RUNTIME_PASSED;
		okay=PioneerPersistentProof(r,k,n,limit); if (!okay) r.state=TT_RUNTIME_NONFINITE;
	} while (false);
	for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) uncertain |= fleet[w].Compute().running || fleet[w].Compute().unsafe
		|| fleet[w].Store().pending || fleet[w].Store().recovery_required
		|| (fleet[w].persistentActive && !fleet[w].persistentIdle);
	if (uncertain) { r.recovery_required=1; PioneerPersistentQuarantine(io,fleet); }
	return okay && !uncertain;
}
#endif
