/* SPDX-License-Identifier: MIT */
// Actual QGW7 gated producers -> synthetic byte transport -> actual QGW7
// BFP4 down projection on worker 1, preserving its private execution state.
// Synthetic gate/up/weights; not a complete layer, ROOT lifecycle or device.
#define PIONEER_Q38_HYBRID_MATRIX_TRANSITION_MACHINE_ONLY
#include "pioneer_q38_hybrid_matrix_attention_transition_test.cpp"
#include "../PioneerQ38Bf16ProducerLayout.h"
#include <map>

namespace BP=PioneerQ38Bf16ProducerLayout;
struct ProducerFabric {
 std::map<uint64_t,uint32_t> words;
 static uint64_t Key(unsigned route,unsigned address){return (uint64_t(route)<<32)|address;}
 unsigned Get(unsigned route,unsigned address)const{return words.at(Key(route,address));}
 void Put(unsigned route,unsigned address,unsigned value){words[Key(route,address)]=value;}
};

struct ConnectedProducer:TransitionImage {
 ProducerFabric& fabric;bool producing=false;unsigned packets=0;
 std::array<uint32_t,64> descriptor={};std::array<uint32_t,8> before={};
 ConnectedProducer(const char* worker,const char* leaf,unsigned peer,ProducerFabric& f)
  :TransitionImage(worker,leaf,peer),fabric(f){}
 static bool ProducerL1(unsigned a){return Range(a,BP::Descriptor,256) || Range(a,0x90000,3712);}
 unsigned Load(unsigned a)override{
  if(producing && ProducerL1(a))return WordRead(a);
  if(producing && Range(a,0x74000,96))return Get(a);
  if(producing && a<0x180000 && !CodeAddress(a) && !Range(a,0x6e000,8192)
   && !Range(a,HW::Boot,512) && !Range(a,HW::Status,64) && a!=HW::Doorbell
   && !Range(a,0x74000,96) && a!=0x74100 && !Range(a,0x83f00,256))
   fprintf(stderr,"unmodeled producer read %x at pc %x\n",a,pc);
  return TransitionImage::Load(a);
 }
 void Store(unsigned a,unsigned value)override{
  if(producing && ProducerL1(a)){
   assert(PioneerQ38HybridTokenL1::SnapshotContains(boot[3],a));WordWrite(a,value);return;
  }
  if(producing && Range(a,0xffb20000,40)){transfer.command[(a-0xffb20000)/4]=value;return;}
  if(producing && a==0xffb20040){assert(value==1);Packet();return;}
  TransitionImage::Store(a,value);
 }
 void Packet(){
  const unsigned p=packets++,reads=BP::Reads(descriptor.data());assert(p<reads+2);
  const bool write=p>=reads,trailer=p==reads+1;
  const unsigned local=write?(trailer?0x90e00:0x90c00):(p?0x90400:0x90000);
  const unsigned bytes=write?(trailer?128U:512U):1024U;
  const unsigned remote=descriptor[4]+descriptor[write?(trailer?13:12):10+p];
  const unsigned expected[]={write?local:remote,0,write?boot[4]:descriptor[8],
   write?remote:local,0,write?descriptor[8]:boot[4],0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(transfer.command[i]==expected[i]);
  for(unsigned i=0;i<bytes;i+=4){
   if(write)fabric.Put(descriptor[8],remote+i,WordRead(local+i));
   else WordWrite(local+i,fabric.Get(descriptor[8],remote+i));
  }
  auto& c=transfer.counters;c[4]++;c[14]++;
  if(write){c[1]++;c[8]+=bytes/64;c[10]++;}
  else{c[2]++;c[3]+=bytes/64;c[5]++;}
 }
 void StageProducer(unsigned layer,unsigned sequence){
  assert(boot[3]<=68 && pc==waitPC && !producing);producing=true;packets=0;
  const unsigned indices[]={2,3,4,5,1,8,4,10};
  for(unsigned i=0;i<8;i++)before[i]=transfer.counters[indices[i]];
  auto& d=descriptor;d.fill(0);d[0]=BP::Magic;d[1]=1;d[2]=67108864;d[4]=0x4000000;
  d[5]=boot[3]-1;d[6]=2025;d[7]=boot[3];d[8]=PioneerQ38ProjectionRoute(0);d[9]=boot[4];
  d[10]=BP::GateOffset+d[5]*1024;d[11]=BP::UpOffset+d[5]*1024;
  d[12]=BP::NativeOffset+d[5]*512;d[13]=BP::LinesOffset+d[5]*128;d[14]=BP::Format;d[15]=68;
  for(unsigned i=0;i<16;i++)d[16+i]=source[i];for(unsigned i=0;i<8;i++)d[32+i]=boot[8+i];
  d[41]=248319;d[42]=layer;d[43]=4;assert(BP::DescriptorShape(d.data()));
  unsigned j[64]={};j[0]=HW::Magic;j[1]=sequence;j[2]=3;j[3]=boot[3];j[4]=boot[4];j[5]=layer;j[7]=2025;
  for(unsigned i=0;i<8;i++)j[8+i]=boot[8+i];
  j[16]=BP::Magic;j[17]=256;j[18]=512;j[19]=2025;j[20]=12;
  j[27]=d[41];j[28]=4;j[29]=17408;j[30]=BP::Format;j[31]=BP::Witness;
  assert(HW::JobShape(j,boot.data(),sequence-1,0));
  for(unsigned i=0;i<64;i++){WordWrite(BP::Descriptor+i*4,d[i]);Put(HW::Job+i*4,j[i]);}
  Put(HW::Doorbell,sequence);
 }
 void FinishProducer(unsigned sequence){
  assert(FinishCombined(sequence));assert(packets==4);
  unsigned proof[64],job[16],after[8],line[32];
  for(unsigned i=0;i<64;i++)proof[i]=Get(0x83f00+i*4);
  for(unsigned i=0;i<16;i++)job[i]=Get(0x74000+i*4);
  const unsigned indices[]={2,3,4,5,1,8,4,10};
  for(unsigned i=0;i<8;i++)after[i]=transfer.counters[indices[i]];
  assert(BP::Proof(proof,job,descriptor.data(),source.data(),boot[3],boot[4],before.data(),after));
  for(unsigned i=0;i<32;i++)line[i]=fabric.Get(descriptor[8],descriptor[4]+descriptor[13]+i*4);
  assert(BP::Line(line,descriptor.data(),sequence));producing=false;
 }
};

int main(int argc,char** argv){
 assert(argc==3);ProducerFabric fabric;const unsigned route=PioneerQ38ProjectionRoute(0),base=0x4000000;
 {
  ConnectedProducer missing(argv[1],argv[2],1,fabric);missing.Boot();missing.Stage(0,1,4);
  assert(!missing.FinishCombined(1) && !missing.native.matrixOps);
 }
 for(unsigned i=0;i<17408;i++){
  fabric.Put(route,base+BP::GateOffset+i*4,PioneerAttentionBits(float(int(i%257)-128)*.03125f));
  fabric.Put(route,base+BP::UpOffset+i*4,PioneerAttentionBits(float(int((i*17)%257)-128)*.015625f));
 }
 auto first=std::make_unique<ConnectedProducer>(argv[1],argv[2],1,fabric);first->Boot();
 for(unsigned w=1;w<=68;w++){
  std::unique_ptr<ConnectedProducer> other;ConnectedProducer* p=first.get();
  if(w!=1){other=std::make_unique<ConnectedProducer>(argv[1],argv[2],w,fabric);p=other.get();p->Boot();}
  p->StageProducer(0,1);p->FinishProducer(1);
 }
 // Model a byte-for-byte ROOT transport from the producers' actual output;
 // no host calculation substitutes for the gated producer instructions.
 for(unsigned i=0;i<17408*2;i+=4){
  const unsigned v=fabric.Get(route,base+BP::NativeOffset+i);
  for(unsigned b=0;b<4;b++)first->transfer.root[i+b]=uint8_t(v>>(8*b));
 }
 first->Stage(0,2,4);assert(first->FinishCombined(2) && first->native.Clean());
 // Post-hoc observer: unpack the synthetic weights independently and use
 // the bytes actually written by the producers, never feed an answer back.
 uint32_t expected[32]={};unsigned nonzero=0;
 for(unsigned tile=0;tile<17408/32;tile++){
  float decoded[1024];
  assert(PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,
   first->transfer.weights.data()+tile*576,576,decoded,1024));
  for(unsigned half=0;half<2;half++)for(unsigned phase:{0U,2U})for(unsigned row=0;row<32;row++){
   uint16_t weights[16],inputs[16];
   for(unsigned c=0;c<16;c++){
    const unsigned at=(tile*32+half*16+c)*2;
    const unsigned input=first->transfer.root[at]|unsigned(first->transfer.root[at+1])<<8;
    if(!row && !phase)nonzero+=input!=0;
    const unsigned bits=NativeBfpMachine::Bits(decoded[row*32+half*16+c]);assert(!(bits&65535));
    weights[c]=uint16_t(bits>>16);inputs[c]=uint16_t(input);
   }
   assert(PioneerBlackholeBf16Arithmetic::Instruction(weights,inputs,phase,expected[row],expected[row]));
  }
 }
 assert(nonzero>1000);
 for(unsigned band=0;band<2;band++)for(unsigned row=0;row<32;row++){
  const unsigned actual=first->Get(W::Output+(band*32+row)*4),want=expected[row];
  if(actual!=want)fprintf(stderr,"band %u row %u actual %08x expected %08x\n",band,row,actual,want);
  assert(actual==want);
 }
 puts("PASS actual QGW7 gated producer cohort 68 -> byte-only synthetic transport -> SAME worker 1 BFP4 full-K down projection; 64 words match post-hoc unpack/accumulation using shared arithmetic model; missing producer refused; no ROOT lifecycle, real model, hardware or TPS");
}
