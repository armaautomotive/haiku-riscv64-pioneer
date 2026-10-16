/* SPDX-License-Identifier: MIT */
#define PIONEER_RESIDENT_STORE_MOCK_ONLY
#include "pioneer_resident_store_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerAttentionTile.h"
#include <vector>

struct AttentionMock:StoreMock {
	std::vector<uint32_t> words,saved;
	uint32_t reset=0x47800,status[3]={0x11111111,0x22222222,0x33333333};
	std::vector<unsigned> controlWrites;
	unsigned starts=0,stops=0,attentionWrites=0;
	bool timeout=false,badPc=false,badCount=false,badCounters=false,stopFails=false;
	bool badControl=false,restoreCodeFails=false,restoreDataFails=false,nonfinite=false,badSource=false;
	AttentionMock():words(TT_ATTN_TILE_SAVED_WORDS),saved(TT_ATTN_TILE_SAVED_WORDS) {
		for (unsigned w=0;w<words.size();w++) words[w]=0xab000000+w;
	}
	uint32_t ResetRead() { return reset; }
	uint32_t ResidentNocRead(uint32_t offset) {
		uint32_t value=StoreMock::ResidentNocRead(offset);
		return badSource && offset==BlackholeDramSourceOffset(1)?value^1:value;
	}
	uint32_t* AttentionSavedWords() { return saved.data(); }
	uint32_t AttentionReadWord(unsigned w) { assert(w<words.size()); return words[w]; }
	void AttentionWriteWord(unsigned w,uint32_t value) {
		assert(w<words.size() && !uncertain && !quarantined && reset==0x47800);
		attentionWrites++;
		if (!w || (w>=TT_ATTN_TILE_DESC && w<TT_ATTN_TILE_STACK)) controlWrites.push_back(Mock::writes+1);
		if (++Mock::writes==Mock::drop) return;
		if ((w<TT_ATTN_TILE_DESC?restoreCodeFails:restoreDataFails) && value==0xab000000+w) return;
		words[w]=value;
	}
	uint32_t RuntimeControlRead(unsigned w) { assert(w<3); return badControl && w==0?1:0; }
	uint32_t RuntimeStatusRead(unsigned w) { assert(w<3); return status[w]; }
	void RuntimeStatusWrite(unsigned w,uint32_t value) {
		assert(w<3 && !quarantined && reset==0x47800);
		controlWrites.push_back(Mock::writes+1);
		if (++Mock::writes!=Mock::drop) status[w]=value;
	}
	uint32_t RuntimePcRead() { return badPc?0x72000:TT_ATTENTION_TILE_PARK; }
	void RuntimeResetWrite(uint32_t value) {
		assert(!quarantined && (value==0x87ffff || value==0x46800 || value==0x47800));
		controlWrites.push_back(Mock::writes+1);
		if (reset==0x46800 && value==0x87ffff) { assert(!timeout && !badPc && !badCount && !badCounters); stops++; }
		if (++Mock::writes==Mock::drop || (stopFails && starts && value==0x47800)) return;
		reset=value;
		if (value!=0x46800) return;
		starts++; if (timeout) return;
		assert(words[0]==TT_ATTENTION_TILE_BOOT && words[TT_ATTN_TILE_DESC]==0x314e5441);
		status[0]=TT_ATTENTION_TILE_DONE; status[1]=words[TT_ATTN_TILE_DESC+1]+(badCount?1:0); status[2]=TT_ATTENTION_TILE_MARKER;
		if (badCounters) counters[1]++;
		for (unsigned w=0;w<256;w++) words[TT_ATTN_TILE_OUTPUT+w]=nonfinite?0x7fc00000:0x40000000;
		// Model arbitrary stack writes inside the fixed image's saved range.
		words[TT_ATTN_TILE_STACK+100]=0x12345678;
	}
	void Restored() {
		assert(reset==0x47800);
		for (unsigned w=0;w<words.size();w++) assert(words[w]==0xab000000+w);
	}
};
static bool RunAttention(AttentionMock& io,PioneerResidentLease& store,PioneerAttentionTileLease& compute,tt_attention_tile_result& r)
{
	uint32_t query[256],key[128],value[128],output[256]={};
	for (auto& w:query) w=0x3f800000;
	for (auto& w:key) w=0x3c003c00;
	for (auto& w:value) w=0x40004000;
	bool ready=PioneerAttentionTilePrepare(io,io.Baseline(),io.Compute(),store,compute);
	bool passed=ready && PioneerAttentionTileRun(io,io.Baseline(),io.Compute(),store,compute,2,query,key,value,output,r);
	if (passed) for (auto w:output) assert(w==0x40000000);
	return passed;
}
static void Clean(AttentionMock& io,PioneerResidentLease& store,PioneerAttentionTileLease& compute)
{
	assert(PioneerAttentionTileClose(io,io.Baseline(),io.Compute(),store,compute)); io.Restored();
	assert(PioneerResidentLeaseEnd(io,io.Compute(),store) && io.ReleaseHostPage()==0); io.Safe();
}
int main()
{
	// Flattening is injective, bounded, and excludes all unused L1 gaps.
	std::vector<bool> used(0x180000/4);
	for (unsigned w=0;w<TT_ATTN_TILE_SAVED_WORDS;w++) {
		uint32_t a=PioneerAttentionTileWordAddress(w); assert(!(a&3) && a<0x180000 && !used[a/4]); used[a/4]=true;
		assert(a==0x6000 || (a>=0x72000 && a<0x74000) || (a>=0x74000 && a<0x74010)
			|| (a>=0x76000 && a<0x78000) || (a>=0x90000 && a<0xb4000));
	}
	assert(PioneerAttentionTileWordAddress(TT_ATTN_TILE_SAVED_WORDS)==UINT32_MAX);
	AttentionMock io; tt_host_page_result host; PioneerResidentLease store; PioneerAttentionTileLease compute; tt_attention_tile_result r={};
	assert(Prepared(io,host,store)); unsigned before=io.Mock::writes;
	assert(RunAttention(io,store,compute,r));
	std::vector<unsigned> drops={1,2,100,2048,2050,2305,2306,2433,2434,2561,2562,2817};
	for (unsigned w:io.controlWrites) drops.push_back(w-before);
	assert(r.started && r.completed && r.stopped && !r.recovery_required && r.staged_words==512 && io.starts==1 && io.stops==1);
	assert(RunAttention(io,store,compute,r) && io.starts==2);
	Clean(io,store,compute);
	for (unsigned fault=0;fault<8;fault++) {
		AttentionMock m; tt_host_page_result h; PioneerResidentLease s; PioneerAttentionTileLease c;
		assert(Prepared(m,h,s));
		m.timeout=fault==0; m.badPc=fault==1; m.badCount=fault==2; m.badCounters=fault==3; m.stopFails=fault==4;
		m.badControl=fault==5; m.nonfinite=fault==6; m.badSource=fault==7;
		r={}; bool passed=RunAttention(m,s,c,r);
		if (passed) fprintf(stderr,"fault %u unexpectedly passed\n",fault);
		assert(!passed);
		if (fault<5) {
			assert(r.recovery_required && s.recovery_required && c.unsafe && m.quarantined && !m.releases);
			unsigned writes=m.Mock::writes;
			assert(!PioneerAttentionTileClose(m,m.Baseline(),m.Compute(),s,c) && writes==m.Mock::writes);
			if (fault<4) assert(!m.stops);
		} else if (fault==6) { assert(r.nonfinite==256 && r.stopped && !r.recovery_required); Clean(m,s,c); }
		else if (fault==5) { assert(!r.started); Clean(m,s,c); }
		else assert(!r.started && !c.saved && !m.attentionWrites);
	}
	for (unsigned fault=0;fault<2;fault++) {
		AttentionMock m; tt_host_page_result h; PioneerResidentLease s; PioneerAttentionTileLease c;
		assert(Prepared(m,h,s) && RunAttention(m,s,c,r));
		m.restoreCodeFails=fault==0; m.restoreDataFails=fault==1;
		assert(!PioneerAttentionTileClose(m,m.Baseline(),m.Compute(),s,c) && c.unsafe && s.recovery_required);
	}
	// Sample payload writes in each immutable code/input/output range; drop
	// EVERY descriptor/status/reset/launch/stop write. All payloads have full
	// readback checks, independent of this control-flow fault sampling.
	unsigned tested=0;
	for (unsigned drop:drops) {
		AttentionMock m; tt_host_page_result h; PioneerResidentLease s; PioneerAttentionTileLease c;
		assert(Prepared(m,h,s)); m.Mock::drop=m.Mock::writes+drop;
		r={}; bool passed=RunAttention(m,s,c,r); tested++;
		if (r.recovery_required) {
			assert(!passed && m.quarantined && !m.releases);
			unsigned writes=m.Mock::writes;
			assert(!PioneerAttentionTileClose(m,m.Baseline(),m.Compute(),s,c) && writes==m.Mock::writes);
		} else Clean(m,s,c);
	}
	uint32_t q[256]={},k[64]={},v[64]={};
	assert(PioneerAttentionTileInputValid(1,q,k,v));
	assert(!PioneerAttentionTileInputValid(0,q,k,v) && !PioneerAttentionTileInputValid(257,q,k,v));
	assert(!PioneerAttentionTileInputValid(1,NULL,k,v));
	q[255]=0x7f800000; assert(!PioneerAttentionTileInputValid(1,q,k,v)); q[255]=0;
	k[63]=0x7c000000; assert(!PioneerAttentionTileInputValid(1,q,k,v)); k[63]=0;
	v[63]=0x7c00; assert(!PioneerAttentionTileInputValid(1,q,k,v));
	printf("PASS: OFFLINE dedicated attention lease; full L1/code/stack restoration, no-NoC completion/PC proofs, immutable bounds; %u dropped writes, launch/timeout/stop/restore/nonfinite faults; no reset/restore after uncertainty\n",tested);
}
