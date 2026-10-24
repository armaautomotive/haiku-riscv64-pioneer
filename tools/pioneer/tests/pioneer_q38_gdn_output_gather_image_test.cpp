/* SPDX-License-Identifier: MIT */
// Actual op6 peers -> GDDR bytes -> actual QOG1 ROOT -> SAME live matrix.
// Only NoC/GDDR/Tensix byte endpoints simulated;NOT native/model timing.
#define PIONEER_Q38_PROJECTION_GATHER_MACHINE_ONLY
#include "pioneer_q38_projection_gather_image_test.cpp"
#define PIONEER_Q38_GDN_OUTPUT_REUSE_PROJECTION_PEER
#define PIONEER_Q38_GDN_OUTPUT_INPUT_MACHINE_ONLY
#include "pioneer_q38_gdn_output_input_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputGatherShape.h"
struct GdnOutputGatherMachine:ProjectionGatherMachine {
 std::array<std::array<uint32_t,16>,24> peerSources={};
 unsigned shadowWrites=0,poolFault=0,shadowFault=0,sourceWord=0;
 GdnOutputGatherMachine(const char* path,unsigned bank=0,bool edge=false,unsigned phase=17,unsigned token=0)
  :ProjectionGatherMachine(path,0,13,bank,edge,phase){
  auto& d=descriptor;d[0]=TT_Q38_GDN_OUTPUT_GATHER_MARKER;d[40]=0;d[41]=phase-1;d[44]=TT_Q38_GDN_RENDEZVOUS_MARKER;d[45]=token;
  d[46]=TT_Q38_GDN_OUTPUT_GATHER_SOURCES;d[47]=24;SetDescriptor();
  for(unsigned w=0;w<24;w++){
   for(unsigned i=0;i<16;i++){
    peerSources[w][i]=i?0x12340000+w*0x100+i:0x16005+w*0x100;
    memory[TT_Q38_GDN_OUTPUT_GATHER_SOURCES+(w*16+i)*4]=peerSources[w][i];
    memory[TT_Q38_GDN_OUTPUT_GATHER_SHADOW+(w*16+i)*4]=0xdeadbeef;
   }
   auto& line=lines[w];const uint32_t fields[]={TT_Q38_GDN_OUTPUT_INPUT_MARKER,phase,w,8,6144,0,13,0,phase-1};
   for(unsigned i=0;i<9;i++)line[8+i]=fields[i];line[28]=peerSources[w][0];line[29]=TT_Q38_GDN_RENDEZVOUS_MARKER;line[30]=token;
   assert(PioneerQ38GdnOutputGatherLine(line.data(),d.data(),w,peerSources[w].data()));
  }
  for(unsigned at:{TT_Q38_GDN_OUTPUT_GATHER_SOURCES-4,TT_Q38_GDN_OUTPUT_GATHER_SOURCES+1536,
   TT_Q38_GDN_OUTPUT_GATHER_SHADOW-4,TT_Q38_GDN_OUTPUT_GATHER_SHADOW+1536})memory[at]=0xdecafbad;
  assert(PioneerQ38GdnOutputGatherShape(d.data()) && !PioneerQ38ProjectionGatherShape(d.data()));
 }
 uint32_t Load(uint32_t at)override{
  if(Range(at,TT_Q38_GDN_OUTPUT_GATHER_SOURCES,1536) || Range(at,TT_Q38_GDN_OUTPUT_GATHER_SHADOW,1536)){
   if((poolFault && packets==poolFault && at==TT_Q38_GDN_OUTPUT_GATHER_SOURCES+sourceWord*4)
    || (shadowFault && packets==shadowFault && at==TT_Q38_GDN_OUTPUT_GATHER_SHADOW+sourceWord*4))observed=true;
   return memory.at(at);
  }
  return ProjectionGatherMachine::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  if(Range(at,TT_Q38_GDN_OUTPUT_GATHER_SHADOW,1536)){
   assert(!observed && !packets && !(at&3));shadowWrites++;memory[at]=value;return;
  }
  assert(!Range(at,TT_Q38_GDN_OUTPUT_GATHER_SOURCES,1536));
  if(at==0xffb20040){
   assert(value==1 && !pending);ProjectionGatherMachine::SubmitGather();
   if(packets==poolFault)memory[TT_Q38_GDN_OUTPUT_GATHER_SOURCES+sourceWord*4]^=1;
   if(packets==shadowFault)memory[TT_Q38_GDN_OUTPUT_GATHER_SHADOW+sourceWord*4]^=1;
   return;
  }
  ProjectionGatherMachine::Store(at,value);
 }
 bool RunGdn(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(unsigned at:{TT_Q38_GDN_OUTPUT_GATHER_SOURCES-4,TT_Q38_GDN_OUTPUT_GATHER_SOURCES+1536,
   TT_Q38_GDN_OUTPUT_GATHER_SHADOW-4,TT_Q38_GDN_OUTPUT_GATHER_SHADOW+1536})assert(memory.at(at)==0xdecafbad);
  if(pc==0x64058){
   assert(memory.at(0x17ffc8)==TT_Q38_GDN_OUTPUT_GATHER_FAULT && !memory.at(0x17ffc4));
   unsigned p=packets,r=nocReads,w=commandWrites,c=compactWrites,s=shadowWrites;
   for(unsigned i=0;i<32;i++)StepScalar();assert(p==packets && r==nocReads && w==commandWrites && c==compactWrites && s==shadowWrites);return false;
  }
  assert(memory.at(0x17ffc8)==TT_Q38_GDN_OUTPUT_GATHER_MARKER && memory.at(0x17ffc0)==0x6400c);
  return memory.at(0x17ffc4)==24;
 }
 void VerifyGdn(){
  assert(packets==26 && !pending && operandsReceived==3072 && compactWrites==384 && shadowWrites==384);
  for(unsigned i=0;i<3072;i++)assert(memory.at(0xa0000+i*4)==native[i]);
  for(unsigned i=0;i<192;i++)assert(compact[i]==lines[i/8][i%8] && memory.at(0xab000+i*4)==compact[i]);
  for(unsigned i=0;i<384;i++)assert(memory.at(TT_Q38_GDN_OUTPUT_GATHER_SHADOW+i*4)==peerSources[i/16][i%16]);
  uint32_t s[32];for(unsigned i=0;i<32;i++)s[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  assert(PioneerQ38GdnOutputGatherSummary(s,descriptor.data(),before,counts));
  for(unsigned i=0;i<32;i++){s[i]^=1;assert(!PioneerQ38GdnOutputGatherSummary(s,descriptor.data(),before,counts));s[i]^=1;}
 }
};
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);unsigned producers=0,roots=0,matrices=0,faults=0,minStack=0x70000;
 for(unsigned bank=0;bank<7;bank++){
  std::vector<std::unique_ptr<GdnOutputInputPeer>> peers;
  for(unsigned chunk=0;chunk<24;chunk++){
   peers.emplace_back(new GdnOutputInputPeer(argv[1],chunk+1));auto& p=*peers.back();
   for(unsigned i=0;i<16;i++)p.source[i]=i?0x12340000+chunk*0x100+i:0x16005+chunk*0x100;
   p.memory[0x7404c]=p.source[0];assert(p.BootNew());
  }
  std::array<uint32_t,16> first={};
  for(unsigned changed=0;changed<2;changed++){
   unsigned token=changed?248319:0;GdnOutputGatherMachine root(argv[2],bank,changed!=0,17+changed,token);std::vector<float> full(6144);
   for(unsigned chunk=0;chunk<24;chunk++){
    auto& p=*peers[chunk];unsigned sequence=changed?(chunk?2:3):1;p.StageGdn(sequence,17+changed,bank,changed!=0);
    p.descriptor[45]=token;p.memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+45*4]=token;
    for(unsigned i=0;i<256;i++){
     float value=float(int((i*37+chunk*19+changed*73)%255)-127)*.25f;if(!chunk)value=Float((i&1)<<31);
     p.gate[i]=Bits(value);full[chunk*256+i]=value;
    }
    assert(p.Finish(sequence));p.VerifyGdn();producers++;
    memcpy(root.native.data()+chunk*128,p.native.data(),512);root.lines[chunk]=p.line;
   }
   auto originalLines=root.lines;auto originalNative=root.native;assert(root.RunGdn());root.VerifyGdn();
   assert(root.lines==originalLines && root.native==originalNative);roots++;minStack=std::min(minStack,root.minStack);
   PioneerQ38CohortInput reference;assert(PioneerQ38CohortQuantize(8,6144,full.data(),reference));
   for(unsigned i=0;i<3072;i++)assert(root.memory.at(0xa0000+i*4)==reference.activation[i]);
   for(unsigned i=0;i<192;i++)assert(root.memory.at(0xab000+i*4)==reference.scales[i]);
   auto& matrix=*peers[0];FeedMatrix(matrix,root);assert(matrix.MatrixNext(changed?4:2) && !matrix.dotProbeReads);matrices++;
   bool different=false;for(unsigned row=0;row<16;row++){if(!changed)first[row]=matrix.buffers[4096+row];else different|=first[row]!=matrix.buffers[4096+row];}
   if(changed)assert(different);
  }
  for(unsigned chunk=0;chunk<24;chunk++)assert(peers[chunk]->Stop(chunk?3:5));
  printf("PASS ACTUAL ALL24 op6 gated6144->GDDR->QOG1 ROOT->SAME live Q8_0 matrix bank%u/2changed inputs/distinct peer NIUs/independent FP32+FP64 references/NO reset;NOT complete tensor/native time\n",bank);
 }
 for(unsigned chunk=0;chunk<24;chunk++)for(unsigned word=0;word<32;word++){
  GdnOutputGatherMachine root(argv[2]);if(word<8)root.lines[chunk][word]=0x7fc00001;else root.lines[chunk][word]^=1;
  auto before=root.compact;assert(!root.RunGdn() && root.packets==chunk+1 && !root.operandsReceived && root.compact==before);faults++;
 }
 for(unsigned packet:{1U,12U,24U,25U,26U})for(unsigned fault=0;fault<5;fault++){
  GdnOutputGatherMachine root(argv[2]);if(!fault)root.failPacket=packet;if(fault==1)root.badCounter=packet;
  if(fault==2)root.foreignCounter=packet;if(fault==3)root.sourceFault=packet;if(fault==4)root.descriptorFault=packet;
  assert(!root.RunGdn() && root.packets==packet);faults++;
 }
 for(unsigned target=0;target<2;target++)for(unsigned word=0;word<384;word++){
  GdnOutputGatherMachine root(argv[2]);if(target)root.shadowFault=1;else root.poolFault=1;root.sourceWord=word;
  assert(!root.RunGdn() && root.packets==1 && root.observed && !root.operandsReceived);faults++;
 }
 for(unsigned packet:{24U,25U,26U})for(unsigned target=0;target<2;target++){
  GdnOutputGatherMachine root(argv[2]);if(target)root.shadowFault=packet;else root.poolFault=packet;root.sourceWord=383;
  assert(!root.RunGdn() && root.packets==packet && root.observed);faults++;
 }
 for(unsigned packet:{1U,24U,25U,26U})for(unsigned word=0;word<10;word++){
  GdnOutputGatherMachine root(argv[2]);root.badCommandRead=(packet-1)*10+word+1;
  assert(!root.RunGdn() && root.packets==packet-1);faults++;
 }
 for(unsigned word=0;word<16;word++){
  GdnOutputGatherMachine root(argv[2]);root.seals[word]^=1;assert(!root.RunGdn() && !root.packets && !root.shadowWrites);faults++;
 }
 for(unsigned chunk=0;chunk<24;chunk++){
  GdnOutputGatherMachine root(argv[2]);root.memory[TT_Q38_GDN_OUTPUT_GATHER_SOURCES+chunk*64]=0;
  assert(!root.RunGdn() && !root.packets && !root.shadowWrites);faults++;
 }
 for(unsigned word=0;word<64;word++){
  if(word>=32 && word<40)continue; // nonzero identity DATA alone is not native authority
  GdnOutputGatherMachine root(argv[2]);root.descriptor[word]^=1;root.SetDescriptor();
  bool valid=root.RunGdn();assert(!valid && !root.operandsReceived);
  if(word==45)assert(root.packets==1); // valid but different token DATA must not accept old lines
  else assert(!root.packets && !root.shadowWrites);
  faults++;
 }
 for(unsigned identity=0;identity<4;identity++){
  GdnOutputGatherMachine root(argv[2]);root.descriptor[32+identity*2]=root.descriptor[33+identity*2]=0;root.SetDescriptor();
  assert(!root.RunGdn() && !root.packets && !root.shadowWrites);faults++;
 }
 {GdnOutputGatherMachine old(argv[3]);while(old.pc!=0x64038 && old.pc!=0x64058)old.StepScalar();
  assert(old.pc==0x64038 && old.memory.at(0x17ffc8)==TT_Q38_PROJECTION_GATHER_MARKER && !old.memory.at(0x17ffc4)
   && !old.packets && !old.shadowWrites);faults++;}
 printf("PASS ACTUAL QOI1->QOG1->matrix producers%u/roots%u/matrices%u/faults%u/maxRootStack%u/ALL24actual peer sources+384word shadow seals/ALL lines BEFORE operands+compact sink/fail-first inert;NOT native full projection/model speed\n",producers,roots,matrices,faults,0x70000-minStack);
}
