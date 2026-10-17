/* SPDX-License-Identifier: MIT */
#define PIONEER_DESCRIPTOR_WORKER_MACHINE_ONLY
#include "pioneer_descriptor_worker_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeGatherWorkerImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeGatherAttentionWorkerImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeGatherGluWorkerImage.h"
struct GatherWorker:TableWorker {
	unsigned writes=0,dropPrefix=0;
	GatherWorker(const std::vector<uint8_t>& code,unsigned w):TableWorker(code,w) {
		for (unsigned i=4080;i<4096;i++) source[i]=0xdecafbad;
	}
	bool ActivationUnchanged() const override { return !memcmp(buffers,source,4080*4); }
	uint32_t Load(uint32_t a) override {
		if (a>=0x83fc0 && a<0x84000) { assert(!pending && !inputPending); return buffers[(a-0x80000)/4]; }
		return TableWorker::Load(a);
	}
	void Store(uint32_t a,uint32_t v) override {
		if (a>=0x83fc0 && a<0x84000) {
			assert(pc>=0x717c0 && pc<0x71910 && !pending && !inputPending && l1[Index(0x71304)]+1==jobs);
			writes++; if ((a-0x83fc0)/4+1!=dropPrefix) buffers[(a-0x80000)/4]=v;
			return;
		}
		TableWorker::Store(a,v);
	}
	void VerifyPrefix() {
		assert(writes==16*jobs);
		for (unsigned i=0;i<16;i++) assert(buffers[4080+i]==l1[Index(0x71340+i*4)]);
	}
};
int main(int argc,char** argv)
{
	assert(argc==4); const auto plain=Read(argv[1]),attention=Read(argv[2]),glu=Read(argv[3]);
	assert(plain.size()==TT_GATHER_WORKER_IMAGE_WORDS*4 && attention.size()==TT_GATHER_ATTENTION_WORKER_IMAGE_WORDS*4 && glu.size()==TT_GATHER_GLU_WORKER_IMAGE_WORDS*4);
	for (unsigned i=0;i<1024;i++) {
		assert(BlackholeGatherWorkerCodeWord(i)==(i<plain.size()/4?Word(plain,i*4):0));
		assert(BlackholeGatherAttentionWorkerCodeWord(i)==(i<attention.size()/4?Word(attention,i*4):0));
		assert(BlackholeGatherGluWorkerCodeWord(i)==(i<glu.size()/4?Word(glu,i*4):0));
	}
	setenv("PIONEER_COORDINATED_ACCUM_MATH","1",1);
	for (unsigned w=1;w<=119;w++) {
		GatherWorker m(w<=6?glu:w<=8?attention:plain,w); m.BootCoord();
		for (unsigned trial:{0U,1U,9U}) { m.JobCoord(trial); m.VerifyPrefix(); }
		unsigned stores=m.writes; m.StopCoord(); assert(m.writes==stores);
	}
	for (unsigned w=1;w<=8;w++) {
		GatherWorker m(w<=6?glu:attention,w); m.l1[m.Index(TT_DECODER_TABLE_COUNT)]=w<=6?4:3;
		m.Put(1,true,false); if (w<=6) { m.Put(2,false,true); m.Put(3,false,false); } else m.Put(2,false,false);
		m.BootCoord(); m.JobCoord(0); m.VerifyPrefix(); uint32_t before[16]; memcpy(before,m.buffers+4080,64); unsigned stores=m.writes;
		m.Enter(); m.CompleteStub(17); if (w<=6) { m.Enter(true); m.CompleteStub(512,true); }
		assert(m.writes==stores && !memcmp(before,m.buffers+4080,64)); m.JobCoord(9);
		for (unsigned i=0;i<16;i++) assert(m.buffers[4080+i]==m.l1[m.Index(0x71340+i*4)]);
		m.StopCoord();
	}
	for (unsigned word=1;word<=16;word++) { GatherWorker m(glu,1); m.BootCoord(); m.dropPrefix=word; bool refused=false;
		try { m.JobCoord(0); } catch (uint32_t tag) { refused=tag==0x4e4f4346; }
		assert(refused && !m.l1[m.Index(0x71304)]);
	}
	puts("PASS OFFLINE actual fused-gather worker,all119 peers/repeated exact math/ALL16 proof words BEFORE finished,attention+GLU/STOP do not touch prefix,ALL16 lost stores refuse publication;NOT native/driver");
}
