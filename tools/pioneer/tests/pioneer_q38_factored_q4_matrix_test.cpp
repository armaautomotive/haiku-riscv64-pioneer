/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_cached_fpu_machine.h" // unchanged RV32 Step/NoC/cache model
#include "pioneer_q38_q4_factored_operator_machine.h"
#define PIONEER_Q38_FACTORED_OPERATOR_FIXTURE_ONLY
#include "pioneer_q38_q4_factored_operator_test.cpp"
struct FactoredConnected:FactoredOperatorMachine {
 CachedIO* bus=nullptr;unsigned blocks=0,inputBlockBase=0;mutable unsigned readerStart=0;
 unsigned OperandOffset()const override{return ((groups/8)&1)*0x10000;}
 bool OperandReady(unsigned a,unsigned bytes)const override{
  const unsigned slot=(groups/8)&1;
  const bool valid=bus && bus->ready[slot] && bus->copies[slot]==CL::Packets
   && bus->dataBlock[slot]==(groups/8-inputBlockBase)%blocks
   && a>=CL::Raw+slot*0x10000 && a+bytes<=0x963c0+slot*0x10000;
  if(valid && a==CL::Metadata+slot*0x10000 && !(compactImports%2)){bus->reader=slot;readerStart=groups;}
  return valid && bus->reader==slot;
 }
};
struct FactoredMatrixRV32:ServiceMachine {
 FactoredConnected& native;CachedIO bus;bool awaiting=false;unsigned acks=0,drainedGroups=0,constants=0;
 FactoredMatrixRV32(const char* folder,FactoredConnected& m,const CL::Job& job,const std::vector<uint8_t>& root)
  :ServiceMachine(folder,m.l1),native(m),bus(job,root,&m.l1){native.bus=&bus;native.blocks=job.words[2]/256;native.delay[0]=100003;native.delay[1]=7;native.packDelay=100019;}
 bool InlineTensix()const override{return true;}
 void Progress()override{
  if(bus.reader<2 && drainedGroups==native.readerStart+8 && native.Clean() && !awaiting)bus.reader=2;
  bus.Progress();
 }
 void FenceObserved()override{bus.Fence();}
 void Instruction(unsigned op)override{
  if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
  else if(op==0x4500017e){assert(awaiting && !gpr63);acks++;native.Drain();drainedGroups=native.groups;gpr63=1;awaiting=false;}
  else native.Op(op);
  Progress();
 }
 void Store(unsigned a,unsigned v)override{
  assert(!(a&3));
  if(a==0xffe40000){Instruction(v);return;}
  if(a==0xffe000fc){assert(!v);gpr63=0;return;}
  if(Range(a,0xffef0000,224*4)){native.Config((a-0xffef0000)/4,v);Progress();return;}
  if(CL::HeaderWord(a) || Range(a,0xffb20000,0x2000)){bus.Write(a,v);return;}
  if(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,CL::Output,1024)){
   if(Range(a,0x6e000,8192))minStore=std::min(minStore,a);
   if(Range(a,CL::Output,1024)){assert(native.Clean() && !awaiting && bus.reader==2 && bus.completed==bus.issues && bus.queued.empty() && bus.responses.empty());stores++;}
   Put(a,v);Progress();return;
  }
  assert(bus.issues==bus.requestStart);native.Write(a,v);constants++;Progress();
 }
 unsigned Load(unsigned a)override{
  assert(!(a&3));if(a==0xffe000fc)return gpr63;
  if(Range(a,0xffef0000,224*4)){assert(!native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4];}
  if(CL::HeaderWord(a) || Range(a,0xffb20000,0x2000))return bus.Read(a);
  if(Range(a,0xffbd9400,64) || Range(a,0xffbd9600,64)){
   assert(native.Clean() && !awaiting && bus.reader==2 && bus.completed==bus.issues);
   const unsigned row=(a-0xffbd8000)/64,col=a%64/4;native.outputReads++;return native.FloatDst(row,col);
  }
  assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x74000,128) || Range(a,0x17ffc0,12));return Get(a);
 }
 void Reenter(const CL::Job& job){
  assert(pc==0x70038 && native.Clean() && !awaiting && !(native.groups%8));
  bus.Select(job);native.blocks=job.words[2]/256;native.inputBlockBase=native.groups/8;
  for(unsigned i=0;i<16;i++)Put(0x74000+i*4,job.words[i]);
  for(unsigned i=0;i<3;i++)Put(0x17ffc0+i*4,0);pc=0x6000;
 }
};
struct FactoredMatrixData {
 std::vector<std::vector<TileFixture>> pairs;std::vector<std::array<unsigned,32>> expected;
 FactoredMatrixData(unsigned width,unsigned groups,unsigned trial){
  for(unsigned pair=0;pair<groups/2;pair++){
   pairs.emplace_back();expected.emplace_back();expected.back().fill(0);
   for(unsigned block=0;block<width/256;block++){
    pairs.back().emplace_back(trial+pair*101+block);
    if(pair){pairs.back().back().input=pairs[0][block].input;pairs.back().back().integer=pairs[0][block].integer;}
    for(unsigned g=0;g<8;g++)ReferenceGroup(pairs.back().back(),g,expected.back());
   }
  }
 }
 void StageCache(CachedIO& io)const{
  const unsigned base=io.job.words[3]-io.dramBase,blocks=io.width/256;
  for(unsigned pair=0;pair<pairs.size();pair++)for(unsigned block=0;block<blocks;block++)for(unsigned row=0;row<32;row++)for(unsigned byte=0;byte<144;byte++)
   io.dram[base+((pair*2+row/16)*blocks+block)*2304+byte*16+row%16]=pairs[pair][block].original[row*144+byte];
 }
 void StageRoot(std::vector<uint8_t>& root)const{
  for(unsigned block=0;block<pairs[0].size();block++)for(unsigned k=0;k<256;k++){
   const unsigned a=CL::InputBase+(block*256+k)*2,b=pairs[0][block].input[k];root[a]=uint8_t(b);root[a+1]=uint8_t(b>>8);
  }
 }
};
int main(int argc,char** argv){
 assert(argc==2);assert(Round(0x3f808000)==0x3f800000);unsigned cases=0,words=0,maxStack=0,maxTouched=0;
 for(unsigned width:{5120U,17408U})for(unsigned groups:{2U,4U}){
  CL::Job job={{CL::Magic,1,width,0x04004000,groups,786,1,CL::Peer(1),PioneerQ38Bf16Wire::Magic,CL::InputBase,CL::InputRoot,17,CL::Output,0,0,0}};
  std::vector<uint8_t> root(0x180000,0xa7);auto native=std::make_unique<FactoredConnected>();
  auto cpu=std::make_unique<FactoredMatrixRV32>(argv[1],*native,job,root);
  for(unsigned i=0;i<16;i++)cpu->Put(0x74000+i*4,job.words[i]);
  for(unsigned pass=0;pass<2;pass++){
   FactoredMatrixData data(width,groups,pass*71);data.StageRoot(root);
   if(pass){job.words[11]++;cpu->Reenter(job);}data.StageCache(cpu->bus);
   assert(cpu->Execute(0x31465151) && cpu->Get(0x17ffc4)==groups*(width/16));
   for(unsigned pair=0;pair<groups/2;pair++)for(unsigned row=0;row<32;row++){
    const unsigned got=cpu->Get(CL::Output+(pair*32+row)*4);
    if(got!=data.expected[pair][row]){fprintf(stderr,"FACTORED_RV32_MISMATCH K%u rows%u pass%u pair%u row%u got%08x expected%08x\n",width,groups*16,pass,pair,row,got,data.expected[pair][row]);assert(false);}
    words++;
   }
   assert(native->Clean() && native->groups==(pass+1)*(groups/2)*(width/256)*8 && cpu->bus.overlap);
   assert(cpu->bus.issues==(pass+1)*(groups/2)*(width/256)*26 && cpu->bus.completed==cpu->bus.issues);
   assert(native->initialZeroLoads==(pass+1)*(groups/2)*9 && native->scopeClears==native->groups*2);
   assert(native->outputReads==cpu->stores && cpu->stores==(pass+1)*groups*16);
   cases++;
  }
  maxStack=std::max(maxStack,0x70000-cpu->minSP);maxTouched=std::max(maxTouched,0x70000-cpu->minStore);
 }
 assert(cases==8 && words==384);
 printf("PASS_FACTORED_Q4_RETURNING_RV32 cases%u outputs%u fullK5120_17408 rows32_64 repeatedJobsLiveContext ORIGINAL_cache_ROOT_BF16_NoC_L1_PACK_matrix_metadata_SFPU_affine;unchangedStrictStep_noScalarLearnedInputOrSum;SP%u touched%u;NOT_NATIVE_OWNER_HARDWARE_MODEL_QUALITY_OR_TPS\n",cases,words,maxStack,maxTouched);
}
