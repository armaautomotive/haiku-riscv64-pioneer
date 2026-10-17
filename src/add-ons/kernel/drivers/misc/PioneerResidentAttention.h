/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RESIDENT_ATTENTION_H
#define PIONEER_RESIDENT_ATTENTION_H
#include "PioneerAttentionTile.h"
#include "PioneerBulkUpload.h"
#include "PioneerPayloadBlock.h"
enum { TT_RESIDENT_KV_BYTES=28*1024*1024, TT_RESIDENT_ATTN_INPUT_WORDS=3072 };
struct PioneerResidentAttentionLease {
	bool reserved=false;
	bool payloadBlocks=false; // immutable v2 choice, independent capability
	bool coordinated=false; // explicit v3, only immutable mode8 may select it
	unsigned tokens[28]={};
	uint32_t snapshot[2048]={}; // private ordinary kernel RAM, never a DMA owner
};
struct tt_resident_attention_result {
	tt_attention_tile_result math;
	uint32_t layer,groups,before[8],after[8];
};
static inline bool PioneerResidentAttentionVersion(unsigned version,const PioneerResidentAttentionLease& kv,bool coordinated=false)
{
	return (coordinated?version==3:(version==1 || version==2))
		&& (!kv.reserved || (kv.payloadBlocks==(version>=2) && kv.coordinated==coordinated));
}
static inline bool PioneerResidentAttentionCapacity(uint64_t bytes)
{
	return PioneerResidentCapacity(bytes) && bytes>=TT_RESIDENT_KV_BYTES+4096;
}
static inline bool PioneerResidentAttentionInput(unsigned tokens,unsigned layer,const uint32_t* input)
{
	if (!input || !tokens || tokens>256 || layer>=28) return false;
	for (unsigned w=0;w<2048;w++) if ((input[w]&0x7fffffff)>0x477fe000) return false;
	for (unsigned w=2048;w<3072;w++)
		if ((input[w]&0x7c00)==0x7c00 || (input[w]&0x7c000000)==0x7c000000) return false;
	return true;
}
template<class IO>
void PioneerResidentAttentionCounters(IO& io,uint32_t* values)
{
	for (unsigned w=0;w<4;w++) {
		values[w]=io.ResidentNocRead(0x208+w*4);
		values[4+w]=io.ResidentNocRead(BlackholeHostWriteCounter(w));
	}
}
static inline bool PioneerResidentAttentionCounterProof(unsigned tokens,const uint32_t* before,const uint32_t* after)
{
	if (!tokens || tokens>256) return false;
	unsigned reads=16*((tokens+63)/64);
	const uint32_t expected[8]={reads,64*tokens,reads+16,reads,16,64,reads+16,16};
	for (unsigned w=0;w<8;w++) if (uint32_t(after[w]-before[w])!=expected[w]) return false;
	return true;
}
template<class IO>
bool PioneerResidentAttentionRun(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& noc,PioneerResidentLease& store,PioneerAttentionTileLease& compute,
	PioneerBulkUploadLease& bulk,PioneerResidentAttentionLease& kv,uint64_t capacity,
	unsigned tokens,unsigned layer,const uint32_t* input,uint32_t* output,tt_resident_attention_result& result)
{
	result={}; auto& r=result.math; r.version=1; result.layer=layer;
	if (!output || !PioneerResidentAttentionCapacity(capacity) || !PioneerResidentAttentionInput(tokens,layer,input)
		|| !kv.reserved || tokens>kv.tokens[layer]+1 || !compute.ready || !compute.resident
		|| compute.unsafe || compute.running || !bulk.ready || bulk.unsafe || !bulk.saved
		|| baseline.translated[0][0]!=18 || baseline.translated[0][1]!=12
		|| !io.BulkMappingStable() || !PioneerAttentionTileHeld(io,baseline,noc,store)) return false;
	r.tokens=tokens; r.prepared=1; r.state=TT_ATTN_TILE_STAGE_FAILED;
	bulk.dirty=true;
	if (!PioneerBulkPayloadStage(io,input,3072,0)) return false;
	r.staged_words=3072;
	compute.data_dirty=true;
	{
		const uint32_t descriptor[]={0x3152564b,tokens,layer,PioneerResidentAddress(capacity,capacity-TT_RESIDENT_KV_BYTES)};
		if (!PioneerAttentionTileStage(io,TT_ATTN_TILE_DESC,4,descriptor)) return false;
	}
	store.l1_dirty=true; store.l1_restored=false;
	for (unsigned w=0;w<3;w++) io.RuntimeStatusWrite(w,0);
	for (unsigned w=0;w<3;w++) if (io.RuntimeStatusRead(w)) return false;
	if (io.AttentionReadWord(0)!=TT_ATTENTION_TILE_BOOT || !PioneerAttentionTileHeld(io,baseline,noc,store)) return false;
	r.state=TT_ATTN_TILE_START_FAILED; r.context_reset=1;
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerRuntimeSources(io,baseline,noc,store)
		|| !BlackholeResidentIdle(io)) goto finished;
	PioneerResidentAttentionCounters(io,result.before);
	for (unsigned w=0;w<4;w++) r.before[w]=result.before[w];
	r.started=1; compute.running=true; r.state=TT_ATTN_TILE_COMPLETION_FAILED;
	io.RuntimeResetWrite(0x46800);
	for (unsigned attempt=0;attempt<50000;attempt++) {
		r.polls++;
		if (io.ResetRead()!=0x46800) break;
		r.phase=io.RuntimeStatusRead(0); r.count=io.RuntimeStatusRead(1); r.marker=io.RuntimeStatusRead(2);
		if (r.marker==0x464e5441) break;
		if (r.phase==TT_ATTENTION_TILE_DONE && r.count==tokens && r.marker==TT_ATTENTION_TILE_MARKER) {
			io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
			PioneerResidentAttentionCounters(io,result.after);
			for (unsigned w=0;w<4;w++) r.after[w]=result.after[w];
			if (r.pc[0]==TT_ATTENTION_TILE_PARK && r.pc[1]==r.pc[0] && BlackholeResidentIdle(io)
				&& PioneerResidentAttentionCounterProof(tokens,result.before,result.after)) { r.completed=1; break; }
		}
		io.Pause();
	}
	if (!r.completed || io.ResetRead()!=0x46800 || !PioneerRuntimeSources(io,baseline,noc,store)
		|| !BlackholeResidentIdle(io)) goto finished;
	// Result stays private until the entire eight-head operation and stop.
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) goto finished;
	io.RuntimeResetWrite(0x47800); io.Pause();
	r.stopped=io.ResetRead()==0x47800 && io.ResetRead()==0x47800;
	if (!r.stopped) goto finished;
	compute.running=false;
	if (!PioneerAttentionTileHeld(io,baseline,noc,store)) goto finished;
	if (!PioneerBulkPayloadRead(io,4096,2048,kv.snapshot,0)) goto finished;
	for (unsigned w=0;w<2048;w++) {
		r.nonfinite+=(kv.snapshot[w]&0x7f800000)==0x7f800000;
	}
	if (r.nonfinite) { r.state=TT_ATTN_TILE_NONFINITE; goto finished; }
	if (!PioneerAttentionTileHeld(io,baseline,noc,store)) goto finished;
	for (unsigned w=0;w<2048;w++) output[w]=kv.snapshot[w];
	kv.tokens[layer]=tokens; result.groups=8; r.state=TT_ATTN_TILE_PASSED;
finished:
	if (!r.started && r.context_reset && io.ResetRead()==0x87ffff && io.ResetRead()==0x87ffff) {
		io.RuntimeResetWrite(0x47800); io.Pause();
	}
	if (store.pending || store.recovery_required || (r.started && (!r.completed || !r.stopped))
		|| (!r.started && io.ResetRead()!=0x47800) || (r.stopped && !PioneerAttentionTileHeld(io,baseline,noc,store))) {
		compute.unsafe=true; store.recovery_required=true; r.recovery_required=1; io.QuarantineHostPage();
	}
	return r.state==TT_ATTN_TILE_PASSED && !r.recovery_required;
}
#endif
