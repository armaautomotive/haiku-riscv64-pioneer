/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQwen38CohortHost.h"
static void Complete(const PioneerQ38CohortWave& wave,tt_q38_cohort_probe& r)
{
	r.result={}; auto& result=r.result; result.version=1; result.state=TT_RUNTIME_PASSED;
	result.workers=result.started=result.completed=result.stopped=r.workers;
	for (unsigned w=0;w<r.workers;w++) {
		auto& core=result.core[w]; core.version=1; core.state=TT_RUNTIME_PASSED;
		core.prepared=core.context_reset=core.started=core.completed=core.stopped=1;
		core.phase=0x7200c; core.panels=r.part[w].groups*(r.inputs/16); core.marker=0x31483851; core.pc[0]=core.pc[1]=0x72038;
		unsigned blocks=r.inputs/PioneerQ38CohortBlockInputs(r.type),reads=blocks*r.part[w].groups;
		unsigned flits=PioneerQ38CohortBytes(r.type,r.inputs,r.part[w].groups)/64;
		if (w) { reads+=(r.inputs*2+16383)/16384+1; flits+=(r.inputs*2+((blocks*4+63)&~63U))/64; }
		for (unsigned c=0;c<8;c++) { core.before[c]=UINT32_MAX-10; core.after[c]=core.before[c]+(c==1?flits:(c==0 || c==2 || c==3 || c==6)?reads:0); }
		for (unsigned row=0;row<r.part[w].groups*16;row++) r.output[w][row]=PioneerAttentionBits(float(wave.firstRow[w]+row+1));
	}
}
static void Check(PioneerQwen38Gguf& model,FILE* file)
{
	PioneerQwen38PackedPlan packed; assert(packed.Build(model,8192,true));
	unsigned matrices=0,waves=0;
	for (unsigned t=0;t<model.Tensors().size();t++) {
		const auto& tensor=model.Tensors()[t]; if (!tensor.type) continue;
		std::vector<float> x(size_t(tensor.inputs)); for (unsigned k=0;k<x.size();k++) x[k]=float(int((k*29)%255)-127)*.00390625f;
		PioneerQ38CohortInput input; assert(PioneerQ38CohortQuantize(tensor.type,unsigned(tensor.inputs),x.data(),input));
		for (unsigned workers:{1U,7U,120U}) {
			PioneerQwen38FleetPlan plan; assert(plan.Build(model,packed,t,workers));
			std::vector<float> output(size_t(tensor.outputs),-1); std::vector<unsigned> rows(output.size(),0);
			unsigned count=plan.Jobs().back().wave+1;
			for (unsigned number=0;number<count;number++) {
				PioneerQ38CohortWave wave; assert(wave.Build(model,packed,plan,number) && wave.Ready());
				auto request=PioneerQ38CohortRequest(wave,input); assert(request && PioneerQ38CohortValid(*request));
				Complete(wave,*request); assert(PioneerQ38CohortResultProof(*request));
				assert(PioneerQ38CohortCollect(wave,input,*request,output.data(),unsigned(output.size())));
				for (unsigned w=0;w<wave.workers;w++) for (unsigned row=0;row<wave.part[w].groups*16;row++) assert(++rows[wave.firstRow[w]+row]==1);
				waves++;
			}
			for (unsigned row=0;row<output.size();row++) assert(rows[row]==1 && output[row]==float(row+1));
			PioneerQ38CohortWave invalid; assert(!invalid.Build(model,packed,plan,count));
		}
		matrices++;
	}
	const auto* tensor=model.Find("blk.0.ffn_down.weight"); assert(tensor); unsigned t=unsigned(tensor-model.Tensors().data());
	PioneerQwen38FleetPlan plan; assert(plan.Build(model,packed,t,7,3));
	PioneerQ38CohortWave wave; assert(wave.Build(model,packed,plan,0));
	std::vector<float> x(17408,.125f),scratch(5120,-1),before=scratch;
	PioneerQ38CohortInput input; assert(PioneerQ38CohortQuantize(12,17408,x.data(),input));
	auto good=PioneerQ38CohortRequest(wave,input); assert(good); Complete(wave,*good);
	for (unsigned fault=0;fault<40;fault++) {
		auto bad=std::make_unique<tt_q38_cohort_probe>(*good); auto& c=bad->result.core[bad->workers-1];
		switch(fault) {
			case 0:bad->version++;break; case 1:bad->result.version++;break; case 2:bad->result.state=TT_RUNTIME_NONFINITE;break;
			case 3:bad->result.workers--;break; case 4:bad->result.started--;break; case 5:bad->result.completed--;break;
			case 6:bad->result.stopped--;break; case 7:bad->result.recovery_required=1;break;
			case 8:c.version++;break; case 9:c.state=TT_RUNTIME_NONFINITE;break; case 10:c.prepared=2;break;
			case 11:c.context_reset=2;break; case 12:c.started=2;break; case 13:c.completed=2;break; case 14:c.stopped=2;break;
			case 15:c.recovery_required=1;break; case 16:c.phase++;break; case 17:c.panels++;break; case 18:c.marker++;break;
			case 19:c.pc[0]++;break; case 20:c.pc[1]++;break;
			case 29:bad->output[bad->workers-1][bad->part[bad->workers-1].groups*16-1]=0x7fc00000;break;
			case 30:bad->output[bad->workers-1][255]=1;break; case 31:bad->output[bad->workers][0]=1;break;
			case 32:bad->result.core[bad->workers].pc[0]=1;break; case 33:bad->activation[0]^=1;break;
			case 34:bad->scales[0]^=1;break; case 35:bad->part[0].offset+=64;break; case 36:bad->type=8;break;
			case 37:bad->inputs=6144;break; case 38:bad->reserved[0]=1;break; case 39:bad->part[0].groups++;break;
			default:c.after[fault-21]++;break;
		}
		assert(!PioneerQ38CohortCollect(wave,input,*bad,scratch.data(),unsigned(scratch.size())) && scratch==before);
	}
	for (unsigned fault=0;fault<10;fault++) {
		auto bad=wave;
		switch(fault) {
			case 0:bad.type=8;break; case 1:bad.inputs=6144;break; case 2:bad.outputs++;break; case 3:bad.workers++;break;
			case 4:bad.tensor++;break; case 5:bad.wave++;break; case 6:bad.part[0].bank^=1;break;
			case 7:bad.part[0].offset+=64;break; case 8:bad.firstRow[0]+=16;break; case 9:bad.part[bad.workers].groups=1;break;
		}
		assert(!bad.Ready() && !PioneerQ38CohortRequest(bad,input));
		assert(!PioneerQ38CohortCollect(bad,input,*good,scratch.data(),unsigned(scratch.size())) && scratch==before);
	}
	PioneerQwen38Gguf other; assert(other.Open(file)); PioneerQwen38PackedPlan another; assert(another.Build(other,8192,true));
	PioneerQ38CohortWave wrong; assert(!wrong.Build(other,another,plan,0));
	for (unsigned type:{8U,12U,14U}) {
		x.assign(17408,.125f); auto unchanged=input;
		for (uint32_t bits:{0x7f800000U,0x7fc00000U,1U,0x7f000000U}) {
			x[17407]=PioneerAttentionFloat(bits); assert(!PioneerQ38CohortQuantize(type,17408,x.data(),input) && !memcmp(&input,&unchanged,sizeof(input)));
		}
	}
	assert(model.Open(file) && !wave.Ready() && !PioneerQ38CohortRequest(wave,input));
	printf("PASS OFFLINE host cohort adapter:all%u matrices x1/7/120 workers,%u waves,exact global row assembly,distinct Q8_0/Q8_K input formats,ALL40 result/input/counter corruptions+10 stale/forged wave fields refuse atomically,source/epoch binding;NO FD/launch/CPU products/inference\n",matrices,waves);
}
int main(int argc,char** argv)
{
	assert(argc<=2); Fixture fixture; PioneerQwen38Gguf model; assert(model.Open(fixture.file)); Check(model,fixture.file);
	if (argc==2) { FILE* f=fopen(argv[1],"rb"); assert(f); PioneerQwen38Gguf real; assert(real.Open(f)); Check(real,f); fclose(f); }
	return 0;
}
