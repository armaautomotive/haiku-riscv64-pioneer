/* SPDX-License-Identifier: MIT */
// OFFLINE actual unchanged table/gather/math worker bytes with the NEW planned
// first QKV descriptor. Independent local-row fixtures,NOT whole-model proof.
#define PIONEER_GATHER_WORKER_MACHINE_ONLY
#include "pioneer_gather_worker_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQkvDescriptorPlan.h"
int main(int argc,char** argv)
{
	assert(argc==4);
	const auto plain=Read(argv[1]),attention=Read(argv[2]),glu=Read(argv[3]);
	assert(plain.size()==TT_GATHER_WORKER_IMAGE_WORDS*4 && attention.size()==TT_GATHER_ATTENTION_WORKER_IMAGE_WORDS*4 && glu.size()==TT_GATHER_GLU_WORKER_IMAGE_WORDS*4);
	for (unsigned i=0;i<1024;i++) {
		assert(BlackholeGatherWorkerCodeWord(i)==(i<plain.size()/4?Word(plain,i*4):0));
		assert(BlackholeGatherAttentionWorkerCodeWord(i)==(i<attention.size()/4?Word(attention,i*4):0));
		assert(BlackholeGatherGluWorkerCodeWord(i)==(i<glu.size()/4?Word(glu,i*4):0));
	}
	setenv("PIONEER_COORDINATED_ACCUM_MATH","1",1);
	setenv("PIONEER_COORDINATED_UNPACK_GATE_MATH","1",1);
	PioneerDecoderJob model={}; model.layers[0].q=0x04000000; model.layers[0].v=0x05000000; model.layers[0].k=0x06000000;
	for (unsigned worker=1;worker<120;worker++) {
		PioneerPairedQkvSpan span; PioneerDecoderDescriptor expected;
		assert(PioneerPairedQkvWorker(worker,span) && PioneerQkvDescriptorEntry(model,worker,0,expected));
		GatherWorker m(worker<=6?glu:worker<=8?attention:plain,worker);
		m.outputs=span.rows; m.base=expected.words[0];
		m.weights.assign(size_t(m.inputs)*m.outputs*9/8/4,0);
		for (unsigned row=0;row<m.outputs;row++) for (unsigned b=0;b<m.inputs/32;b++) {
			m.weights[PioneerQ8NativeF32ScaleByte(m.inputs,b,row)/4]=Bits(m.Half(m.WeightScale(row,b)));
			for (unsigned i=0;i<32;i++) { uint8_t q=0; assert(PioneerInt8UnpackEncode(m.NativeWeight(row,b*32+i),q));
				uint64_t at=PioneerQ8NativeF32ValueByte(m.inputs,b*32+i,row); m.weights[at/4]|=uint32_t(q)<<(8*(at&3)); }
		}
		for (unsigned index=0;index<3;index++) m.Put(index,false,false);
		m.BootCoord();
		for (unsigned trial:{0U,9U,1U}) {
			m.JobCoord(trial); m.VerifyPrefix(); assert(m.Cursor()==m.jobs%3);
			for (unsigned i=0;i<8;i++) assert(m.l1[m.Index(0x70500+i*4)]==expected.words[i]);
		}
		unsigned writes=m.writes; m.StopCoord(); assert(m.writes==writes);
	}
	puts("PASS OFFLINE actual QKV-planned first descriptors/ALL119 frozen math+gather workers/32+64 rows/three changed-input IEEE exact jobs/table wrap/ALL proof prefix bits/STOP;independent local-row fixtures,NOT full QKV model/driver/native performance");
}
