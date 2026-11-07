/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#define PIONEER_NATIVE_DRAM_LOADER_TEST_ONLY
#include "pioneer_native_dram_band_loader_test.cpp"
#define PIONEER_NATIVE_BFP4_MACHINE_ONLY
#include "pioneer_q38_native_bfp4_band_numeric_test.cpp"
#include "pioneer_blackhole_arithmetic_observer.h"
#include "../PioneerNativeBfp4DramBand.h"
#include "../PioneerNativeBfp4BandGolden.h"
#include "../PioneerNativeBfp4BandTiming.h"
#include <string>

// Actual linked RV32, NoC producer and unpack/math share the SAME L1. The
// only timing source here is a deliberately synthetic CSR, not a card clock.
struct Dram4Image:Machine {
 using D=PioneerNativeBfp4DramBandLayout;
 Band4Machine native;LoaderIO<D> transfer;std::vector<uint8_t> code;
 unsigned minSP=0x70000,minStore=0x70000,stores=0,acks=0,dropAckAt=0,csrReads=0;
 bool awaiting=false,stackSeen=false,changeFrame=false;
 Dram4Image(const char* folder,unsigned width,unsigned route=17U|(12U<<6))
  :native(width,D::Weight,D::Input,D::WeightStride,D::InputStride,D::Header,D::Output),transfer(width,route,&native.l1){
  assert(&native.l1==&transfer.l1);
  std::string root=folder;code=Read((root+"/text.bin").c_str());auto boot=Read((root+"/boot.bin").c_str());
  assert(boot.size()==4 && !code.empty() && code.size()<=8192 && !(code.size()&3));
  std::fill(std::begin(l1),std::end(l1),0xdeadbeef);l1[0]=Word(boot,0);
  for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
  for(unsigned i=0;i<3;i++)l1[Index(0x17ffc0+i*4)]=0;
  unsigned frame[8]={0x34534442,1,width,route,1,0,0,0};for(unsigned i=0;i<8;i++)l1[Index(0x74000+i*4)]=frame[i];
  Fixture fixture(width);
  for(unsigned tile=0;tile<native.tiles;tile++){
   memcpy(transfer.dram.data()+tile*576,fixture.packed[tile].data(),576);
   for(unsigned c=0;c<32;c++){
    unsigned at=D::DramInput-D::DramWeight+tile*64+c*2;
    transfer.dram[at]=uint8_t(fixture.input[tile][c]);transfer.dram[at+1]=uint8_t(fixture.input[tile][c]>>8);
   }
  }
 }
 static bool Range(uint32_t a,uint32_t first,unsigned bytes){return a>=first && a-first<bytes;}
 unsigned Index(uint32_t a)override{
  assert(!(a&3));if(a==0x6000)return 0;
  if(Range(a,0x70000,unsigned(code.size())))return 1+(a-0x70000)/4;
  if(Range(a,0x6e000,8192))return 2049+(a-0x6e000)/4;
  if(Range(a,0x74000,32))return 4097+(a-0x74000)/4;
  if(Range(a,0x17ffc0,12))return 4105+(a-0x17ffc0)/4;
  assert(Range(a,D::Output,256));return 4108+(a-D::Output)/4;
 }
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,0x70000,unsigned(code.size()));}
 unsigned StepLimit()const override{return 10000000+PioneerDramQueuedReadSweep::PollLimit*2048;}
 uint32_t ReadCycleCsr(unsigned csr)override{
  assert(csr==0xb00 || csr==0xb80);csrReads++;uint64_t clock=UINT64_C(0xffff0000)+uint64_t(steps)*37;
  return csr==0xb00?uint32_t(clock):uint32_t(clock>>32);
 }
 bool Loaded()const{return transfer.completions>=2*native.tiles && !(transfer.completions%(2*native.tiles)) && transfer.pending.empty();}
 void Instruction(uint32_t op)override{
  assert(Loaded());
  if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
  else if(op==0x4500017e){assert(awaiting && !gpr63);acks++;if(acks!=dropAckAt){native.Drain();gpr63=1;awaiting=false;}}
  else if((op>>24)==0x26)PioneerBlackholeArithmeticObserver::Matrix(native,op,true,[&]{native.Op(op);});
  else native.Op(op);
 }
 void Store(uint32_t a,uint32_t v)override{
  assert(!(a&3));if(a==0xffe40000){Instruction(v);return;}
  if(a==0xffe000fc){assert(!v && Loaded());gpr63=0;return;}
  if(Range(a,0xffef0000,224*4)){assert(Loaded());native.Config((a-0xffef0000)/4,v);return;}
  if(transfer.Header(a)||Range(a,0xffb20000,0x2000)){transfer.Write(a,v);return;}
  assert(Range(a,0x6e000,8192)||Range(a,0x17ffc0,12)||Range(a,D::Output,256));
  if(Range(a,0x6e000,8192) && a>=0x6e040)minStore=std::min(minStore,unsigned(a));
  if(Range(a,D::Output,256)){assert(native.Clean() && Loaded());stores++;}l1[Index(a)]=v;
 }
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(a==0xffe000fc)return gpr63;
  if(Range(a,0xffef0000,224*4)){assert(Loaded() && !native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4];}
  if(Range(a,0xffbd8800,64)||Range(a,0xffbd8a00,64)){
   assert(native.Clean() && Loaded());unsigned row=(a-0xffbd8000)/64,col=(a%64)/4;
   assert(native.valid[row] && (row==32 || row==40));native.outputReads++;return Bits(native.destination[row][col]);
  }
  if(transfer.Header(a)||Range(a,0xffb20000,0x2000)){
   auto result=transfer.Read(a);if(changeFrame && Loaded())l1[Index(0x74010)]=2;return result;
  }
  // No scalar weight/input/DRAM/host/reference access.
  assert(CodeAddress(a)||Range(a,0x6e000,8192)||Range(a,0x74000,32)||Range(a,0x17ffc0,12));return l1[Index(a)];
 }
 bool Execute(){
  while(pc!=0x70038 && pc!=0x70058){Step();if(regs[2])stackSeen=true;if(stackSeen){assert(regs[2]>=0x6e040 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}}
  assert(l1[Index(pc)]==0x0000006f && minStore>=0x6e040);
  if(pc==0x70058){assert(l1[Index(0x17ffc8)]==0x4542444e);return false;}
  assert(l1[Index(0x17ffc0)]==0x7000c && l1[Index(0x17ffc4)]==(0x02000000|native.tiles*32) && l1[Index(0x17ffc8)]==0x34534442);
  assert(Loaded() && native.Clean() && stores==64 && native.outputReads==64 && native.clears==2 && csrReads==12);
  assert(native.unpacks[0]==2*native.tiles && native.unpacks[1]==native.unpacks[0] && native.matrixOps==16*native.tiles);
  assert(native.visits[0]==8*native.tiles && native.visits[2]==8*native.tiles && !native.visits[1] && !native.visits[3]);
  transfer.Exact(2);
  unsigned timing[16];for(unsigned i=0;i<16;i++)timing[i]=l1[Index(0x6e000+i*4)];
  assert(PioneerNativeBfp4BandTiming::State(timing,native.tiles*32,0)==1 && PioneerNativeBfp4BandTiming::State(timing+8,native.tiles*32,1)==1);
  assert(PioneerNativeBfp4BandTiming::Later(timing+12,timing+6));return true;
 }
};
#ifndef PIONEER_BFP4_DRAM_IMAGE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || argc==4);unsigned width=unsigned(std::stoul(argv[2]));Dram4Image image(argv[1],width);
 std::array<uint32_t,32> privateExpected={};
 if(argc==4){
  // The optional private data must come from the separately SHA-authenticated
  // same-open real-model exporter. The image may NOT scalar-read this buffer.
  auto data=Read(argv[3]);assert(width==5120 && data.size()==102544 && Word(data,0)==0x34504252 && Word(data,4)==1 && Word(data,8)==width);
  unsigned first=Word(data,12);assert(first==0 || first==8704 || first==17376);
  memcpy(image.transfer.dram.data(),data.data()+16,160*576);
  memcpy(image.transfer.dram.data()+Dram4Image::D::DramInput-Dram4Image::D::DramWeight,data.data()+16+160*576,5120*2);
  for(unsigned row=0;row<32;row++)privateExpected[row]=Word(data,16+160*576+5120*2+row*4);
 }
 assert(image.Execute());
 const auto* expected=argc==4?privateExpected.data():PioneerNativeBfp4BandGolden(width);assert(expected);
 for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)assert(image.l1[image.Index(Dram4Image::D::Output+pass*128+row*4)]==expected[row]);
 printf("PASS actual OFFBOARD BFP4 GDDR RV32 K%u %s: two independent loads, 64 rounded outputs, same NoC/unpack L1, synthetic clocks, stack%u, steps%u; NO physical/model/speed claim\n",width,argc==4?"EXTERNAL_SAMPLE":"SYNTHETIC_WEIGHTS",0x70000-image.minSP,image.steps);
 if(width==5120 && argc==3){
  unsigned failures=0;
  for(unsigned word=0;word<8;word++){
   Dram4Image bad(argv[1],width);bad.l1[bad.Index(0x74000+word*4)]^=word==4?1:0x80000000;
   assert(!bad.Execute() && !bad.transfer.issues && !bad.native.clears && !bad.stores);failures++;
  }
  for(unsigned fault:{1U,5U,6U,7U,9U,10U,11U,12U}){
   Dram4Image bad(argv[1],width);bad.transfer.fault=fault;assert(!bad.Execute() && !bad.native.clears && !bad.stores && bad.transfer.issues<=16);failures++;
  }
  {Dram4Image bad(argv[1],width);bad.changeFrame=true;assert(!bad.Execute() && bad.transfer.completions==2*bad.native.tiles && bad.stores==32);failures++;}
  {Dram4Image bad(argv[1],width);bad.dropAckAt=1;assert(!bad.Execute() && bad.Loaded() && !bad.stores);failures++;}
  printf("PASS actual OFFBOARD BFP4 GDDR RV32 %u frame/NIU/counter/header/completion fault controls, no retry; NO physical authority\n",failures);
 }
}
#endif
