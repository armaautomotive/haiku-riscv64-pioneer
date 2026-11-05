/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#define PIONEER_Q38_COMPACT_FPU_PIPELINE_MACHINE_ONLY
#include "pioneer_q38_q4_fpu_compact_pipeline_test.cpp"
#include <string>
// Actual compiled image;the strict original RV32I decoder is unchanged.
// Scalar learned payload reads/writes and all NoC/GDDR/host access refuse.
struct CompactFpuRV32:Machine {
 CompactFpuPipelineMachine native;std::vector<uint8_t> code;
 unsigned minSP=0x70000,minStore=0x70000,stores=0,acks=0,dropAckAt=0,portWrites=0,directIssues=0;
 unsigned dropConfig=~0U;bool awaiting=false,stackSeen=false;
 explicit CompactFpuRV32(const char* folder){
  const std::string root=folder;code=Read((root+"/text.bin").c_str());const auto boot=Read((root+"/boot.bin").c_str());
  assert(boot.size()==4 && code.size()==4948 && !(code.size()&3));
  std::fill(std::begin(l1),std::end(l1),0xdeadbeef);l1[0]=Word(boot,0);
  for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
  for(unsigned i=0;i<3;i++)l1[Index(0x17ffc0+i*4)]=0;
 }
 static bool Range(unsigned a,unsigned first,unsigned bytes){return a>=first && a-first<bytes;}
 unsigned Index(unsigned a)override{
  assert(!(a&3));if(a==0x6000)return 0;
  if(Range(a,0x70000,unsigned(code.size())))return 1+(a-0x70000)/4;
  if(Range(a,0x6e000,8192))return 2049+(a-0x6e000)/4;
  if(Range(a,0x17ffc0,12))return 4097+(a-0x17ffc0)/4;
  assert(Range(a,0x98000,256));return 4100+(a-0x98000)/4;
 }
 bool CodeAddress(unsigned a)const override{return a==0x6000 || Range(a,0x70000,unsigned(code.size()));}
 bool InlineTensix()const override{return true;}
 unsigned StepLimit()const override{return 10000000;}
 void Instruction(unsigned op)override{
  if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
  else if(op==0x4500017e){assert(awaiting && !gpr63);acks++;if(acks!=dropAckAt){native.Drain();gpr63=1;awaiting=false;}}
  else{assert(!awaiting);directIssues++;native.Op(op);}
 }
 void Store(unsigned a,unsigned value)override{
  assert(!(a&3));
  if(a==0xffe40000){portWrites++;Instruction(value);return;}
  if(a==0xffe000fc){assert(!value);gpr63=0;return;}
  if(Range(a,0xffef0000,224*4)){const unsigned i=(a-0xffef0000)/4;if(i!=dropConfig)native.Config(i,value);return;}
  assert(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,0x98000,256));
  if(Range(a,0x6e000,8192))minStore=std::min(minStore,a);
  if(Range(a,0x98000,256)){assert(native.Clean() && !awaiting);stores++;}l1[Index(a)]=value;
 }
 unsigned Load(unsigned a)override{
  assert(!(a&3));if(a==0xffe000fc)return gpr63;
  if(Range(a,0xffef0000,224*4)){assert(!native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4];}
  if(Range(a,0xffbd8800,64) || Range(a,0xffbd8a00,64)){
   if(!native.Clean() || awaiting)throw CompactFpuPipelineMachine::Fault{};
   assert(!native.config[3]);const unsigned row=(a-0xffbd8000)/64,col=(a%64)/4;assert(row==32 || row==40);
   native.outputReads++;return native.FloatDst(row,col);
  }
  assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x17ffc0,12));return l1[Index(a)];
 }
 bool Execute(){
  while(pc!=0x70038 && pc!=0x70058){Step();if(regs[2])stackSeen=true;if(stackSeen){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}}
  assert(l1[Index(pc)]==0x0000006f && minStore>=0x6e000);
  if(pc==0x70058){assert(l1[Index(0x17ffc8)]==0x45434651);return false;}
  assert(l1[Index(0x17ffc0)]==0x7000c && l1[Index(0x17ffc4)]==0x04000100 && l1[Index(0x17ffc8)]==0x31434651);
  assert(native.Clean() && stores==64 && native.outputReads==64 && native.matrixOps==256 && native.rawLoads==64
   && native.compactImports==4 && native.compactStores==144 && native.extraZeroLoads==14 && native.metadataLoads==0 && native.inputLoads==16);
  for(unsigned phase=0;phase<4;phase++)assert(native.visits[phase]==64);return true;
 }
 bool Matches(const std::array<unsigned,32>& expected){
  for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)if(l1[Index(0x98000+pass*128+row*4)]!=expected[row])return false;
  return true;
 }
};
#ifndef PIONEER_Q38_COMPACT_FPU_IMAGE_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2);unsigned cases=0,maxStack=0,maxTouch=0,acks=0;
 for(unsigned trial=0;trial<8;trial++){
  Q4PipelineFixture data(trial);std::array<uint8_t,4608> compact;
  assert(PioneerQ38Q4FpuCompactData::Prepare(data.original.data(),data.original.size(),compact.data(),compact.size()));
  CompactFpuRV32 image(argv[1]);image.native.StageCompact(compact,data.input);image.native.delay[0]=100003;image.native.delay[1]=7;
  assert(image.Execute() && image.Matches(data.expected));cases++;acks=image.acks;
  maxStack=std::max(maxStack,0x70000-image.minSP);maxTouch=std::max(maxTouch,0x70000-image.minStore);
  if(!trial)for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)
   printf("Q4_FPU_PIPELINE_REFERENCE trial0 pass%u row%u word%08x\n",pass,row,image.l1[image.Index(0x98000+pass*128+row*4)]);
 }
 Q4PipelineFixture data(0);std::array<uint8_t,4608> compact;
 assert(PioneerQ38Q4FpuCompactData::Prepare(data.original.data(),data.original.size(),compact.data(),compact.size()));
 unsigned refusals=0;
 for(unsigned fault=0;fault<7;fault++){
  CompactFpuRV32 bad(argv[1]);bad.native.StageCompact(compact,data.input);bool failed=false;
  if(fault==0)bad.dropConfig=72;
  if(fault==1)bad.dropAckAt=1;if(fault==2)bad.dropAckAt=acks/2;
  if(fault==3)bad.native.skipExtraZero=true;if(fault==4)bad.native.skipMove=1;
  if(fault==5)bad.native.dropProducer=1;if(fault==6)bad.native.dropProducer=2;
  try{failed=!bad.Execute();}catch(CompactFpuPipelineMachine::Fault){failed=true;}catch(unsigned marker){failed=marker==NativeBfpMachine::Unknown;}
  assert(failed && !bad.stores && !bad.native.outputReads);refusals++;
 }
 unsigned numerical=0;
 for(unsigned fault=0;fault<8;fault++){
  CompactFpuRV32 bad(argv[1]);bad.native.StageCompact(compact,data.input);
  if(fault<4)bad.native.dropCompactStore=fault==0?1:fault==1?2:fault==2?71:72;else bad.native.dropPhase=fault-4;
  assert(bad.Execute() && !bad.Matches(data.expected));numerical++;
 }
 printf("PASS_COMPACT_ACTUAL_RV32 cases%u_twoJobs_each64WordsExact controls%u numerical%u original8KiBROM_8KiBStack_bounds SP%u touched%u;SAME_RAW_BYTES_UNPACK_SFPU_BF16_MOVD2A_4PHASE_FPU_NO_SCALAR_PAYLOAD;NO_NATIVE_MODEL_QUALITY_OR_SPEED\n",cases,refusals,numerical,maxStack,maxTouch);
}
#endif
