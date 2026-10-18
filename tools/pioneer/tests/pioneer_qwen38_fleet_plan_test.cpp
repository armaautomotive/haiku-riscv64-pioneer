/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQwen38FleetPlan.h"
static void Check(PioneerQwen38Gguf& model,FILE* file)
{
	PioneerQwen38PackedPlan packed; assert(packed.Build(model,8192,true));
	unsigned tensors=0,unaligned=0;
	for (unsigned t=0;t<model.Tensors().size();t++) {
		const auto& tensor=model.Tensors()[t]; if (!tensor.type) continue;
		for (unsigned workers:{1U,7U,119U,120U}) {
			PioneerQwen38FleetPlan plan; assert(plan.Build(model,packed,t,workers));
			std::vector<unsigned> covered(size_t(tensor.outputs/16),0),used(workers,0);
			unsigned groupBytes=16*PioneerQ38TiledRowBytes(tensor.type,tensor.inputs),ordinal=0;
			for (const auto& j:plan.Jobs()) {
				assert(j.worker==ordinal%workers && j.wave==ordinal/workers && j.groups && j.groups<=16 && !(j.firstRow&15) && j.bank<7 && !(j.proposedOffset&63));
				const auto& p=packed.Parts().at(j.fragment);
				assert(p.tensor==t && p.bank==j.bank && j.firstRow>=p.firstRow && j.firstRow+j.groups*16<=p.firstRow+p.rows
					&& j.proposedOffset==uint64_t(p.proposedAddress)-packed.Base+uint64_t((j.firstRow-p.firstRow)/16)*groupBytes && j.bytes==j.groups*groupBytes);
				for (unsigned g=0;g<j.groups;g++) assert(++covered[j.firstRow/16+g]==1);
				used[j.worker]++; unaligned+=bool(j.proposedOffset&4095); ordinal++;
			}
			for (unsigned v:covered) assert(v==1);
			if (tensor.outputs/16>=workers) for (unsigned v:used) assert(v);
			else for (unsigned w=0;w<workers;w++) assert(bool(used[w])==(w<tensor.outputs/16));
			assert(plan.Ready() && plan.Tensor()==t && plan.Workers()==workers);
		}
		tensors++;
	}
	assert(tensors==498 && unaligned);
	PioneerQwen38FleetPlan plan;
	unsigned quant=0; while (!model.Tensors()[quant].type) quant++;
	for (unsigned w:{0U,121U}) assert(!plan.Build(model,packed,quant,w) && !plan.Ready() && plan.Jobs().empty());
	for (unsigned g:{0U,17U}) assert(!plan.Build(model,packed,quant,120,g) && plan.Jobs().empty());
	assert(!plan.Build(model,packed,851));
	unsigned scalar=0; while (model.Tensors()[scalar].type) scalar++;
	assert(!plan.Build(model,packed,scalar));
	PioneerQwen38PackedPlan rowPlan; assert(rowPlan.Build(model,8192,false) && !plan.Build(model,rowPlan,quant));
	PioneerQwen38Gguf other; assert(other.Open(file) && !plan.Build(other,packed,quant));
	auto& mutableParts=const_cast<std::vector<PioneerQ38PackedPart>&>(packed.Parts());
	unsigned part=0; while (mutableParts[part].kind!=PIONEER_Q38_WEIGHT || mutableParts[part].tensor!=quant) part++;
	auto original=mutableParts[part];
	for (unsigned fault=0;fault<12;fault++) {
		auto& bad=mutableParts[part]; bad=original;
		switch (fault) {
			case 0:bad.bank=7;break; case 1:bad.rows=0;break; case 2:bad.firstRow++;break;
			case 3:bad.rows=UINT32_MAX;break; case 4:bad.rowBytes++;break; case 5:bad.bytes--;break;
			case 6:bad.proposedAddress=packed.Base-4096;break; case 7:bad.proposedAddress++;break;
			case 8:bad.allocated=0;break; case 9:bad.allocated++;break;
			case 10:bad.proposedAddress=0xc4000000;break; case 11:bad.tensor=851;break;
		}
		assert(!plan.Build(model,packed,quant) && !plan.Ready() && plan.Jobs().empty());
	}
	mutableParts[part]=original;
	assert(plan.Build(model,packed,quant));
	assert(file && model.Open(file));
	assert(!plan.Ready() && plan.Jobs().empty());
	printf("PASS OFFLINE fleet blueprint:all%u quantized tensors x1/7/119/120 workers,exact rows/bytes/fragment/bank bounds,no gaps/duplicate wave workers,all workers used when useful,64B offsets/invalid shape and source epoch refusal;NOT native50-compatible coordinates/launch/inference\n",tensors);
}
int main(int argc,char** argv)
{
	assert(argc<=2); Fixture fixture; PioneerQwen38Gguf model; assert(model.Open(fixture.file)); Check(model,fixture.file);
	if (argc==2) { FILE* f=fopen(argv[1],"rb"); assert(f); PioneerQwen38Gguf real; assert(real.Open(f)); Check(real,f); fclose(f); }
}
