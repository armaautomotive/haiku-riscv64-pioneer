/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DIRECT_SRCB_IMAGE_MODEL_ONLY
#include "pioneer_q38_direct_srcb_image_test.cpp"
#include <array>
// Separate native-SM7 source observer copied from the OFFLINE predecoded
// study,but ORIGINAL NoC tile/descriptor/packets remain untouched. Only
// exact new scratch writes and this freshly expanded block may feed SrcA.
struct LocalQ4Image:DirectSrcBImage {
	bool direct=false,directPending=false,omitUnpack=false,omitCompletion=false,omitAdcReset=false,directConfigured=false;
	unsigned directAddress=0,localUnpacks=0,localReads=0,directSetup=0;
	struct DirectHazard {};
	std::vector<uint8_t> helper;std::array<bool,1088> scratch{};
	unsigned scratchBlocks=0,scratchWrites=0,phaseStart=0,omitScratch=UINT32_MAX,restoreOmissions=0;
	bool omitRestore=false;
	LocalQ4Image(const char* candidate,unsigned t,unsigned k,unsigned g,bool work)
		:DirectSrcBImage(candidate,t,k,g) {
		direct=work&&t==12;helper=Read((std::string(candidate)+"/fetch.bin").c_str());assert(helper.size()==960);
		for(unsigned w=0;w<helper.size()/4;w++)l1[Index(0x70000+w*4)]=Word(helper,w*4);
	}
	bool CodeAddress(uint32_t at)const override{return Range(at,0x70000,unsigned(helper.size()))||DirectSrcBImage::CodeAddress(at);}
	uint32_t Load(uint32_t at)override{return Range(at,0x70000,unsigned(helper.size()))?l1[Index(at)]:DirectSrcBImage::Load(at);}
	void ResetScratch(){scratch.fill(false);scratchBlocks=scratchWrites=localUnpacks=localReads=0;phaseStart=matrixInstructions;
		for(unsigned w=0;w<1088;w++)memory[0x92000+w*4]=0xdeadbeef;
		memory[0x91ffc]=memory[0x93100]=0xcafebabe;}
	bool DirectQ4Panel() const override {return direct;}
	bool DirectNativeQ8Sfpu()const override{return direct;} // same existing sticky native-SM7 SFPU contract
	bool NativeInt8Unpack() const override {return direct;}
	virtual unsigned DirectPanelBase() const {return 0x92100;}
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
			if(std::count(scratch.begin(),scratch.end(),true)!=1088 || scratchBlocks!=(matrixInstructions-phaseStart)/(16*MatrixPhasesPerPanel())+1)throw DirectHazard{};
			assert(!directPending && !rawPending && !pending && phaseQueue.empty() && pendingAccumulator.empty()
				// RWC/ADC reset operations may be queued in their own units;
				// ALL consumers/metadata stores completed before config above.
				&& !sources[unpackBank] && !ownerA && thread[5]==4 && adc1==255
				&& config[0]==0x423e && config[1]==0xa03c0000 && config[49]==0
				&& config[64]==0x10001e && config[65]==16 && config[72]==14 && config[73]==3 && config[84]==0x400040);
			assert(directSetup==5 && directConfigured);directSetup=0;directConfigured=false;
			directAddress=(config[76]+1)*16;unsigned panel=matrixInstructions/MatrixPhasesPerPanel()%DirectPanels();
			assert(directAddress==DirectPanelBase()+panel*256);directPending=true;drained=false;localUnpacks++;return;
		}
		if(directPending) {
			if(op!=0xa2101816 && op!=0x4500017e)throw DirectHazard{};
			if(op==0x4500017e) {
				if(omitCompletion){gpr63=1;return;} // deliberately FALSE acknowledgement,do not publish bank
				for(unsigned i=0;i<256;i++) {
					unsigned at=directAddress+i,value=(memory.at(at&~3U)>>(8*(at&3)))&255;
					if(omitUnpack)throw DirectHazard{};
					a[i/16][i%16]=float(DirectValue(value));localReads++;
				}
				sources[unpackBank]=true;unpackBank^=1;ownerA=sources[mathBank];directPending=false;panelMoves=0;
			}
		}
		DirectSrcBImage::Instruction(op);
	}
	void Store(uint32_t at,uint32_t value) override {
		if(direct&&omitRestore&&at==0xffef0000&&value==0x4231&&config[0]==0x423e){restoreOmissions++;return;}
		if(Range(at,0x92000,4352)) {
			assert(direct&&!directPending&&!pending&&!rawPending&&phaseQueue.empty()&&pendingAccumulator.empty()&&drained&&!(at&3));
			unsigned word=(at-0x92000)/4;
			if(!word){assert(matrixInstructions-phaseStart==scratchBlocks*16*MatrixPhasesPerPanel());scratch.fill(false);scratchBlocks++;}
			assert(scratchBlocks&&!scratch[word]);unsigned panel=(word>=64?(word-64)/64:0),column=(word>=64?(word-64)%64:0);
			unsigned source=word<64?word:64+(panel/4)*128+(panel%2)*64+column;
			uint32_t expected=memory.at(0x90000+source*4);
			if(word>=64)expected=(expected>>((panel&2)?4:0))&0x0f0f0f0fU;
			assert(value==expected);scratchWrites++;
			if(word!=omitScratch){memory[at]=value;scratch[word]=true;}return;
		}
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
		DirectSrcBImage::Store(at,value);
	}
};

int main(int argc,char** argv) {
 assert(argc==3||(argc==4&&!strcmp(argv[3],"--work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}){
  if(argc==4&&(type!=12||k!=5120||groups!=1||!work))continue;
  printf("LOCAL_Q4_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
  DirectSrcBImage old(argv[1],type,k,groups);LocalQ4Image next(argv[2],type,k,groups,work);
  assert(old.Boot()&&next.Boot());old.memory[0x74038]=next.memory[0x74038]=work;
  for(unsigned job=1;job<=2;job++){
   if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
   old.Poison();next.Poison();old.PoisonTail();next.PoisonTail();next.ResetScratch();
   unsigned a=old.steps,b=next.steps,od=old.drains,nd=next.drains;
   assert(old.Job(job)&&next.Job(job));
   assert(old.requests==next.requests&&old.responses==next.responses&&old.matrixInstructions==next.matrixInstructions
    &&old.sumWrites==next.sumWrites&&old.sumReads==next.sumReads&&old.dotProbeReads==next.dotProbeReads
    &&old.affineRecords==next.affineRecords&&old.affineExpansions==next.affineExpansions
    &&old.paddingWrites==next.paddingWrites&&old.baseWrites==next.baseWrites&&old.copyWrites==next.copyWrites
    &&old.directReads==next.directReads&&!next.directPending&&!next.rawPending&&next.pendingAccumulator.empty()&&next.phaseQueue.empty());
   for(unsigned w=0;w<64;w++)assert(old.memory.at(0x83f00+w*4)==next.memory.at(0x83f00+w*4));
   for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
   assert(next.memory.at(0x91ffc)==0xcafebabe&&next.memory.at(0x93100)==0xcafebabe);
   if(work&&type==12){
    unsigned blocks=groups*(k/256);assert(next.scratchBlocks==blocks&&next.scratchWrites==blocks*1088&&next.localUnpacks==blocks*16&&next.localReads==blocks*4096);
    assert(old.drains-od==next.drains-nd+blocks*28);
    printf("OFFLINE_LOCAL_Q4 RV%u/%u drains%u/%u fresh_blocks%u;NO_NATIVE_RATE_CLAIM\n",old.steps-a,next.steps-b,old.drains-od,next.drains-nd,blocks);
   }else assert(!next.scratchWrites&&!next.localUnpacks&&old.vectorOps==next.vectorOps&&old.drains==next.drains);
  }
  old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2)&&next.Job(3,2));
 }
 for(unsigned fault:{1U,5U,40U,208U}){
  LocalQ4Image bad(argv[2],12,17408,3,true);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();bad.ResetScratch();bad.faultAt=fault;
  assert(!bad.Job(1)&&bad.pending&&bad.requests==fault&&!bad.memory.at(0x74144));
  unsigned requests=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.pending&&bad.requests==requests);
 }
 for(unsigned fault=0;fault<7;fault++){
  LocalQ4Image bad(argv[2],12,5120,1,true);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();bad.ResetScratch();
  if(fault<3)bad.omitScratch=fault==0?0: fault==1?64:1087;
  bad.omitUnpack=fault==3;bad.omitCompletion=fault==4;bad.omitAdcReset=fault==5;
  if(fault==6){bad.omitRestore=true;assert(!bad.Job(1)&&bad.restoreOmissions==1&&!bad.outputReads&&!bad.memory.at(0x74144));continue;}
  bool caught=false;try{(void)bad.Job(1);}catch(const LocalQ4Image::DirectHazard&){caught=true;}
  assert(caught&&!bad.outputReads&&!bad.memory.at(0x74144));
 }
 puts(argc==4?"PASS PARTIAL local Q4 pilot/faults;FULL SUITE SKIPPED":"PASS OFFLINE local Q4 ALL54 shapes/twochangedjobs/STOP:original FP32+independent FP64/SAME64proofs/NoC/source/affine,original quantized wire/DRAM,only saved local scratch expansion/native SrcA path;missing scratch/unpack/completion/ADC and NoC fault observers reject;NO native/driver/speed qualification");
}
