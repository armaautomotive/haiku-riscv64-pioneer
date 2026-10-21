/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DEFER_AFFINE_IMAGE_TEST_MODEL_ONLY
#include "pioneer_q38_defer_affine_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ScaleReusePeerImage.h"
struct ScaleImage:DeferredAffineImage {
	using DeferredAffineImage::DeferredAffineImage;
	struct ScaleLifetimeHazard {};
	unsigned scaleLoads=0; bool loseScale=false;
	void Instruction(uint32_t op) override {
		if (BlockAffineActive() && op>>24==0x71 && ((op>>20)&7)==3) {
			scaleLoads++; if (loseScale) return;
		}
		if (BlockAffineActive() && op>>24==0x86 && ((op>>12)&15)==3) {
			unsigned panel=matrixInstructions/MatrixPhasesPerPanel()-1,block=(panel/16)%(inputs/256);
			uint32_t scale=memory.at(0x94000+block*4);
			for(unsigned lane=0;lane<32;lane++) if(lr[3][lane]!=scale) throw ScaleLifetimeHazard{};
		}
		DeferredAffineImage::Instruction(op);
	}
};
int main(int argc,char** argv) {
	assert(argc==3 || (argc==4 && !strcmp(argv[3],"--q4-work-pilot"))); setvbuf(stdout,nullptr,_IOLBF,0);
	const auto code=Read((std::string(argv[2])+"/text.bin").c_str()),prefix=Read((std::string(argv[2])+"/affine.bin").c_str());
	for(unsigned w=0;w<2048;w++) {
		assert(PioneerQ38ScaleReusePeerWord(w)==(w*4<code.size()?Word(code,w*4):0));unsigned at=0x70000+w*4;
		assert(PioneerQ38ScaleReusePeerPrefixWord(at)==(at>=0x70b00 && at-0x70b00<prefix.size()?Word(prefix,at-0x70b00):0));
	}
	assert(PioneerQ38ScaleReusePeerWord(2048)==UINT32_MAX);
	for(unsigned at:{0x6fffcU,0x72000U,0x70001U})assert(PioneerQ38ScaleReusePeerPrefixWord(at)==UINT32_MAX);
	for(unsigned type:{8U,12U,14U}) for(unsigned k:{5120U,6144U,17408U})
	for(unsigned groups:{1U,3U,16U}) for(bool work:{false,true}) {
		if(argc==4 && (type!=12 || k!=5120 || groups!=1 || !work))continue;
		printf("SCALE_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
		ScaleImage old(argv[1],type,k,groups),next(argv[2],type,k,groups);
		old.PoisonDst();next.PoisonDst(); for(unsigned lane=0;lane<32;lane++) old.lr[3][lane]=next.lr[3][lane]=0x7fc1abcd;
		assert(old.Boot() && next.Boot());old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for(unsigned job=1;job<=2;job++) {
			if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
			old.Poison();next.Poison();unsigned before=old.steps,after=next.steps;
			assert(old.Job(job) && next.Job(job));
			assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
				&& old.drains==next.drains && old.configWrites==next.configWrites && old.dotProbeReads==next.dotProbeReads
				&& old.sumWrites==next.sumWrites && old.sumReads==next.sumReads);
			for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if(work && type==12){
				assert(old.scaleLoads==32*next.scaleLoads && next.scaleLoads==job*groups*(k/256)*2);
				if(groups==16 && job==2)printf("OFFLINE_SCALE_REUSE K%u RV%u/%u scale_ops%u/%u;NOT_SILICON_OR_TOKEN_RATE\n",k,old.steps-before,next.steps-after,old.scaleLoads,next.scaleLoads);
			}else assert(old.scaleLoads==next.scaleLoads);
		}
		old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2) && next.Job(3,2));
	}
	ScaleImage missing(argv[2],12,5120,1);assert(missing.Boot());missing.memory[0x74038]=1;missing.Poison();missing.loseScale=true;
	bool caught=false;try{(void)missing.Job(1);}catch(const ScaleImage::ScaleLifetimeHazard&){caught=true;}
	assert(caught && !missing.outputReads && !missing.memory.at(0x74144));
	puts(argc==4?"PASS PARTIAL ONLY scale reuse Q4 Work5120/group1+missing-load observer;FULL suite SKIPPED":"PASS OFFLINE ACTUAL Q4 scale reuse:all formats/widths/groups/modes/two changed-input jobs EXACT,ALL32 LReg3 lanes current-block identity before EACH use,unchanged matrix/config/drains/NoC,poisoned register and missing-load observer refusal;NO native/speed qualification");
}
