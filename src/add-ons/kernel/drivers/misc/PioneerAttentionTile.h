/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_ATTENTION_TILE_H
#define PIONEER_ATTENTION_TILE_H
#include "BlackholeAttentionTileImage.h"
#include "PioneerRuntimeMatvec.h"

// Dedicated qualification session, not the inference/KV-cache ABI.
// No supplied code, device address, shape, coordinate or user pointer.
enum {
	TT_ATTN_TILE_DESC=2049, TT_ATTN_TILE_STACK=2053, TT_ATTN_TILE_QUERY=4101,
	TT_ATTN_TILE_KEY=6149, TT_ATTN_TILE_VALUE=22533, TT_ATTN_TILE_OUTPUT=38917,
	TT_ATTN_TILE_SAVED_WORDS=40965, TT_ATTN_TILE_MAX_TOKENS=256,
	TT_ATTN_TILE_REFUSED=0, TT_ATTN_TILE_PASSED=1, TT_ATTN_TILE_CONFIG_FAILED=2,
	TT_ATTN_TILE_STAGE_FAILED=3, TT_ATTN_TILE_START_FAILED=4,
	TT_ATTN_TILE_COMPLETION_FAILED=5, TT_ATTN_TILE_NONFINITE=6, TT_ATTN_TILE_COLD=7
};
static inline uint32_t PioneerAttentionTileWordAddress(unsigned word)
{
	if (!word) return 0x6000;
	if (word<TT_ATTN_TILE_DESC) return 0x72000+(word-1)*4;
	if (word<TT_ATTN_TILE_STACK) return 0x74000+(word-TT_ATTN_TILE_DESC)*4;
	if (word<TT_ATTN_TILE_QUERY) return 0x76000+(word-TT_ATTN_TILE_STACK)*4;
	if (word<TT_ATTN_TILE_KEY) return 0x90000+(word-TT_ATTN_TILE_QUERY)*4;
	if (word<TT_ATTN_TILE_VALUE) return 0x92000+(word-TT_ATTN_TILE_KEY)*4;
	if (word<TT_ATTN_TILE_OUTPUT) return 0xa2000+(word-TT_ATTN_TILE_VALUE)*4;
	if (word<TT_ATTN_TILE_SAVED_WORDS) return 0xb2000+(word-TT_ATTN_TILE_OUTPUT)*4;
	return UINT32_MAX;
}
struct tt_attention_tile_result {
	uint32_t version,state,prepared,started,completed,stopped,recovery_required,context_reset;
	uint32_t tokens,staged_words,polls,phase,count,marker,pc[2],before[4],after[4],nonfinite;
};
struct PioneerAttentionTileLease {
	bool saved=false,code_dirty=false,data_dirty=false,ready=false,running=false,unsafe=false;
};
static inline bool PioneerAttentionTileInputValid(unsigned tokens,const uint32_t* query,
	const uint32_t* key,const uint32_t* value)
{
	if (!tokens || tokens>256 || !query || !key || !value) return false;
	for (unsigned w=0;w<256;w++) if ((query[w]&0x7fffffff)>0x477fe000) return false;
	for (unsigned w=0;w<tokens*64;w++) {
		if ((key[w]&0x7c00)==0x7c00 || (key[w]&0x7c000000)==0x7c000000
			|| (value[w]&0x7c00)==0x7c00 || (value[w]&0x7c000000)==0x7c000000) return false;
	}
	return true;
}
template<class IO>
bool PioneerAttentionTileHeld(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& noc,const PioneerResidentLease& store)
{
	return store.ready && !store.pending && !store.recovery_required && io.ResetRead()==0x47800
		&& PioneerRuntimeSources(io,baseline,noc,store) && BlackholeResidentIdle(io);
}
template<class IO>
bool PioneerAttentionTilePrepare(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& noc,const PioneerResidentLease& store,PioneerAttentionTileLease& compute)
{
	if (compute.ready) return !compute.unsafe && !compute.running;
	uint32_t* saved=io.AttentionSavedWords();
	if (!saved || compute.saved || !PioneerAttentionTileHeld(io,baseline,noc,store)) return false;
	for (unsigned w=0;w<3;w++) {
		uint32_t value=io.RuntimeControlRead(w);
		if ((w==0 && (value&1)) || (w!=0 && value)) return false;
	}
	for (unsigned w=0;w<TT_ATTN_TILE_SAVED_WORDS;w++) saved[w]=io.AttentionReadWord(w);
	compute.saved=true;
	if (!PioneerAttentionTileHeld(io,baseline,noc,store)) { compute.unsafe=true; return false; }
	compute.code_dirty=true;
	for (unsigned w=0;w<TT_ATTENTION_TILE_CODE_WORDS;w++) io.AttentionWriteWord(w+1,BlackholeAttentionTileCodeWord(w));
	io.AttentionWriteWord(0,TT_ATTENTION_TILE_BOOT);
	bool valid=io.AttentionReadWord(0)==TT_ATTENTION_TILE_BOOT;
	for (unsigned w=0;w<TT_ATTENTION_TILE_CODE_WORDS;w++) valid &= io.AttentionReadWord(w+1)==BlackholeAttentionTileCodeWord(w);
	compute.ready=valid; return valid;
}
template<class IO>
bool PioneerAttentionTileClose(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& noc,PioneerResidentLease& store,PioneerAttentionTileLease& compute)
{
	if (!compute.saved) return !compute.unsafe;
	bool safe=!compute.unsafe && !compute.running && io.AttentionSavedWords()
		&& PioneerAttentionTileHeld(io,baseline,noc,store);
	const uint32_t* saved=io.AttentionSavedWords();
	if (safe && compute.code_dirty) for (unsigned attempt=0;attempt<2 && compute.code_dirty;attempt++) {
		for (unsigned w=1;w<TT_ATTN_TILE_DESC;w++) io.AttentionWriteWord(w,saved[w]);
		io.AttentionWriteWord(0,saved[0]);
		bool restored=true;
		for (unsigned w=0;w<TT_ATTN_TILE_DESC;w++) restored &= io.AttentionReadWord(w)==saved[w];
		compute.code_dirty=!restored;
	}
	if (safe && !compute.code_dirty && compute.data_dirty) for (unsigned attempt=0;attempt<2 && compute.data_dirty;attempt++) {
		for (unsigned w=TT_ATTN_TILE_DESC;w<TT_ATTN_TILE_SAVED_WORDS;w++) io.AttentionWriteWord(w,saved[w]);
		bool restored=true;
		for (unsigned w=TT_ATTN_TILE_DESC;w<TT_ATTN_TILE_SAVED_WORDS;w++) restored &= io.AttentionReadWord(w)==saved[w];
		compute.data_dirty=!restored;
	}
	compute.ready=false;
	compute.unsafe |= !safe || compute.code_dirty || compute.data_dirty;
	if (compute.unsafe) store.recovery_required=true;
	return !compute.unsafe;
}
template<class IO>
bool PioneerAttentionTileStage(IO& io,unsigned first,unsigned words,const uint32_t* input)
{
	for (unsigned w=0;w<words;w++) io.AttentionWriteWord(first+w,input[w]);
	bool valid=true;
	for (unsigned w=0;w<words;w++) valid &= io.AttentionReadWord(first+w)==input[w];
	return valid;
}
template<class IO>
bool PioneerAttentionTileRun(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& noc,PioneerResidentLease& store,PioneerAttentionTileLease& compute,
	unsigned tokens,const uint32_t* query,const uint32_t* key,const uint32_t* value,uint32_t* output,tt_attention_tile_result& r)
{
	r={}; r.version=1;
	if (!output || !PioneerAttentionTileInputValid(tokens,query,key,value) || !compute.ready
		|| compute.unsafe || compute.running || !PioneerAttentionTileHeld(io,baseline,noc,store)) return false;
	r.tokens=tokens; r.prepared=1; r.state=TT_ATTN_TILE_STAGE_FAILED;
	compute.data_dirty=true; // includes the stack used by this fixed compiled image
	if (!PioneerAttentionTileStage(io,TT_ATTN_TILE_QUERY,256,query)
		|| !PioneerAttentionTileStage(io,TT_ATTN_TILE_KEY,tokens*64,key)
		|| !PioneerAttentionTileStage(io,TT_ATTN_TILE_VALUE,tokens*64,value)) return false;
	r.staged_words=256+tokens*128;
	for (unsigned w=0;w<256;w++) io.AttentionWriteWord(TT_ATTN_TILE_OUTPUT+w,0x7fc00000);
	for (unsigned w=0;w<256;w++) if (io.AttentionReadWord(TT_ATTN_TILE_OUTPUT+w)!=0x7fc00000) return false;
	{
		const uint32_t descriptor[]={0x314e5441,tokens,1,0};
		if (!PioneerAttentionTileStage(io,TT_ATTN_TILE_DESC,4,descriptor)) return false;
	}
	r.state=TT_ATTN_TILE_CONFIG_FAILED;
	store.l1_dirty=true; store.l1_restored=false;
	for (unsigned w=0;w<3;w++) io.RuntimeStatusWrite(w,0);
	for (unsigned w=0;w<3;w++) if (io.RuntimeStatusRead(w)) return false;
	if (io.AttentionReadWord(0)!=TT_ATTENTION_TILE_BOOT || !PioneerAttentionTileHeld(io,baseline,noc,store)) return false;
	r.state=TT_ATTN_TILE_START_FAILED; r.context_reset=1;
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerRuntimeSources(io,baseline,noc,store)
		|| !BlackholeResidentIdle(io)) goto finished;
	for (unsigned w=0;w<4;w++) r.before[w]=io.ResidentNocRead(0x208+w*4);
	r.started=1; compute.running=true; r.state=TT_ATTN_TILE_COMPLETION_FAILED;
	io.RuntimeResetWrite(0x46800);
	for (unsigned attempt=0;attempt<50000;attempt++) {
		r.polls++;
		if (io.ResetRead()!=0x46800) break;
		r.phase=io.RuntimeStatusRead(0); r.count=io.RuntimeStatusRead(1); r.marker=io.RuntimeStatusRead(2);
		if (r.marker==0x464e5441) break;
		if (r.phase==TT_ATTENTION_TILE_DONE && r.count==tokens && r.marker==TT_ATTENTION_TILE_MARKER) {
			io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
			bool exact=BlackholeResidentIdle(io);
			for (unsigned w=0;w<4;w++) { r.after[w]=io.ResidentNocRead(0x208+w*4); exact &= r.after[w]==r.before[w]; }
			if (exact && r.pc[0]==TT_ATTENTION_TILE_PARK && r.pc[1]==r.pc[0]) { r.completed=1; break; }
		}
		io.Pause();
	}
	if (!r.completed || io.ResetRead()!=0x46800 || !PioneerRuntimeSources(io,baseline,noc,store)
		|| !BlackholeResidentIdle(io)) goto finished;
	for (unsigned w=0;w<256;w++) {
		output[w]=io.AttentionReadWord(TT_ATTN_TILE_OUTPUT+w);
		r.nonfinite+=(output[w]&0x7f800000)==0x7f800000;
	}
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) goto finished;
	io.RuntimeResetWrite(0x47800); io.Pause();
	r.stopped=io.ResetRead()==0x47800 && io.ResetRead()==0x47800;
	if (r.stopped) compute.running=false;
	r.state=!r.stopped?TT_ATTN_TILE_COLD:r.nonfinite?TT_ATTN_TILE_NONFINITE:TT_ATTN_TILE_PASSED;
finished:
	if (!r.started && r.context_reset && io.ResetRead()==0x87ffff && io.ResetRead()==0x87ffff) {
		io.RuntimeResetWrite(0x47800); io.Pause();
	}
	if (store.pending || store.recovery_required || (r.started && (!r.completed || !r.stopped))
		|| (!r.started && io.ResetRead()!=0x47800)) {
		compute.unsafe=true; store.recovery_required=true; r.recovery_required=1; io.QuarantineHostPage();
	}
	return r.state==TT_ATTN_TILE_PASSED && !r.recovery_required;
}
#endif
