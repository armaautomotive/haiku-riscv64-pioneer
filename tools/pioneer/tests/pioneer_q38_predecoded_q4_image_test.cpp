/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DEFER_AFFINE_IMAGE_TEST_MODEL_ONLY
#include "pioneer_q38_defer_affine_image_test.cpp"
#include "../PioneerQ38PredecodedQ4.h"
// Original-format oracle is retained separately. ONLY wire bytes change.
struct PredecodedQ4Image:DeferredAffineImage {
	mutable DeferredAffineImage original;
	bool direct=false,directPending=false,omitUnpack=false,omitCompletion=false,omitAdcReset=false,directConfigured=false;
	unsigned directAddress=0,directUnpacks=0,directReads=0,directSetup=0;
	struct DirectHazard {};
	PredecodedQ4Image(const char* reference,const char* candidate,unsigned type,unsigned k,unsigned groups,bool work,unsigned worker=1)
		:DeferredAffineImage(candidate,type,k,groups,worker),original(reference,type,k,groups,worker) {
		direct=work && type==12;
		if(!direct)return;
		for(auto& bytes:tiles) {
			std::vector<uint8_t> expanded(PioneerQ38PredecodedQ4::Bytes,0xcc);
			assert(PioneerQ38PredecodedQ4::Prepare(bytes.data(),unsigned(bytes.size()),expanded.data(),unsigned(expanded.size())));bytes.swap(expanded);
		}
		tile=tiles[0];memory[0x74008]=unsigned(tile.size());
		for(unsigned w=0;w<tile.size()/4;w++)memory[0x90000+w*4]=Word(tile,w*4);
		for(auto& p:packets)if(p.tile>=0){p.address=base+unsigned(p.tile)*unsigned(tile.size());p.bytes=unsigned(tile.size());}
	}
	bool DirectQ4Panel() const override {return direct;}
	bool NativeInt8Unpack() const override {return direct;}
	virtual unsigned DirectPanelBase() const {return 0x90100;}
	virtual unsigned DirectPanels() const {return 16;}
	virtual int DirectValue(unsigned value) const {if(value>15)throw DirectHazard{};return int(value);}
	void Instruction(uint32_t op) override {
		if(direct && directConfigured && config[0]==0x423e && !ownerA && !sources[mathBank] && !directPending) {
			if(op==0x3700000f && !directSetup)directSetup=1;
			else if(directSetup) {
				const uint32_t expected[]={0,0x5e23fc00,0x5120000a,0x5420000f,0x420000c1};
				if(directSetup>4 || op!=expected[directSetup] || (omitAdcReset && directSetup==2))throw DirectHazard{};
				directSetup++;
			}
		}
		if(direct && op==0x420000c1) {
			assert(!directPending && !rawPending && !pending && phaseQueue.empty() && pendingAccumulator.empty()
				// RWC/ADC reset operations may be queued in their own units;
				// ALL consumers/metadata stores completed before config above.
				&& !sources[unpackBank] && !ownerA && thread[5]==4 && adc1==255
				&& config[0]==0x423e && config[1]==0xa03c0000 && config[49]==0
				&& config[64]==0x10001e && config[65]==16 && config[72]==14 && config[73]==3 && config[84]==0x400040);
			assert(directSetup==5 && directConfigured);directSetup=0;directConfigured=false;
			directAddress=(config[76]+1)*16;unsigned panel=matrixInstructions/MatrixPhasesPerPanel()%DirectPanels();
			assert(directAddress==DirectPanelBase()+panel*256);directPending=true;drained=false;directUnpacks++;return;
		}
		if(directPending) {
			if(op!=0xa2101816 && op!=0x4500017e)throw DirectHazard{};
			if(op==0x4500017e) {
				if(omitCompletion){gpr63=1;return;} // deliberately FALSE acknowledgement,do not publish bank
				for(unsigned i=0;i<256;i++) {
					unsigned at=directAddress+i,value=(memory.at(at&~3U)>>(8*(at&3)))&255;
					if(omitUnpack)throw DirectHazard{};
					a[i/16][i%16]=float(DirectValue(value));directReads++;
				}
				sources[unpackBank]=true;unpackBank^=1;ownerA=sources[mathBank];directPending=false;panelMoves=0;
			}
		}
		DeferredAffineImage::Instruction(op);
	}
	void Store(uint32_t at,uint32_t value) override {
		if(direct && at==0xffef0130 && config[0]==0x423e) {
			uint64_t address=(uint64_t(value)+1)*16;
			if(address>=DirectPanelBase() && address<DirectPanelBase()+DirectPanels()*256)directConfigured=true;
		}
		if(direct && at==0xffef0000 && setupConfiguration==25 && config[0]
			&& value!=config[0] && (value==0x4231 || value==0x423e)) {
			assert(drained && !directPending && !rawPending && phaseQueue.empty() && pendingAccumulator.empty() && !ownerA && !ownerB);
			assert(config[0]==(value==0x4231?0x423e:0x4231));config[0]=value;configWrites++;return;
		}
		if(directPending && at!=0xffe000fc && !Range(at,0x6e000,8192))throw DirectHazard{};
		DeferredAffineImage::Store(at,value);
	}
	bool CorrectRows() const override {
		original.vector=vector;original.scales=scales;original.matrixInstructions=matrixInstructions;
		memcpy(original.buffers,buffers,sizeof(buffers));return original.CorrectRows();
	}
};
#ifndef PIONEER_Q38_PREDECODED_Q4_IMAGE_MODEL_ONLY
int main(int argc,char** argv) {
	assert(argc==3 || (argc==4 && !strcmp(argv[3],"--q4-work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
	for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})
	for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
		if(argc==4 && (type!=12 || k!=5120 || groups!=1 || !work))continue;
		printf("PREDECODE_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
		DeferredAffineImage old(argv[1],type,k,groups);PredecodedQ4Image next(argv[1],argv[2],type,k,groups,work);
		assert(old.Boot() && next.Boot());old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for(unsigned job=1;job<=2;job++) {
			if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
			old.Poison();next.Poison();unsigned before=old.steps,after=next.steps;
			assert(old.Job(job) && next.Job(job));
			assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
				&& old.sumWrites==next.sumWrites && old.sumReads==next.sumReads && old.dotProbeReads==next.dotProbeReads);
			for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if(type==12 && work) {
				assert(next.directUnpacks==job*groups*(k/16) && next.directReads==next.directUnpacks*256 && !next.nativeStoreOps && !next.moveInstructions);
				if(groups==16 && job==2)printf("OFFLINE_PREDECODE K%u RV%u/%u vector_ops%u/%u drains%u/%u;NOT_NATIVE_OR_TOKEN_RATE\n",k,old.steps-before,next.steps-after,old.vectorOps,next.vectorOps,old.drains,next.drains);
			}
		}
		old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2) && next.Job(3,2));
	}
	for(unsigned fault=0;fault<4;fault++) {
		PredecodedQ4Image bad(argv[1],argv[2],12,5120,1,true);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();
		if(fault==0)bad.omitUnpack=true;if(fault==1)bad.tiles[0][256]=16;
		if(fault==2)bad.omitCompletion=true;if(fault==3)bad.omitAdcReset=true;
		bool caught=false;try{(void)bad.Job(1);}catch(const PredecodedQ4Image::DirectHazard&){caught=true;}
		assert(caught && !bad.outputReads && !bad.memory.at(0x74144));
	}
	for(unsigned fault:{1U,5U,40U,208U}) {
		PredecodedQ4Image bad(argv[1],argv[2],12,17408,3,true);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.faultAt=fault;
		assert(!bad.Job(1) && bad.pending && bad.requests==fault && bad.responses==fault-1 && !bad.memory.at(0x74144));
		unsigned requests=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests);
	}
	puts(argc==4?"PASS PARTIAL ONLY predecoded-Q4 actual image pilot+deferred unpack/value failure observers;FULL SUITE SKIPPED":"PASS OFFLINE ACTUAL predecoded-Q4:ORIGINAL format FP32+FP64 exact,all formats/widths/groups/modes/two changed inputs,DEFERRED native SM7 unpack,zero SFPU quant decode/MOV,unchanged matrix/source/STOP,checked/non-Q4 old layout,missing unpack/out-of-domain refuse before publication;NO_NATIVE_LAYOUT_OWNER_OR_SPEED_QUALIFICATION");
}
#endif
