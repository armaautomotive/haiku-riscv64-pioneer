/* SPDX-License-Identifier: MIT */
#define PIONEER_ATTENTION_MOCK_ONLY
#include "pioneer_attention_tile_policy_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerResidentAttention.h"
struct ResidentAttentionMock:AttentionMock {
	std::vector<uint32_t> bulkWords=std::vector<uint32_t>(8192,0xabcdef00);
	uint32_t nc[8]={100,200,300,400,500,600,300,800};
	unsigned bulkWrites=0,dropBulk=0;
	bool map=true;
	bool BulkMappingStable() { return map; }
	uint32_t BulkL1Read(unsigned w) { assert(w<8192); return bulkWords[w]; }
	void BulkL1Write(unsigned w,uint32_t v) {
		assert(w<8192 && reset==0x47800 && !quarantined);
		if (++bulkWrites!=dropBulk) bulkWords[w]=v;
	}
	struct Payload {
		ResidentAttentionMock& io;
		void PayloadFence() {}
		uint32_t PayloadRead(unsigned w) { return io.BulkL1Read(w); }
		void PayloadWrite(unsigned w,uint32_t value) { io.BulkL1Write(w,value); }
	};
	bool BulkPayloadStage(const uint32_t* input,unsigned words) { Payload data={*this}; return PioneerPayloadStageBlock(data,words,3072,input); }
	bool BulkPayloadRead(unsigned first,unsigned words,uint32_t* output) { Payload data={*this}; return PioneerPayloadReadBlock(data,first,words,8192,output); }
	uint32_t ResidentNocRead(unsigned offset) {
		const unsigned offsets[8]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
		for (unsigned i=0;i<8;i++) if (offset==offsets[i]) return nc[i];
		return AttentionMock::ResidentNocRead(offset);
	}
	void RuntimeResetWrite(uint32_t value) {
		assert(!quarantined && (value==0x87ffff || value==0x46800 || value==0x47800));
		if (reset==0x46800 && value==0x87ffff) { assert(!timeout && !badPc && !badCount && !badCounters); stops++; }
		if (++Mock::writes==Mock::drop || (stopFails && starts && value==0x47800)) return;
		reset=value;
		if (value!=0x46800) return;
		starts++; if (timeout) return;
		assert(words[0]==TT_ATTENTION_TILE_BOOT && words[TT_ATTN_TILE_DESC]==0x3152564b);
		unsigned tokens=words[TT_ATTN_TILE_DESC+1],reads=16*((tokens+63)/64);
		const unsigned delta[8]={reads,64*tokens,reads+16,reads,16,64,reads+16,16};
		for (unsigned w=0;w<8;w++) nc[w]+=delta[w];
		if (badCounters) nc[7]++;
		status[0]=TT_ATTENTION_TILE_DONE; status[1]=tokens+(badCount?1:0); status[2]=TT_ATTENTION_TILE_MARKER;
		for (unsigned w=0;w<2048;w++) bulkWords[4096+w]=nonfinite?0x7fc00000:0x40000000;
	}
};
static bool Run(ResidentAttentionMock& io,PioneerResidentLease& store,PioneerAttentionTileLease& compute,
	PioneerResidentAttentionLease& kv,tt_resident_attention_result& result,unsigned n=1,unsigned layer=0)
{
	PioneerBulkUploadLease bulk; bulk.ready=bulk.saved=true;
	uint32_t input[3072],output[2048];
	for (unsigned w=0;w<3072;w++) input[w]=w<2048?0x3f800000:0x3c003c00;
	for (auto& w:output) w=0xabcdef00;
	bool ready=PioneerAttentionTilePrepare(io,io.Baseline(),io.Compute(),store,compute,true);
	bool passed=ready && PioneerResidentAttentionRun(io,io.Baseline(),io.Compute(),store,compute,
		bulk,kv,0x80000000,n,layer,input,output,result);
	for (auto w:output) assert(w==(passed?0x40000000:0xabcdef00));
	return passed;
}
int main()
{
	for (bool reserved:{false,true}) for (bool blocks:{false,true}) for (unsigned version=0;version<4;version++) {
		PioneerResidentAttentionLease kv; kv.reserved=reserved; kv.payloadBlocks=blocks;
		assert(PioneerResidentAttentionVersion(version,kv)==((version==1 || version==2) && (!reserved || blocks==(version==2))));
	}
	for (bool reserved:{false,true}) for (bool blocks:{false,true}) for (bool coordinated:{false,true}) for (unsigned version=0;version<5;version++) {
		PioneerResidentAttentionLease kv; kv.reserved=reserved; kv.payloadBlocks=blocks; kv.coordinated=coordinated;
		assert(PioneerResidentAttentionVersion(version,kv,true)==(version==3 && (!reserved || (blocks && coordinated))));
		assert(PioneerResidentAttentionVersion(version,kv,false)==((version==1 || version==2) && (!reserved || (!coordinated && blocks==(version==2)))));
	}
	for (unsigned n=1;n<=256;n++) {
		uint32_t a[8]={},b[8]; unsigned reads=16*((n+63)/64);
		const unsigned expected[8]={reads,64*n,reads+16,reads,16,64,reads+16,16};
		for (unsigned w=0;w<8;w++) { a[w]=UINT32_MAX-7; b[w]=a[w]+expected[w]; }
		assert(PioneerResidentAttentionCounterProof(n,a,b));
		for (unsigned w=0;w<8;w++) { b[w]++; assert(!PioneerResidentAttentionCounterProof(n,a,b)); b[w]--; }
	}
	{
		ResidentAttentionMock io; tt_host_page_result host; PioneerResidentLease store; PioneerAttentionTileLease compute;
		PioneerResidentAttentionLease kv; kv.reserved=true; tt_resident_attention_result result;
		assert(Prepared(io,host,store));
		assert(Run(io,store,compute,kv,result) && result.groups==8 && kv.tokens[0]==1);
		assert(!Run(io,store,compute,kv,result,3) && io.starts==1);
		assert(Run(io,store,compute,kv,result,2) && kv.tokens[0]==2);
		assert(Run(io,store,compute,kv,result,1,27) && kv.tokens[27]==1);
		assert(!PioneerAttentionTilePrepare(io,io.Baseline(),io.Compute(),store,compute,false));
		assert(PioneerAttentionTileClose(io,io.Baseline(),io.Compute(),store,compute)); io.Restored();
		assert(PioneerResidentLeaseEnd(io,io.Compute(),store)); assert(!io.ReleaseHostPage()); io.Safe();
	}
	for (unsigned fault=0;fault<9;fault++) {
		ResidentAttentionMock io; tt_host_page_result host; PioneerResidentLease store; PioneerAttentionTileLease compute;
		PioneerResidentAttentionLease kv; kv.reserved=true; tt_resident_attention_result result;
		assert(Prepared(io,host,store));
		io.timeout=fault==0; io.badPc=fault==1; io.badCount=fault==2; io.badCounters=fault==3;
		io.stopFails=fault==4; io.badControl=fault==5; io.nonfinite=fault==6; io.badSource=fault==7; io.map=fault!=8;
		assert(!Run(io,store,compute,kv,result) && !kv.tokens[0]);
		if (fault<5) {
			assert(compute.unsafe && store.recovery_required && io.quarantined && result.math.recovery_required);
			unsigned writes=io.Mock::writes;
			assert(!PioneerAttentionTileClose(io,io.Baseline(),io.Compute(),store,compute) && writes==io.Mock::writes);
		} else {
			assert(PioneerAttentionTileClose(io,io.Baseline(),io.Compute(),store,compute));
			if (fault==7) assert(!PioneerResidentLeaseEnd(io,io.Compute(),store));
			else { assert(PioneerResidentLeaseEnd(io,io.Compute(),store)); assert(!io.ReleaseHostPage()); io.Safe(); }
		}
	}
	for (unsigned dropped=1;dropped<=3072;dropped++) {
		ResidentAttentionMock io; tt_host_page_result host; PioneerResidentLease store; PioneerAttentionTileLease compute;
		PioneerResidentAttentionLease kv; kv.reserved=true; tt_resident_attention_result result;
		assert(Prepared(io,host,store)); io.dropBulk=dropped;
		assert(!Run(io,store,compute,kv,result) && !io.starts && !kv.tokens[0] && !store.recovery_required);
		assert(PioneerAttentionTileClose(io,io.Baseline(),io.Compute(),store,compute));
		assert(PioneerResidentLeaseEnd(io,io.Compute(),store)); assert(!io.ReleaseHostPage());
	}
	printf("PASS: resident attention bounds/counter wrap/sequence/mode, all3072 dropped input stores, timeout/PC/count/NoC/stop/nonfinite/source/mapping faults; no output/reset/restore after uncertainty\n");
}
