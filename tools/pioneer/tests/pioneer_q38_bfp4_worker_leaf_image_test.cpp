/* SPDX-License-Identifier: MIT */
// Execute the actual linked RV32 leaf with shared simulated NoC/FPU L1.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#define PIONEER_BFP4_WORKER_LOADER_TEST_ONLY
#include "pioneer_q38_bfp4_worker_loader_test.cpp"
#ifdef PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT
#include "pioneer_q38_bfp4_bulk_io_fixture.h"
using Transfer=BulkIO;
#else
using Transfer=IO;
#endif
#define PIONEER_NATIVE_BFP4_MACHINE_ONLY
#include "pioneer_q38_native_bfp4_band_numeric_test.cpp"
#include "pioneer_blackhole_arithmetic_observer.h"
#include "../PioneerNativeBfp4BandGolden.h"
#include <string>
struct WorkerImage:Machine {
 Band4Machine native;Transfer transfer;std::vector<uint8_t> code;unsigned stores=0,acks=0,dropAckAt=0,minSP=0x70000;
 bool awaiting=false,stackSeen=false,damageFrame=false,inputPreservationChecked=false,damageInput=false,inputDamaged=false,dropAfterFirstBand=false,faultAfterFirstBand=false;
 bool damageHeader=false,damageOnMath=false;
 static W::Job MakeBands(unsigned worker,unsigned width,unsigned bands){
  auto job=Make(worker,width);job.words[4]=bands;assert(W::Shape(job));return job;
 }
 WorkerImage(const char* folder,unsigned worker,unsigned width,unsigned bands=2,bool distinct=false)
  :native(width,L::Weight,L::Input,L::WeightStride,L::InputStride,L::Header,L::Output),transfer(MakeBands(worker,width,bands),0,&native.l1){
  transfer.autoBand=true;std::string root=folder;code=Read((root+"/text.bin").c_str());auto boot=Read((root+"/boot.bin").c_str());
  assert(boot.size()==4 && !code.empty() && code.size()<=8192 && !(code.size()&3));
  std::fill(std::begin(l1),std::end(l1),0xdeadbeef);l1[0]=Word(boot,0);
  for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
  for(unsigned i=0;i<W::Words;i++)l1[Index(0x74000+i*4)]=transfer.job.words[i];
  for(unsigned i=0;i<3;i++)l1[Index(0x17ffc0+i*4)]=0;
  Fixture fixture(width);
  std::array<float,1024> zero={};std::array<uint8_t,576> zeroPacked={};
  assert(PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,zero.data(),zero.size(),zeroPacked.data(),zeroPacked.size()));
  for(unsigned tile=0;tile<native.tiles;tile++){
   for(unsigned band=0;band<bands;band++)std::memcpy(transfer.weights.data()+(band*native.tiles+tile)*576,
    distinct && (band&1)?zeroPacked.data():fixture.packed[tile].data(),576);
   for(unsigned c=0;c<32;c++){transfer.root[tile*64+c*2]=uint8_t(fixture.input[tile][c]);transfer.root[tile*64+c*2+1]=uint8_t(fixture.input[tile][c]>>8);}
  }
 }
 static bool Range(uint32_t a,uint32_t first,unsigned bytes){return a>=first && a-first<bytes;}
 unsigned Index(uint32_t a)override{
  assert(!(a&3));if(a==0x6000)return 0;
  if(Range(a,0x70000,unsigned(code.size())))return 1+(a-0x70000)/4;
  if(Range(a,0x6e000,8192))return 2049+(a-0x6e000)/4;
  if(Range(a,0x74000,64))return 4097+(a-0x74000)/4;
  if(Range(a,0x17ffc0,12))return 4113+(a-0x17ffc0)/4;
  assert(Range(a,W::Output,8*32*4));return 4116+(a-W::Output)/4;
 }
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,0x70000,unsigned(code.size()));}
 unsigned StepLimit()const override{return 10000000+PioneerDramQueuedReadSweep::PollLimit*2048;}
 bool Loaded()const{return transfer.completions>=
#ifdef PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT
  2*native.tiles/16 && !(transfer.completions%(2*native.tiles/16))
#else
  2*native.tiles
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT
  && !((transfer.completions-2*native.tiles)%native.tiles)
#else
  && !(transfer.completions%(2*native.tiles))
#endif
#endif
  && transfer.pending.empty();}
 void Instruction(uint32_t op)override{
  assert(Loaded());
  if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
  else if(op==0x4500017e){assert(awaiting && !gpr63);acks++;if(acks!=dropAckAt){native.Drain();gpr63=1;awaiting=false;
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT
   if(damageOnMath && native.matrixOps && !inputDamaged){transfer.l1[L::Input+1]^=1;inputDamaged=true;}
#endif
  }}
  else if((op>>24)==0x26)PioneerBlackholeArithmeticObserver::Matrix(native,op,true,[&]{native.Op(op);});
  else native.Op(op);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));if(a==0xffe40000){Instruction(value);return;}
  if(a==0xffe000fc){assert(!value && Loaded());gpr63=0;return;}
  if(Range(a,0xffef0000,224*4)){assert(Loaded());native.Config((a-0xffef0000)/4,value);return;}
  if(transfer.Header(a) || Range(a,0xffb20000,0x2000)
#ifdef PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT
   || transfer.Local(a)
#endif
  ){transfer.Write(a,value);return;}
  assert(Range(a,0x6e000,8192)||Range(a,0x17ffc0,12)||Range(a,W::Output,8*32*4));
  if(Range(a,W::Output,8*32*4)){assert(native.Clean() && Loaded());stores++;}
  l1[Index(a)]=value;
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT
  if(stores==32 && !inputPreservationChecked){ // First compute must preserve every staged input byte.
   for(unsigned tile=0;tile<native.tiles;tile++){
    const unsigned at=L::Input+tile*L::InputStride;
    for(unsigned i=0;i<L::Header;i++)assert(transfer.l1[at-L::Header+i]==0);
    assert(!std::memcmp(transfer.l1.data()+at,transfer.root.data()+tile*L::InputPayload,L::InputPayload));
   }
   inputPreservationChecked=true;
  }
  if(stores==32 && damageInput && !inputDamaged){transfer.l1[L::Input+1]^=1;inputDamaged=true;}
  if(stores==32 && damageHeader && !inputDamaged){transfer.l1[L::Input-L::Header]^=1;inputDamaged=true;}
  if(stores==32 && dropAfterFirstBand){dropAckAt=acks+1;dropAfterFirstBand=false;}
  if(stores==32 && faultAfterFirstBand){transfer.fault=5;faultAfterFirstBand=false;}
#endif
  if(damageFrame && stores==32)l1[Index(0x74000+11*4)]^=1;
 }
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(a==0xffe000fc)return gpr63;
  if(Range(a,0xffef0000,224*4)){assert(Loaded() && !native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4];}
  if(Range(a,0xffbd8800,64)||Range(a,0xffbd8a00,64)){
   assert(native.Clean() && Loaded());unsigned row=(a-0xffbd8000)/64,col=(a%64)/4;
   assert(native.valid[row] && (row==32 || row==40));native.outputReads++;return Bits(native.destination[row][col]);
  }
  if(transfer.Header(a)||transfer.InputScratch(a)||Range(a,0xffb20000,0x2000)
#ifdef PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT
   || transfer.Local(a)
#endif
  )return transfer.Read(a);
  assert(CodeAddress(a)||Range(a,0x6e000,8192)||Range(a,0x74000,64)||Range(a,0x17ffc0,12)||Range(a,W::Output,8*32*4));
  return l1[Index(a)];
 }
 bool Execute(){
  while(pc!=0x70038 && pc!=0x70058){Step();if(regs[2])stackSeen=true;if(stackSeen){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}}
  assert(l1[Index(pc)]==0x0000006f);
  if(pc==0x70058){assert(l1[Index(0x17ffc8)]==0x4542444e);return false;}
  const unsigned bands=transfer.job.words[4];
  assert(l1[Index(0x17ffc0)]==0x7000c && l1[Index(0x17ffc4)]==bands*32 && l1[Index(0x17ffc8)]==0x34534442);
  assert(Loaded() && native.Clean() && stores==bands*32 && native.outputReads==bands*32 && native.clears==bands && !transfer.scalar);
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT
  assert(inputPreservationChecked && transfer.issues==(bands+1)*native.tiles && transfer.completions==transfer.issues);
#elif defined(PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT)
  assert(transfer.issues==2*bands*native.tiles/16 && transfer.completions==transfer.issues);
#else
  assert(transfer.issues==2*bands*native.tiles && transfer.completions==transfer.issues);
#endif
  assert(native.unpacks[0]==bands*native.tiles && native.unpacks[1]==native.unpacks[0] && native.matrixOps==8*bands*native.tiles);
  return true;
 }
};
#ifndef PIONEER_BFP4_WORKER_IMAGE_ONLY
int main(int argc,char** argv){
 assert(argc==2 || argc==3);
 if(argc==3){
  WorkerImage image(argv[1],63,5120);auto sample=Read(argv[2]);
  assert(sample.size()==102544 && Word(sample,0)==0x34504252 && Word(sample,4)==1 && Word(sample,8)==5120);
  for(unsigned band=0;band<2;band++)std::memcpy(image.transfer.weights.data()+band*160*576,sample.data()+16,160*576);
  std::memcpy(image.transfer.root.data(),sample.data()+16+160*576,10240);
  assert(image.Execute());
  for(unsigned band=0;band<2;band++)for(unsigned row=0;row<32;row++)
   assert(image.l1[image.Index(W::Output+(band*32+row)*4)]==Word(sample,16+160*576+10240+row*4));
  puts("PASS actual OFFBOARD BFP4 worker RV32: private real-weight sample, 64 exact predicted words, ROOT-L1 input; NO DEVICE");
  return 0;
 }
 for(unsigned width:{5120U,17408U})for(unsigned worker:{1U,63U,119U}){
  WorkerImage image(argv[1],worker,width);assert(image.Execute());const auto* expected=PioneerNativeBfp4BandGolden(width);assert(expected);
  for(unsigned band=0;band<2;band++)for(unsigned row=0;row<32;row++)assert(image.l1[image.Index(W::Output+(band*32+row)*4)]==expected[row]);
  printf("PASS actual OFFBOARD BFP4 worker RV32 K%u peer%u: 64 exact words, ROOT-L1 BF16, card GDDR, steps%u stack%u; NO DEVICE\n",width,worker,image.steps,0x70000-image.minSP);
 }
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT
 for(unsigned width:{5120U,17408U})for(unsigned bands:{1U,3U,4U,5U,6U,7U,8U}){
  WorkerImage image(argv[1],63,width,bands,true);assert(image.Execute());const auto* expected=PioneerNativeBfp4BandGolden(width);assert(expected);
  for(unsigned band=0;band<bands;band++)for(unsigned row=0;row<32;row++){
   const uint32_t value=image.l1[image.Index(W::Output+(band*32+row)*4)];
   if(band&1)assert((value&0x7fffffffU)==0);else assert(value==expected[row]);
  }
  printf("PASS OFFBOARD input reuse K%u bands%u: %u exact output words, %u NoC reads, steps%u; NO DEVICE\n",width,bands,bands*32,image.transfer.issues,image.steps);
 }
 { // Deliberate corruption demonstrates why a real retained-input lifetime proof is mandatory.
  WorkerImage image(argv[1],63,5120);image.damageInput=true;
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT
  assert(!image.Execute() && image.inputDamaged && image.stores==32);
  puts("PASS on-card diagnostic checksum stopped the tested staged-input corruption before the second band; NO DEVICE OR OWNERSHIP PROOF");
#else
  assert(image.Execute() && image.inputDamaged);
  const auto* expected=PioneerNativeBfp4BandGolden(5120);bool changed=false;
  for(unsigned row=0;row<32;row++)changed|=image.l1[image.Index(W::Output+(32+row)*4)]!=expected[row];
  assert(changed);puts("PASS negative control: staged-input corruption can change the second band without a fault; NO PRODUCTION AUTHORITY");
#endif
 }
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT
 {WorkerImage image(argv[1],63,5120);image.damageHeader=true;assert(!image.Execute() && image.inputDamaged && image.stores==32);}
 {WorkerImage image(argv[1],63,5120);image.damageOnMath=true;assert(!image.Execute() && image.inputDamaged && !image.stores);}
#endif
#endif
 for(unsigned word=0;word<W::Words;word++){
  WorkerImage image(argv[1],1,5120);image.l1[image.Index(0x74000+word*4)]=UINT32_MAX;
  assert(!image.Execute() && !image.transfer.issues && !image.stores);
 }
 {WorkerImage image(argv[1],1,5120);image.damageFrame=true;assert(!image.Execute() && image.stores==32);}
 {WorkerImage image(argv[1],1,5120);image.dropAckAt=1;assert(!image.Execute() && !image.stores);}
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT
 {WorkerImage image(argv[1],1,5120);image.dropAfterFirstBand=true;assert(!image.Execute() && image.stores==32);}
 {WorkerImage image(argv[1],1,5120);image.faultAfterFirstBand=true;assert(!image.Execute() && image.stores==32);}
#endif
 puts("PASS actual OFFBOARD BFP4 worker RV32 frame/guard/FPU-failure controls; NO DEVICE OR WORK");
}
#endif
