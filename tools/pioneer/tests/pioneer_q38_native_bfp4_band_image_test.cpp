/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#define PIONEER_NATIVE_BFP4_MACHINE_ONLY
#include "pioneer_q38_native_bfp4_band_numeric_test.cpp"
#include <string>

// Strictly interpret the actual linked RV32 image and its Tensix instructions.
// Uses an ideal arithmetic observer: no silicon rounding or device claim.
struct Band4RV32:Machine {
 Band4Machine native;std::vector<uint8_t> code;bool direct,awaiting=false,stackSeen=false;
 unsigned minSP=0x70000,minStore=0x70000,stores=0,acks=0,dropAckAt=0,portWrites=0,dropConfig=~0U;
 Band4RV32(const char* folder,unsigned width,bool mode):native(width),direct(mode){
  std::string root=folder;code=Read((root+"/text.bin").c_str());auto boot=Read((root+"/boot.bin").c_str());
  assert(boot.size()==4 && !code.empty() && code.size()<=4096 && !(code.size()&3));
  std::fill(std::begin(l1),std::end(l1),0xdeadbeef);l1[0]=Word(boot,0);
  for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
  for(unsigned i=0;i<3;i++)l1[Index(0x17ffc0+i*4)]=0;
 }
 static bool Range(uint32_t a,uint32_t first,unsigned size){return a>=first && a-first<size;}
 unsigned Index(uint32_t a) override{
  assert(!(a&3));if(a==0x6000)return 0;
  if(Range(a,0x70000,unsigned(code.size())))return 1+(a-0x70000)/4;
  if(Range(a,0x6e000,8192))return 1025+(a-0x6e000)/4;
  if(Range(a,0x17ffc0,12))return 3073+(a-0x17ffc0)/4;
  assert(Range(a,0x130000,256));return 3076+(a-0x130000)/4;
 }
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,0x70000,unsigned(code.size()));}
 bool InlineTensix()const override{return direct;}
 unsigned StepLimit()const override{return 10000000;}
 void Instruction(uint32_t op)override{
  if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
  else if(op==0x4500017e){assert(awaiting && !gpr63);acks++;if(acks!=dropAckAt){native.Drain();gpr63=1;awaiting=false;}}
  else native.Op(op);
 }
 void Store(uint32_t a,uint32_t v)override{
  assert(!(a&3));if(a==0xffe40000){portWrites++;Instruction(v);return;}
  if(a==0xffe000fc){assert(!v);gpr63=0;return;}
  if(Range(a,0xffef0000,224*4)){unsigned w=(a-0xffef0000)/4;if(w!=dropConfig)native.Config(w,v);return;}
  assert(Range(a,0x6e000,8192)||Range(a,0x17ffc0,12)||Range(a,0x130000,256));
  if(Range(a,0x6e000,8192))minStore=std::min(minStore,unsigned(a));
  if(Range(a,0x130000,256)){assert(native.Clean());stores++;}l1[Index(a)]=v;
 }
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(a==0xffe000fc)return gpr63;
  if(Range(a,0xffef0000,224*4)){assert(!native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4];}
  if(Range(a,0xffbd8800,64)||Range(a,0xffbd8a00,64)){
   if(!native.Clean())throw NativeBfpMachine::Unknown;
   assert(!native.config[3]);unsigned row=(a-0xffbd8000)/64,col=(a%64)/4;
   assert(native.valid[row] && (row==32 || row==40));native.outputReads++;return Bits(native.destination[row][col]);
  }
  assert(CodeAddress(a)||Range(a,0x6e000,8192)||Range(a,0x17ffc0,12));return l1[Index(a)];
 }
 bool Execute(){
  while(pc!=0x70038 && pc!=0x70058){Step();if(regs[2])stackSeen=true;if(stackSeen){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}}
  assert(l1[Index(pc)]==0x0000006f && minStore>=0x6e000);
  if(pc==0x70058){assert(l1[Index(0x17ffc8)]==0x4542464e && !stores && !native.outputReads);return false;}
  assert(l1[Index(0x17ffc0)]==0x7000c && l1[Index(0x17ffc4)]==((2U<<24)|(native.tiles*32)) && l1[Index(0x17ffc8)]==0x3442464e);
  assert(native.Clean() && stores==64 && native.outputReads==64 && native.clears==2);
  assert(native.unpacks[0]==2*native.tiles && native.unpacks[1]==native.unpacks[0] && native.matrixOps==16*native.tiles);
  assert(native.visits[0]==8*native.tiles && native.visits[2]==8*native.tiles && !native.visits[1] && !native.visits[3]);return true;
 }
 bool Matches(const std::array<float,32>& expected){
  for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)if(l1[Index(0x130000+pass*128+row*4)]!=Bits(expected[row]))return false;
  return true;
 }
};
int main(int argc,char** argv){
 assert(argc==4);unsigned width=unsigned(std::stoul(argv[2]));
 assert(!strcmp(argv[3],"mmio") || !strcmp(argv[3],"direct"));bool direct=!strcmp(argv[3],"direct");
 Fixture f(width);Band4RV32 image(argv[1],width,direct);f.Stage(image.native);image.native.delay[0]=100003;image.native.delay[1]=7;
 assert(image.Execute() && image.Matches(f.expected));
 if(direct)assert(image.portWrites<256);else assert(image.portWrites>width/32*8);
 printf("PASS linked OFFBOARD BFP4 RV32 K%u %s: two launches, exact 32 ideal outputs each, clean banks, stack %u, steps %u; NO silicon/model/speed proof\n",width,argv[3],0x70000-image.minSP,image.steps);
 if(width==5120 && !direct){
  for(unsigned fault=0;fault<3;fault++){
   Band4RV32 bad(argv[1],width,false);f.Stage(bad.native);
   if(fault==0)bad.dropConfig=72;if(fault==1)bad.dropAckAt=1;if(fault==2)bad.dropAckAt=4+2*bad.native.tiles;
   assert(!bad.Execute() && !bad.stores && !bad.native.outputReads);
  }
  for(unsigned missing:{1U,2U}){
   Band4RV32 bad(argv[1],width,false);f.Stage(bad.native);bad.native.dropProducer=missing;bool refused=false;
   try{bad.Execute();}catch(uint32_t marker){refused=marker==NativeBfpMachine::Unknown;}
   assert(refused && !bad.stores && !bad.native.outputReads);
  }
  for(unsigned phase:{0U,2U}){
   Band4RV32 bad(argv[1],width,false);f.Stage(bad.native);bad.native.dropPhase=phase;
   assert(bad.Execute() && !bad.Matches(f.expected));
  }
  puts("PASS actual linked image fault controls: config and completion park, two missing producers refused, two omitted phases change result; OFFBOARD only");
 }
}
