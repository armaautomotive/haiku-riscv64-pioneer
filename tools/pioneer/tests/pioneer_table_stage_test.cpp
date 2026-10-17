/* SPDX-License-Identifier: MIT */
#define PIONEER_FLEET_POLICY_MOCK_ONLY
#include "pioneer_runtime_fleet_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimePersistentFleet.h"
struct TableStageIO:FleetIO {
	unsigned dropWord=UINT32_MAX,badRead=UINT32_MAX,badMeta=UINT32_MAX;
	bool codeFaultArmed=false; unsigned dropCode=UINT32_MAX,badCode=UINT32_MAX;
	uint32_t codeFaultMask=1;
	TableStageIO(unsigned n):FleetIO(n) {}
	void RuntimeCodeWrite(unsigned w,uint32_t value) {
		if (!codeFaultArmed || w!=dropCode) FleetIO::RuntimeCodeWrite(w,value);
	}
	uint32_t RuntimeCodeRead(unsigned w) {
		return FleetIO::RuntimeCodeRead(w)^((codeFaultArmed && w==badCode)?codeFaultMask:0);
	}
	unsigned RuntimeBufferWords() { return C().RuntimeBufferWords(); }
	void RuntimeBufferWrite(unsigned w,uint32_t value) {
		assert(w>=6144 && w<6144+TT_DECODER_TABLE_WORDS && C().reset==0x47800 && !C().starts);
		if (w!=dropWord) C().RuntimeBufferWrite(w,value);
	}
	uint32_t RuntimeBufferRead(unsigned w) {
		assert(w>=6144 && w<6144+TT_DECODER_TABLE_WORDS && C().reset==0x47800 && !C().starts);
		return C().RuntimeBufferRead(w)^(w==badRead?1:0);
	}
	uint32_t FleetCodeRead(unsigned w) { return FleetIO::FleetCodeRead(w)^(w==badMeta?1:0); }
};
static PioneerDecoderJob Model() {
	PioneerDecoderJob model={}; model.vocabulary=0x40000000;
	for (auto& l:model.layers) for (unsigned role=0;role<7;role++) reinterpret_cast<uint32_t*>(&l)[role]=0x04000000+role*0x1000000;
	return model;
}
static void Prepare(TableStageIO& io,PioneerFleetWorker& f,unsigned worker,bool accum=false,bool gather=false,bool rounded=false,bool gate=false) {
	tt_host_page_result host={}; auto& c=io.C(); assert(Prepared(c,host,f.Store())); f.noc=c.Compute(); f.noc.version=PioneerWorkerVersion(worker);
	assert(PioneerRuntimeMatvecPrepare(c,c.Baseline(),f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true));
	assert(PioneerFleetPreludePrepare(io,f));
	assert(!PioneerPersistentPrepare(io,f,true,true,worker<=8,worker<=6,true,false,true,false,true));
	assert(PioneerPersistentPrepare(io,f,true,true,worker<=8,worker<=6,true,false,true,true,accum,gather,rounded,gate));
	assert(f.accumPipeline==accum && f.fusedGather==gather && f.roundedProduct==rounded && f.unpackWaitGate==gate);
	if (gate) {
		for (unsigned w=0;w<1024;w++) assert(c.code[w+1]==(w==(TT_RUNTIME_Q8_PARK_PC-0x70000)/4?TT_PERSISTENT_MATH_RETURN:BlackholeRuntimeQ8UnpackGateCodeWord(w)));
		assert(!PioneerPersistentPrepare(io,f,true,true,worker<=8,worker<=6,true,false,true,true,accum,gather,false,false));
	}
	if (rounded) {
		for (unsigned w=0;w<1024;w++) assert(c.code[w+1]==(w==(TT_RUNTIME_Q8_PARK_PC-0x70000)/4?TT_PERSISTENT_MATH_RETURN:BlackholeRuntimeQ8RoundedCodeWord(w)));
		assert(!PioneerPersistentPrepare(io,f,true,true,worker<=8,worker<=6,true,false,true,true,accum,gather,false));
	}
	if (gather) assert(!PioneerPersistentPrepare(io,f,true,true,worker<=8,worker<=6,true,false,true,true,accum,false));
	assert(f.descriptorTable && !f.descriptorTablePrepared && !PioneerPersistentBoot(io,f) && !c.starts);
}
int main() {
	auto model=Model();
	// The ONE changed math word must be staged/read back before ANY launch.
	// A held-only staging failure is restorable; an executing UNKNOWN is not.
	for (unsigned fault=0;fault<=32;fault++) {
		TableStageIO io(2); io.current=1; PioneerFleetWorker f;
		tt_host_page_result host={}; auto& c=io.C(); assert(Prepared(c,host,f.Store())); f.noc=c.Compute(); f.noc.version=PioneerWorkerVersion(1);
		assert(PioneerRuntimeMatvecPrepare(c,c.Baseline(),f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true));
		assert(PioneerFleetPreludePrepare(io,f));
		io.codeFaultArmed=true;
		if (!fault) io.dropCode=1+0x370/4;
		else { io.badCode=1+0x370/4; io.codeFaultMask=uint32_t(1)<<(fault-1); }
		assert(!PioneerPersistentPrepare(io,f,true,true,true,true,true,false,true,true,true,true,false,true));
		assert(!f.persistentPrepared && !c.starts && !f.Compute().running);
		io.codeFaultArmed=false;
		assert(PioneerFleetPreludeClose(io,f,true));
		assert(PioneerRuntimeMatvecClose(c,c.Baseline(),f.noc,f.Store(),f.Compute()));
		assert(PioneerResidentLeaseEnd(c,f.noc,f.Store())); c.Restored();
	}
	for (unsigned mode:{0U,1U,2U,3U,4U,5U}) { bool accum=mode>=1,smallRows=mode==2,gather=mode>=3,rounded=mode==4,gate=mode==5; TableStageIO io(120); std::vector<PioneerFleetWorker> fleet(120);
		for (unsigned worker=1;worker<120;worker++) { io.current=worker; auto& f=fleet[worker]; Prepare(io,f,worker,accum,gather,rounded,gate);
			std::vector<uint32_t> before(io.C().buffers,io.C().buffers+12288);
			assert(PioneerPersistentTablePrepare(io,io.C().Baseline(),f,model,worker,smallRows) && f.smallRows==smallRows && f.descriptorTablePrepared && f.Compute().buffers_dirty && !io.C().starts);
			unsigned count=PioneerDecoderDescriptorCount(worker,smallRows);
			for (unsigned i=0;i<count;i++) { PioneerDecoderDescriptor entry; assert(PioneerDecoderDescriptorEntry(model,worker,i,entry,smallRows));
				for (unsigned w=0;w<8;w++) assert(io.C().buffers[6144+i*8+w]==entry.words[w]); }
			for (unsigned w=0;w<12288;w++) if (w<6144 || w>=6144+count*8) assert(io.C().buffers[w]==before[w]);
			assert(!PioneerPersistentTablePrepare(io,io.C().Baseline(),f,model,worker));
			// No RISC was launched. All touched table words are part of the
			// existing bulk backup and must restore exactly with the prelude.
			assert(PioneerFleetPreludeClose(io,f,true));
			assert(PioneerRuntimeMatvecClose(io.C(),io.C().Baseline(),f.noc,f.Store(),f.Compute()));
			for (unsigned w=0;w<12288;w++) assert(io.C().buffers[w]==0xabc00000+w);
			assert(PioneerResidentLeaseEnd(io.C(),f.noc,f.Store())); io.C().Restored();
		}
	}
	for (unsigned word:{6144U,6145U,6146U,6147U,6148U,6149U,6150U,6151U,7943U}) for (bool read:{false,true}) {
		TableStageIO io(2); io.current=1; PioneerFleetWorker f; Prepare(io,f,1);
		if (read) io.badRead=word; else io.dropWord=word;
		assert(!PioneerPersistentTablePrepare(io,io.C().Baseline(),f,model,1) && !f.descriptorTablePrepared && !io.C().starts && f.Compute().buffers_dirty);
	}
	for (unsigned word:{240U,241U}) { TableStageIO io(2); io.current=1; PioneerFleetWorker f; Prepare(io,f,1); io.badMeta=word;
		assert(!PioneerPersistentTablePrepare(io,io.C().Baseline(),f,model,1) && !f.descriptorTablePrepared && !io.C().starts); }
	puts("PASS OFFLINE held-only immutable table staging/ALL-word readback,all119 peers,all surrounding buffers unchanged,repeated staging refused,copy/read/metadata faults refuse boot;NO hardware");
}
