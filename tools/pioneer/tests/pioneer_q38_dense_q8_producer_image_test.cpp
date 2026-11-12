/* SPDX-License-Identifier: MIT */
// Actual QGWB RV32 producer and dispatcher, synthetic history and NoC.
// No hardware, full model, source authentication, timing or Work claim.
#define PIONEER_Q38_HYBRID_MATRIX_TRANSITION_MACHINE_ONLY
#include "pioneer_q38_hybrid_matrix_attention_transition_test.cpp"
#include "../PioneerQ38Bf16ProducerLayout.h"
#include "../PioneerQ38Bf16Input.h"
#include <map>
namespace BP=PioneerQ38Bf16ProducerLayout;
struct DenseProducerImage:TransitionImage {
 std::array<uint32_t,256> input={};std::map<unsigned,unsigned> bank;
 std::array<uint32_t,64> descriptor={};std::array<uint32_t,8> before={};
 unsigned packets=0;bool producing=false;
 DenseProducerImage(const char* worker,const char* leaf,unsigned width=17408,unsigned bands=2,unsigned peer=1):TransitionImage(worker,leaf,peer,width,bands){}
 static bool ProducerL1(unsigned a){return Range(a,BP::Descriptor,256) || Range(a,0x90000,3712);}
 uint32_t Load(uint32_t a)override{
  if(producing && ProducerL1(a))return WordRead(a);
  if(producing && Range(a,0x74000,96))return Get(a);
  return TransitionImage::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  if(producing && ProducerL1(a)){assert(PioneerQ38HybridTokenL1::SnapshotContains(boot[3],a));WordWrite(a,value);return;}
  if(producing && Range(a,0xffb20000,40)){transfer.command[(a-0xffb20000)/4]=value;return;}
  if(producing && a==0xffb20040){assert(value==1);Packet();return;}
  TransitionImage::Store(a,value);
 }
 void Packet(){
  const unsigned p=packets++;assert(p<3);const bool write=p>=1,trailer=p==2;
  const unsigned local=write?(trailer?0x90e00:0x90c00):0x90000,bytes=write?(trailer?128U:512U):1024U;
  const unsigned remote=write?descriptor[4]+descriptor[trailer?13:12]:descriptor[10];
  const unsigned expected[]={write?local:remote,0,write?boot[4]:unsigned(BP::InputRoot),write?remote:local,0,
   write?descriptor[8]:boot[4],0,write?0x212U:0U,bytes,0};
  for(unsigned i=0;i<10;i++)assert(transfer.command[i]==expected[i]);
  for(unsigned i=0;i<bytes;i+=4){if(write)bank[remote+i]=WordRead(local+i);else WordWrite(local+i,input[i/4]);}
  auto& c=transfer.counters;c[4]++;c[14]++;
  if(write){c[1]++;c[8]+=bytes/64;c[10]++;}else{c[2]++;c[3]+=bytes/64;c[5]++;}
 }
 void StageProducer(unsigned role,unsigned sequence,bool corrupt=false,unsigned layer=3){
  assert(pc==waitPC && ((layer%4==3 && role>=5 && role<=7)
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
   || (layer%4!=3 && (role==5 || role==6 || role==13))
#endif
  ));producing=true;packets=0;bank.clear();
  const unsigned width=layer%4!=3 && role==13?6144:5120;
  const unsigned indices[]={2,3,4,5,1,8,4,10};for(unsigned i=0;i<8;i++)before[i]=transfer.counters[indices[i]];
  auto& d=descriptor;d.fill(0);d[0]=BP::Magic;d[1]=1;d[2]=67108864;d[4]=0x4000000;d[5]=boot[3]-1;d[6]=2025;
  d[7]=boot[3];d[8]=PioneerQ38ProjectionRoute(0);d[9]=boot[4];d[10]=BP::ProjectionInput+d[5]*1024;
  d[11]=width;d[12]=BP::NativeOffset+d[5]*512;d[13]=BP::LinesOffset+d[5]*128;d[14]=BP::Format;d[15]=width/256;
  for(unsigned i=0;i<16;i++)d[16+i]=source[i];for(unsigned i=0;i<8;i++)d[32+i]=boot[8+i];
  d[41]=248319;d[42]=layer;d[43]=role;assert(BP::DescriptorShape(d.data()));
  unsigned j[64]={};j[0]=HW::Magic;j[1]=sequence;j[2]=5;j[3]=boot[3];j[4]=boot[4];j[5]=layer;j[7]=2025;
  for(unsigned i=0;i<8;i++)j[8+i]=boot[8+i];j[16]=BP::Magic;j[17]=256;j[18]=512;j[19]=2025;j[20]=12;
  j[27]=d[41];j[28]=role;j[29]=width;j[30]=BP::Format;j[31]=BP::Witness;
  assert(HW::JobShape(j,boot.data(),sequence-1,0));if(corrupt)j[28]=8;
  for(unsigned i=0;i<64;i++){WordWrite(BP::Descriptor+i*4,d[i]);Put(HW::Job+i*4,j[i]);}
  Put(HW::Doorbell,sequence);
 }
 void Check(unsigned sequence){
  assert(FinishCombined(sequence) && packets==3);unsigned proof[64],job[16],after[8],line[32],expected[128];
  for(unsigned i=0;i<64;i++)proof[i]=Get(0x83f00+i*4);for(unsigned i=0;i<16;i++)job[i]=Get(0x74000+i*4);
  const unsigned indices[]={2,3,4,5,1,8,4,10};for(unsigned i=0;i<8;i++)after[i]=transfer.counters[indices[i]];
  assert(BP::Proof(proof,job,descriptor.data(),source.data(),boot[3],boot[4],before.data(),after));
  for(unsigned i=0;i<32;i++)line[i]=bank.at(descriptor[4]+descriptor[13]+i*4);
  assert(BP::Line(line,descriptor.data(),sequence));
  assert(PioneerQ38Bf16Input::Pack(input.data(),256,expected,128));
  for(unsigned i=0;i<128;i++)assert(bank.at(descriptor[4]+descriptor[12]+i*4)==expected[i]);
  producing=false;
 }
};
#ifndef PIONEER_Q38_DENSE_Q8_CONNECTED_TEST
int main(int argc,char** argv){
 assert(argc==3);DenseProducerImage image(argv[1],argv[2]);image.Boot();
 // Exact image pin in runner binds this synthetic layer-history injection.
 assert(image.regs[8]==0x70000);image.Put(image.regs[8]-1136,3);
 for(unsigned i=0;i<256;i++)image.input[i]=PioneerAttentionBits(float(int(i%37)-18)*.03125f);
 for(unsigned role=5;role<=7;role++){image.StageProducer(role,role-4);image.Check(role-4);}
 DenseProducerImage invalid(argv[1],argv[2]);invalid.Boot();invalid.Put(invalid.regs[8]-1136,3);
 invalid.StageProducer(5,1,true);assert(!invalid.FinishCombined(1) && !invalid.packets);
 puts("PASS actual QGWB dense BF16 producer and proof for Q/G,K,V; synthetic history/NoC; NO HARDWARE OR SPEED");
}
#endif
