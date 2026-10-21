/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PREDECODED_Q4_IMAGE_MODEL_ONLY
#include "pioneer_q38_predecoded_q4_image_test.cpp"
#include "../PioneerQ38NativeQ8Data.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NativeQ8PeerImage.h"
// Explicit SM7-domain SYNTHETIC fixtures,not clipping any public model.
// Checked mode below retains original full-range TC fixtures including-128.
static void NativeQ8Fixture(DeferredAffineImage& m) {
	for(unsigned t=0;t<m.tiles.size();t++)for(unsigned i=32;i<544;i++) {
		int q=int((i*37+t*13+(i%16)*59)%255)-127;m.tiles[t][i]=uint8_t(q);
	}
	m.tile=m.tiles[0];for(unsigned w=0;w<m.tile.size()/4;w++)m.memory[0x90000+w*4]=Word(m.tile,w*4);
}
struct NativeQ8Image:PredecodedQ4Image {
	bool freshSetup=false,omitLaneEnable=false,omitMetadataRestore=false;unsigned nativeFormatWrites=0;
	NativeQ8Image(const char* reference,const char* candidate,unsigned type,unsigned k,unsigned groups,bool work,unsigned worker=1)
		:PredecodedQ4Image(reference,candidate,type,k,groups,false,worker) {
		direct=work && type==8;if(!direct)return;
		NativeQ8Fixture(*this);NativeQ8Fixture(original);
		for(auto& bytes:tiles) {
			std::array<uint8_t,4096> page{},encoded{};for(unsigned i=0;i<576;i++)page[i]=bytes[i];
			assert(PioneerQ38NativeQ8Data::PreparePage(page.data(),0,576,encoded.data(),4096));
			for(unsigned i=0;i<576;i++)bytes[i]=encoded[i];
		}
		tile=tiles[0];for(unsigned w=0;w<tile.size()/4;w++)memory[0x90000+w*4]=Word(tile,w*4);
	}
	// Cooperative root tests supply their own original DATA inputs/scales;
	// the original oracle never executes its stored code image.
	NativeQ8Image(const char* candidate,unsigned type,unsigned k,unsigned groups,unsigned worker)
		:NativeQ8Image(candidate,candidate,type,k,groups,true,worker) {}
	unsigned DirectPanelBase()const override{return 0x90020;}
	unsigned DirectPanels()const override{return 2;}
	int DirectValue(unsigned byte)const override{if(byte==128)throw DirectHazard{};return int(byte&127)*((byte&128)?-1:1);}
	bool DirectNativeQ8Sfpu()const override{return direct;}
	void Instruction(uint32_t op)override {
		if(direct && op==0xb2000000)freshSetup=true;
		if(direct && config[0]==0x423e) {
			if(op==0x8a000002 && omitLaneEnable)return;
			if(op>>24==0x71 && !enabled)throw DirectHazard{};
		}
		PredecodedQ4Image::Instruction(op);
	}
	void Store(uint32_t at,uint32_t value)override {
		if(direct && at==0xffef0004 && value==0xa03c0000) {
			nativeFormatWrites++;if(omitMetadataRestore && nativeFormatWrites>1)return;
		}
		if(direct && freshSetup && at==0xffef0000) {
			assert(!directPending && !rawPending && phaseQueue.empty() && pendingAccumulator.empty() && drained);
			freshSetup=false;DeferredAffineImage::Store(at,value);return;
		}
		PredecodedQ4Image::Store(at,value);
	}
};
#ifndef PIONEER_Q38_NATIVE_Q8_IMAGE_MODEL_ONLY
int main(int argc,char** argv) {
	assert(argc==3 || (argc==4 && !strcmp(argv[3],"--q8-work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
	const auto code=Read((std::string(argv[2])+"/text.bin").c_str()),prefix=Read((std::string(argv[2])+"/affine.bin").c_str());
	for(unsigned w=0;w<2048;w++) {
		assert(PioneerQ38NativeQ8PeerWord(w)==(w*4<code.size()?Word(code,w*4):0));unsigned at=0x70000+w*4;
		assert(PioneerQ38NativeQ8PeerPrefixWord(at)==(at>=0x70b00 && at-0x70b00<prefix.size()?Word(prefix,at-0x70b00):0));
	}
	assert(PioneerQ38NativeQ8PeerWord(2048)==UINT32_MAX);
	for(unsigned at:{0x6fffcU,0x72000U,0x70001U})assert(PioneerQ38NativeQ8PeerPrefixWord(at)==UINT32_MAX);
	for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})
	for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
		if(argc==4 && (type!=8 || k!=5120 || groups!=1 || !work))continue;
		printf("NATIVE_Q8_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
		DeferredAffineImage old(argv[1],type,k,groups);NativeQ8Image next(argv[1],argv[2],type,k,groups,work);
		if(type==8 && work)NativeQ8Fixture(old);
		assert(old.Boot() && next.Boot());old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for(unsigned job=1;job<=2;job++) {
			if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
			old.Poison();next.Poison();unsigned before=old.steps,after=next.steps;
			assert(old.Job(job) && next.Job(job));
			assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
				&& old.sumWrites==next.sumWrites && old.sumReads==next.sumReads && old.dotProbeReads==next.dotProbeReads);
			for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if(type==8 && work) {
				assert(next.directUnpacks==job*groups*(k/16) && next.directReads==next.directUnpacks*256 && !next.nativeStoreOps && !next.moveInstructions);
				if(groups==16 && job==2)printf("OFFLINE_NATIVE_Q8 K%u RV%u/%u vector_ops%u/%u drains%u/%u;NOT_NATIVE_OR_TOKEN_RATE\n",k,old.steps-before,next.steps-after,old.vectorOps,next.vectorOps,old.drains,next.drains);
			}
		}
		old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2) && next.Job(3,2));
	}
	for(unsigned fault=0;fault<6;fault++) {
		NativeQ8Image bad(argv[1],argv[2],8,5120,1,true);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();
		if(fault==0)bad.omitUnpack=true;if(fault==1)bad.tiles[0][32]=128;
		if(fault==2)bad.omitCompletion=true;if(fault==3)bad.omitAdcReset=true;
		if(fault==4)bad.omitLaneEnable=true;if(fault==5)bad.omitMetadataRestore=true;
		if(fault==5){assert(!bad.Job(1) && bad.nativeFormatWrites==2 && !bad.outputReads && !bad.memory.at(0x74144));continue;}
		bool caught=false;try{(void)bad.Job(1);}catch(const NativeQ8Image::DirectHazard&){caught=true;}
		assert(caught && !bad.outputReads && !bad.memory.at(0x74144));
	}
	for(unsigned fault:{1U,5U,544U,1636U}) {
		NativeQ8Image bad(argv[1],argv[2],8,17408,3,true);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.faultAt=fault;
		assert(!bad.Job(1) && bad.pending && bad.requests==fault && bad.responses==fault-1 && !bad.memory.at(0x74144));
		unsigned count=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(count==bad.requests);
	}
	puts(argc==4?"PASS PARTIAL ONLY native-Q8 actual image pilot+deferred unpack/failure observers;FULL SUITE SKIPPED":"PASS OFFLINE ACTUAL native-Q8:ORIGINAL TC source FP32+FP64 EXACT,all formats/widths/groups/modes/two changed inputs,SM7-domain synthetic Work,checked full-TC including-128 preserved,DEFERRED source ownership,zero SFPU decode/MOV,same packet footprint,sticky format/explicit all-lane enable,STOP and failure observers;NO_NATIVE_SOURCE_ABI_OR_SPEED_QUALIFICATION");
}
#endif
