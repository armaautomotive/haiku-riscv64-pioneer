/* SPDX-License-Identifier: MIT */
#define PIONEER_ATTENTION_WORKER_MACHINE_ONLY
#include "pioneer_attention_worker_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderDescriptorTable.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTableWorkerImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTableAttentionWorkerImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTableGluWorkerImage.h"
struct TableWorker:AttentionWorker {
	unsigned corruptCopy=0; bool corrupted=false;
	TableWorker(const std::vector<uint8_t>& code,unsigned worker):AttentionWorker(code,worker) {
		if (getenv("PIONEER_COORDINATED_ACCUM_MATH")) Accum(); else Phase(); l1[Index(TT_DECODER_TABLE_COUNT)]=3;
		for (unsigned i=0;i<3;i++) Put(i,false,false);
	}
	void Put(unsigned index,bool attention,bool glu) {
		const uint32_t descriptor[]={base,inputs/16,outputs/16,1,PioneerDecoderDescriptorXY(worker),0,0,attention?0x31475441U:glu?0x314c4741U:0};
		for (unsigned i=0;i<8;i++) buffers[(TT_DECODER_TABLE_BASE-0x80000)/4+index*8+i]=descriptor[i];
	}
	bool CodeAddress(uint32_t a) const override { return (a>=0x71700 && a<0x71910) || AttentionWorker::CodeAddress(a); }
	void Store(uint32_t a,uint32_t value) override {
		if (a==TT_DECODER_TABLE_CURSOR) { l1[Index(a)]=value; return; }
		if (corruptCopy && a==0x70500+4*(corruptCopy-1) && !corrupted) { value^=1; corrupted=true; }
		if (a>=0x70500 && a<0x70520) { assert(pc>=0x71700 && pc<0x71910 && !pending && !inputPending); l1[Index(a)]=value; return; }
		AttentionWorker::Store(a,value);
	}
	unsigned Cursor() { return l1[Index(TT_DECODER_TABLE_CURSOR)]; }
};
int main(int argc,char** argv)
{
	assert(argc==4); const auto plain=Read(argv[1]),attention=Read(argv[2]),glu=Read(argv[3]);
	assert(plain.size()==TT_TABLE_WORKER_IMAGE_WORDS*4 && attention.size()==TT_TABLE_ATTENTION_WORKER_IMAGE_WORDS*4 && glu.size()==TT_TABLE_GLU_WORKER_IMAGE_WORDS*4);
	for (unsigned i=0;i<1024;i++) {
		assert(BlackholeTableWorkerCodeWord(i)==(i<plain.size()/4?Word(plain,i*4):0));
		assert(BlackholeTableAttentionWorkerCodeWord(i)==(i<attention.size()/4?Word(attention,i*4):0));
		assert(BlackholeTableGluWorkerCodeWord(i)==(i<glu.size()/4?Word(glu,i*4):0));
	}
	for (bool smallRows:{false,true}) for (unsigned worker=1;worker<=119;worker++) {
		TableWorker m(worker<=6?glu:worker<=8?attention:plain,worker);
		if (smallRows) { m.outputs=16; m.weights.resize(size_t(m.inputs)*16*9/8/4); m.base+=16*(m.inputs+m.inputs/8);
			for (unsigned i=0;i<3;i++) m.Put(i,false,false); }
		m.BootCoord();
		for (unsigned trial:{0U,9U,1U,2U}) { m.JobCoord(trial); assert(m.Cursor()==m.jobs%3); }
		for (unsigned i=0;i<24;i++) { unsigned slot=i/8,word=i%8;
			const unsigned expected[]={m.base,m.inputs/16,m.outputs/16,1,PioneerDecoderDescriptorXY(worker),0,0,0};
			assert(m.buffers[(TT_DECODER_TABLE_BASE-0x80000)/4+slot*8+word]==expected[word]); }
		m.StopCoord();
	}
	for (unsigned worker=1;worker<=8;worker++) {
		TableWorker m(worker<=6?glu:attention,worker); m.l1[m.Index(TT_DECODER_TABLE_COUNT)]=worker<=6?4:3;
		m.Put(1,true,false); if (worker<=6) { m.Put(2,false,true); m.Put(3,false,false); } else m.Put(2,false,false);
		m.BootCoord(); m.JobCoord(0); m.Enter(); m.CompleteStub(17);
		if (worker<=6) { m.Enter(true); m.CompleteStub(512,true); }
		m.JobCoord(9); assert(!m.Cursor()); m.StopCoord();
	}
	for (unsigned word=1;word<=8;word++) { TableWorker m(glu,1); m.corruptCopy=word; m.BootCoord();
		bool refused=false; try { m.JobCoord(0); } catch (uint32_t marker) { refused=marker==0x4e4f4346; }
		assert(refused && m.corrupted && !m.matrixInstructions && !m.requests && !m.l1[m.Index(0x71304)]); }
	for (unsigned fault=0;fault<4;fault++) { TableWorker m(plain,119); m.BootCoord();
		m.l1[m.Index(TT_DECODER_TABLE_COUNT)]=fault==0?0:fault==1?226:3;
		m.l1[m.Index(TT_DECODER_TABLE_CURSOR)]=fault==2?3:fault==3?UINT32_MAX:0;
		bool refused=false; try { m.JobCoord(0); } catch (uint32_t marker) { refused=marker==0x4e4f4346; }
		assert(refused && !m.matrixInstructions && !m.requests && !m.l1[m.Index(0x71304)]); }
	{ TableWorker m(plain,119); m.BootCoord(); m.l1[m.Index(TT_DECODER_TABLE_COUNT)]=0; m.StopCoord(); }
	puts("PASS OFFLINE actual static-descriptor copy/ALL-bit check,all119 peers/exact phase math/repeated cursor wrap,mixed attention/GLU,STOP bypass,all copy/count/index faults before compute;NOT native qualification");
}
