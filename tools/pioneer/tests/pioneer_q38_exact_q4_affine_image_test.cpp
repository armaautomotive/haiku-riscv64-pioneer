/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_exact_q4_affine_machine.h"
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include <string>
// Execute the actual new RV32I image through the UNCHANGED Step decoder.
// All scalar payload reads/copies,MMIO commands,wait polls,config readback,
// output publications and BOTH jobs are observed. No semantic Main call.
struct ExactAffineImage:Machine {
 ExactAffineMachine native;std::vector<uint8_t> code;
 unsigned width,minSP=0x70000,results=0,acks=0,sourceReads=0,maskReads=0;
 bool awaiting=false,stackSeen=false,dropAck=false,dropMatrixDrain=false;
 bool dropAffineRecord=false;
 unsigned dropConfig=~0U;bool change=true;unsigned nextSeed=176;
 std::array<uint32_t,16> expected[2];
 ExactAffineImage(const char* directory,unsigned k,bool twoProduct=false,bool registerReplay=false):Machine(),width(k){
  native.twoProducts=twoProduct;
  native.allowRegisterReplay=registerReplay;if(registerReplay)native.replay.Poison();
  const std::string root=directory;code=Read((root+"/text.bin").c_str());auto boot=Read((root+"/boot.bin").c_str());
  assert(boot.size()==4 && code.size()<=8192 && !(code.size()&3));
  std::fill(std::begin(l1),std::end(l1),0x7fc00001U);l1[0]=Word(boot,0);
  for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
  expected[0]=ExactAffineFixture(native,k,37);expected[1]=expected[0];
 }
 static bool Range(unsigned a,unsigned first,unsigned bytes){return a>=first && a-first<bytes;}
 virtual void NextFixture(){if(native.allowRegisterReplay)native.replay.Poison();expected[1]=ExactAffineFixture(native,width,nextSeed);}
 virtual unsigned ExpectedSourceReads()const{return 2*width/16*138;}
 unsigned Index(unsigned a)override{
  assert(!(a&3));
  if(a==0x6000)return 0;
  if(Range(a,0x70000,unsigned(code.size())))return 1+(a-0x70000)/4;
  if(Range(a,0x6e000,8192))return 2049+(a-0x6e000)/4;
  if(Range(a,0x17ffc0,12))return 4097+(a-0x17ffc0)/4;
  if(Range(a,0x92000,256))return 4100+(a-0x92000)/4;
  assert(false && "full-K scalar address outside distinct private prototype");return 0;
 }
 bool CodeAddress(unsigned a)const override{return a==0x6000 || Range(a,0x70000,unsigned(code.size()));}
 unsigned StepLimit()const override{return 80000000;}
 void Instruction(unsigned op)override{
  if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
  else if(op==0x4500017e){
   assert(awaiting && !gpr63);acks++;
   if(!dropAck && !(dropMatrixDrain && native.mathPending && native.matrixOps==(native.twoProducts?2U:3U))){
    native.Drain();gpr63=1;awaiting=false;
   }
  }else if(!(dropAffineRecord && op==0x040300d3))native.Op(op);
 }
 bool DataWrite(unsigned a)const{
  return Range(a,0x90000,128) || Range(a,0x90100,128) || Range(a,0x90200,64) || Range(a,0x90300,32)
   || Range(a,0x91000,256) || Range(a,0x93ff0,80) || Range(a,0x94070,80) || Range(a,0x95ff0,1040);
 }
 void Store(unsigned a,unsigned v)override{
  assert(!(a&3));
  if(a==0xffe40000){Instruction(v);return;}
  if(a==0xffe000fc){assert(!v);gpr63=0;return;}
  if(Range(a,0xffef0000,224*4)){const unsigned i=(a-0xffef0000)/4;if(i!=dropConfig)native.Config(i,v);return;}
  if(DataWrite(a)){
   if(Range(a,0x90100,128))native.Seven((a-0x90100)/4,v);
   else if(Range(a,0x90200,64))native.Escape((a-0x90200)/4,v);
   else {if(!native.Clean())throw native.Unknown;native.Write(a,v);}return;
  }
  assert(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,0x92000,128));
  if(Range(a,0x92000,128)){
   assert(native.Clean());results++;
   if(change && results==16 && a==0x9203c){NextFixture();change=false;}
  }
  l1[Index(a)]=v;
 }
 unsigned Load(unsigned a)override{
  assert(!(a&3));
  if(a==0xffe000fc)return gpr63;
  if(Range(a,0xffef0000,224*4)){assert(!native.Pending() && !native.mathPending && !native.sfpuPending);return native.config[(a-0xffef0000)/4];}
  if(Range(a,0xffbd8000+108*64,64)){assert(native.Clean() && !native.config[3]);native.outputReads++;return native.FloatDst(108,(a%64)/4);}
  if(Range(a,0x90300,32)){maskReads++;return native.Mask((a-0x90300)/4);}
  if(Range(a,PioneerQ38ExactQ4Affine::Source,width/16*352)){
   if(!native.Clean())throw native.Unknown;
   assert((a-PioneerQ38ExactQ4Affine::Source)%352<328);sourceReads++;return native.Read(a);
  }
  assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x17ffc0,12));return l1[Index(a)];
 }
 bool Execute(){
  while(pc!=0x70038 && pc!=0x70058){
   const unsigned instruction=l1[Index(pc)];if((instruction&127)==15)native.ObserveFence();
   Step();if(regs[2])stackSeen=true;
   if(stackSeen){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}
  }
  assert(l1[Index(pc)]==0x0000006f);
  if(pc==0x70058){assert(l1[Index(0x17ffc8)]==0x46413451 && !results);return false;}
  assert(l1[Index(0x17ffc0)]==0x7000c && l1[Index(0x17ffc4)]==2 && l1[Index(0x17ffc8)]==0x31413451);
  assert(native.Clean() && results==32 && native.outputReads==32);
  for(unsigned pass=0;pass<2;pass++)for(unsigned n=0;n<16;n++)assert(l1[Index(0x92000+pass*64+n*4)]==expected[pass][n]);
  for(unsigned n=32;n<64;n++)assert(l1[Index(0x92000+n*4)]==0x7fc00001);
  assert(native.matrixOps==(native.twoProducts?4U:6U)*width/16 && native.metadataLoads==2*width/16 && native.zeroLoads==2
   && native.affineStores==4*width/16 && sourceReads==ExpectedSourceReads() && maskReads==2*width/16*8
   && !native.unpackBank[0] && !native.unpackBank[1] && !native.matrixBank[0] && !native.matrixBank[1]);
  if(native.twoProducts)assert(!native.sevenWrites && native.unpacks[0]==4*width/16);
  if(native.allowRegisterReplay)assert(native.replay.records==2 && native.replay.replays==4*width/16-2 && native.replay.expanded==4*width/16*13);
  return true;
 }
};
#ifndef PIONEER_Q38_EXACT_Q4_AFFINE_IMAGE_ONLY
int main(int argc,char** argv){
 assert(argc==4);unsigned cases=0,maxStack=0,maxSteps=0;
 for(unsigned i=0;i<3;i++)for(unsigned schedule=0;schedule<2;schedule++){
  const unsigned width=i==0?5120:i==1?6144:17408;ExactAffineImage image(argv[i+1],width);
  image.native.delay[0]=schedule?1:100003;image.native.delay[1]=schedule?100003:1;
  assert(image.Execute());maxStack=std::max(maxStack,0x70000-image.minSP);maxSteps=std::max(maxSteps,image.steps);cases++;
 }
 for(unsigned fault=0;fault<3;fault++){
  ExactAffineImage image(argv[1],5120);if(fault==0)image.dropConfig=50;if(fault==1)image.dropAck=true;if(fault==2)image.dropMatrixDrain=true;
  assert(!image.Execute() && !image.native.outputReads && !image.results);
  if(fault==2)assert(image.native.matrixOps==3 && image.native.mathPending && !image.native.affineStores);
  else assert(!image.native.matrixOps);cases++;
 }
 for(unsigned producer:{1U,2U}){
  ExactAffineImage image(argv[1],5120);image.native.dropProducer=producer;bool refused=false;
  try{image.Execute();}catch(uint32_t marker){refused=marker==image.native.Unknown;}
  assert(refused && !image.native.outputReads && !image.results);cases++;
 }
 {
  ExactAffineImage image(argv[1],5120);image.native.Write(PioneerQ38ExactQ4Affine::Source+3*352+320,0x7f800000);image.native.ObserveFence();
  assert(!image.Execute() && image.native.matrixOps==9 && image.native.affineStores==6 && !image.results);cases++;
 }
 printf("PASS_ACTUAL_RV32_ORIGINAL_Q4_FULLK_AFFINE cases%u widths5120_6144_17408 dualJobs_changedSource;"
  "originalFP32Outputs_and_untouchedPadding exact;noScalarLearnedDot_orFPinstructions;stack%u/8192 maxRVSteps%u;"
  "NOT_NATIVE_HARDWARE_MODEL_OR_TPS\n",cases,maxStack,maxSteps);
}
#endif
