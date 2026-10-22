/* SPDX-License-Identifier: MIT */
// ACTUAL op5 peers -> simulated GDDR -> ACTUAL ROOT gather -> SAME live
// peer's ACTUAL matrix instructions. Endpoint copies ONLY;test references
// never populate device arithmetic. NOT a native coordinator/model benchmark.
#define PIONEER_Q38_PROJECTION_MACHINE_ONLY
#define PIONEER_Q38_REF_DIAGNOSTIC
#define PIONEER_Q38_REFERENCE_ROUNDING_BOUND
#include "pioneer_q38_projection_persistent_image_test.cpp"
#define PIONEER_Q38_FFN_RESIDENT_ACTIVATION_MACHINE_ONLY
#include "pioneer_q38_ffn_resident_activation_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGatherShape.h"
struct ProjectionGatherMachine:ResidentActivationMachine {
 std::vector<uint32_t> native;
 std::vector<std::array<uint32_t,32>> lines;
 std::array<uint32_t,192> compact;
 uint32_t before[8]={};
 unsigned operandsReceived=0,compactWrites=0,aliasReads=0,badAliasRead=0;
 bool observed=false;
 ProjectionGatherMachine(const char* path,unsigned layer,unsigned role,unsigned bank=0,bool edge=false,unsigned phase=17)
  :ResidentActivationMachine(path,bank,edge){
  unsigned type=0,k=0,n=0;assert(PioneerQ38ProjectionGeometry(layer,role,type,k,n));
  auto& d=descriptor;d[0]=TT_Q38_PROJECTION_GATHER_MARKER;d[5]=k/256;d[6]=phase;d[7]=0;d[9]=129;
  d[10]=TT_Q38_PROJECTION_NATIVE_OFFSET;d[11]=TT_Q38_PROJECTION_LINES_OFFSET;d[12]=TT_Q38_PROJECTION_COMPACT_OFFSET;
  d[13]=k*2;d[14]=(k/(type==8?32:256)*4+63)&~63U;d[15]=type;d[42]=layer;d[43]=role;
  for(unsigned i=0;i<16;i++)d[16+i]=seals[i]=i?0x12340000+i:0x16005;
  SetDescriptor();assert(PioneerQ38ProjectionGatherShape(d.data()));
  native.assign(k/2,0x40014001);lines.resize(k/256);compact.fill(0xdeadbeef);
  for(unsigned i=0;i<k/2;i++)memory[0xa0000+i*4]=0xdeadbeef;
  for(unsigned i=0;i<192;i++)memory[0xab000+i*4]=0xdeadbeef;
  for(unsigned i=0;i<8;i++)before[i]=counts[i];
  for(unsigned chunk=0;chunk<d[5];chunk++){
   auto& line=lines[chunk];for(unsigned i=0;i<(type==8?8U:1U);i++)line[i]=0x3f800000;
   const uint32_t fields[]={TT_Q38_PROJECTION_INPUT_MARKER,phase,chunk,type,k,layer,role,d[40],d[41]};
   for(unsigned i=0;i<9;i++)line[8+i]=fields[i];for(unsigned i=0;i<8;i++)line[17+i]=d[32+i];
   line[25]=d[5];line[26]=chunk+1;line[27]=PioneerQ38ProjectionPeer(chunk);line[28]=seals[0];line[31]=1;
   assert(PioneerQ38ProjectionGatherLine(line.data(),d.data(),chunk));
  }
 }
 unsigned Chunks()const{return (descriptor[13]+16383)/16384;}
 uint32_t Load(uint32_t at)override{
  if(Range(at,0xffb20000,0x2000)){
   assert(!observed);
   if(at==0xffb20210 && ++aliasReads==badAliasRead){observed=true;nocReads++;return counts[2]^1;}
  }
  if(Range(at,0xa0000,descriptor[13]) || Range(at,0xab000,768) || Range(at,0x90e00,128))return memory.at(at);
  uint32_t value=ResidentActivationMachine::Load(at);
  if(Range(at,0xffb20000,0x2000)){
   for(unsigned i=0;i<16;i++)if(at==0xffb20000+SourceOffset(i) && value!=descriptor[16+i])observed=true;
   if(busyOffset && at==0xffb20000+busyOffset)observed=true;
   if(badCommandRead && commandReads==badCommandRead && Range(at,0xffb20000,40))observed=true;
  }
  if(descriptorFault && packets==descriptorFault && at==TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4)observed=true;
  return value;
 }
 void Store(uint32_t at,uint32_t value)override{
  if(Range(at,0xffb20000,0x2000))assert(!observed);
  if(Range(at,0xab000,768)){assert(Range(at,0xab000,descriptor[14]) && !(at&3));compactWrites++;memory[at]=value;return;}
  if(at==0xffb20040){assert(value==1 && !pending);SubmitGather();return;}
  assert(!Range(at,0xa0000,descriptor[13]) && !Range(at,0x90000,3712));
  ResidentActivationMachine::Store(at,value);
 }
 void SubmitGather(){
  unsigned n=descriptor[5],total=n+Chunks()+1;assert(packets<total);unsigned p=packets++;bool write=p==total-1;
  unsigned local,remote,bytes;
  if(p<n){local=0x90e00;remote=descriptor[4]+descriptor[11]+p*128;bytes=128;}
  else if(!write){unsigned at=(p-n)*16384;local=0xa0000+at;remote=descriptor[4]+descriptor[10]+at;bytes=std::min(16384U,descriptor[13]-at);}
  else{local=0xab000;remote=descriptor[4]+descriptor[12];bytes=descriptor[14];}
  const uint32_t expected[]={write?local:remote,0,write?129U:descriptor[8],write?remote:local,0,write?descriptor[8]:129U,0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(packets==failPacket){pending=true;return;}
  assert(!activatedWrites && !operandWrites && !nativeScaleWrites);
  for(unsigned i=0;i<bytes/4;i++){
   if(p<n)memory[local+i*4]=lines[p][i];
   else if(!write){memory[local+i*4]=native[(p-n)*4096+i];operandsReceived++;}
   else compact[i]=memory.at(local+i*4);
  }
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=bytes/64;counts[at+3]++;counts[2]++;counts[6]++;
  if(packets==badCounter)counts[at+3]++;if(packets==foreignCounter)counts[write?0:4]++;
  if(packets==sourceFault)seals[11]^=1;if(packets==descriptorFault)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4]^=1;
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  if(pc==0x64058){
   assert(memory.at(0x17ffc8)==TT_Q38_PROJECTION_GATHER_FAULT && !memory.at(0x17ffc4));
   unsigned p=packets,r=nocReads,w=commandWrites,c=compactWrites;
   for(unsigned i=0;i<32;i++)StepScalar();assert(p==packets && r==nocReads && w==commandWrites && c==compactWrites);return false;
  }
  assert(memory.at(0x17ffc8)==TT_Q38_PROJECTION_GATHER_MARKER && memory.at(0x17ffc0)==0x6400c);
  return memory.at(0x17ffc4)==descriptor[5];
 }
 void Verify(){
  unsigned per=descriptor[15]==8?8:1,words=descriptor[5]*per,padded=descriptor[14]/4;
  assert(packets==descriptor[5]+Chunks()+1 && !pending && operandsReceived==native.size() && compactWrites==words+padded);
  for(unsigned i=0;i<native.size();i++)assert(memory.at(0xa0000+i*4)==native[i]);
  for(unsigned i=0;i<192;i++)assert(compact[i]==(i<words?lines[i/per][i%per]:i<padded?0:0xdeadbeef) && memory.at(0xab000+i*4)==compact[i]);
  uint32_t s[32];for(unsigned i=0;i<32;i++)s[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  assert(PioneerQ38ProjectionGatherSummary(s,descriptor.data(),before,counts));
  for(unsigned i=0;i<32;i++){s[i]^=1;assert(!PioneerQ38ProjectionGatherSummary(s,descriptor.data(),before,counts));s[i]^=1;}
 }
};
// Replace endpoint bytes ONLY at a known WAIT. Root-generated operands and
// scales feed the actual matrix image;independent observers read those same
// bytes. Weight packets remain original bounded nonzero test fixtures.
static void FeedMatrix(ProjectionPeer& p,const ProjectionGatherMachine& root){
 assert(p.pc==TT_Q38_PEER_WAIT_LOAD && !p.pending && p.inputs==root.descriptor[13]/2);
 for(unsigned i=0;i<p.inputs/2;i++)p.root[0xa0000+i*4]=root.memory.at(0xa0000+i*4);
 for(unsigned i=0;i<root.descriptor[14]/4;i++)p.root[0xab000+i*4]=root.memory.at(0xab000+i*4);
 for(unsigned i=0;i<p.inputs;i++)p.vector[i]=uint16_t(p.root.at(0xa0000+(i/2)*4)>>((i%2)*16));
 for(unsigned i=0;i<p.scales.size();i++)p.scales[i]=p.root.at(0xab000+i*4);
}
#ifndef PIONEER_Q38_PROJECTION_GATHER_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);unsigned producers=0,roots=0,matrices=0,faults=0,minStack=0x70000;
 const unsigned cases[][2]={{0,5},{0,13},{0,2},{0,4},{3,10},{64,1},{64,0}};
 for(unsigned selected=0;selected<7;selected++){
  unsigned layer=cases[selected][0],role=cases[selected][1],type=0,k=0,n=0;assert(PioneerQ38ProjectionGeometry(layer,role,type,k,n));
  std::vector<std::unique_ptr<ProjectionPeer>> peers;
  for(unsigned chunk=0;chunk<k/256;chunk++){peers.emplace_back(new ProjectionPeer(argv[1],type,k,chunk+1));assert(peers.back()->BootNew());}
  std::array<uint32_t,16> first={};
  for(unsigned changed=0;changed<2;changed++){
   ProjectionGatherMachine root(argv[2],layer,role,selected,changed!=0,17+changed);std::vector<float> full(k);
   for(unsigned chunk=0;chunk<k/256;chunk++){
    auto& p=*peers[chunk];unsigned sequence=changed?(chunk?2:3):1;p.Stage(sequence,layer,role,17+changed,selected,changed!=0);
    for(unsigned i=0;i<256;i++){
     float value=float(int((i*37+chunk*19+changed*73)%255)-127)*.25f;if(!chunk)value=Float((i&1)<<31);
     p.gate[i]=Bits(value);full[chunk*256+i]=value;
    }
    assert(p.Finish(sequence));p.Verify();producers++;memcpy(root.native.data()+chunk*128,p.native.data(),512);root.lines[chunk]=p.line;
   }
   auto lines=root.lines;auto native=root.native;assert(root.Run());root.Verify();assert(root.lines==lines && root.native==native);
   roots++;minStack=std::min(minStack,root.minStack);
   PioneerQ38CohortInput reference;assert(PioneerQ38CohortQuantize(type,k,full.data(),reference));
   for(unsigned i=0;i<k/2;i++)assert(root.memory.at(0xa0000+i*4)==reference.activation[i]);
   for(unsigned i=0;i<root.descriptor[14]/4;i++)assert(root.memory.at(0xab000+i*4)==reference.scales[i]);
   auto& matrix=*peers[0];FeedMatrix(matrix,root);assert(matrix.MatrixNext(changed?4:2) && !matrix.dotProbeReads);matrices++;
   bool different=false;for(unsigned row=0;row<16;row++){if(!changed)first[row]=matrix.buffers[4096+row];else different|=first[row]!=matrix.buffers[4096+row];}
   if(changed)assert(different);
  }
  for(unsigned chunk=0;chunk<peers.size();chunk++)assert(peers[chunk]->Stop(chunk?3:5));
  printf("PASS ACTUAL op5 ALL%u chunks->GDDR->ACTUAL QPG1 ROOT->SAME live peer matrix type%u K%u,2changed inputs/NO reset/FP32+FP64 references;SIMULATED endpoints+test weights,NOT native or full tensor\n",k/256,type,k);
 }
 for(const auto& selected:cases){
  ProjectionGatherMachine shape(argv[2],selected[0],selected[1]);
  for(unsigned chunk=0;chunk<shape.descriptor[5];chunk++)for(unsigned word=0;word<32;word++){
   ProjectionGatherMachine root(argv[2],selected[0],selected[1]);
   if(word<8)root.lines[chunk][word]=0x7fc00001;else root.lines[chunk][word]^=1;
   auto before=root.compact;assert(!root.Run() && root.packets==chunk+1 && !root.operandsReceived && root.compact==before);faults++;
  }
  printf("PASS ROOT every chunk/32word line stale-or-invalid refusal layer%u role%u;NO operand read or compact sink\n",selected[0],selected[1]);
 }
 for(const auto& selected:cases){
  ProjectionGatherMachine shape(argv[2],selected[0],selected[1]);unsigned n=shape.descriptor[5],total=n+shape.Chunks()+1;
  for(unsigned packet:{1U,n/2,n,n+1,total})for(unsigned fault=0;fault<5;fault++){
   ProjectionGatherMachine root(argv[2],selected[0],selected[1]);if(!fault)root.failPacket=packet;if(fault==1)root.badCounter=packet;
   if(fault==2)root.foreignCounter=packet;if(fault==3)root.sourceFault=packet;if(fault==4)root.descriptorFault=packet;
   assert(!root.Run() && root.packets==packet);faults++;
  }
  for(unsigned packet:{1U,n,n+1,total})for(unsigned word=0;word<10;word++){
   ProjectionGatherMachine root(argv[2],selected[0],selected[1]);root.badCommandRead=(packet-1)*10+word+1;
   assert(!root.Run() && root.packets==packet-1);faults++;
  }
 }
 for(unsigned word=0;word<16;word++){
  ProjectionGatherMachine root(argv[2],0,5);root.seals[word]^=1;assert(!root.Run() && !root.packets && root.observed);faults++;
 }
 for(unsigned offset:{0x40U,0x840U,0x1040U,0x1840U,0x240U,0x2bcU}){
  ProjectionGatherMachine root(argv[2],0,5);root.busyOffset=offset;assert(!root.Run() && !root.packets && root.observed);faults++;
 }
 for(unsigned read:{2U,4U}){
  ProjectionGatherMachine root(argv[2],0,5);root.badAliasRead=read;assert(!root.Run() && !root.packets && root.observed);faults++;
 }
 for(unsigned word=0;word<64;word++){
  if(word==6 || (word>=16 && word<44))continue;ProjectionGatherMachine root(argv[2],0,5);root.descriptor[word]^=1;root.SetDescriptor();
  assert(!root.Run() && !root.packets && !root.nocReads);faults++;
 }
 // A different NONZERO phase/token/identity is valid DATA geometry,not a
 // malformed descriptor. The stale producer line must reject on first
 // read;only a native owner can authorize the new identity itself.
 for(unsigned word:{6U,32U,34U,36U,38U,40U,41U,42U,43U}){
  ProjectionGatherMachine root(argv[2],0,5);root.descriptor[word]^=word==43?3U:1U;root.SetDescriptor();
  assert(PioneerQ38ProjectionGatherShape(root.descriptor.data()));
  assert(!root.Run() && root.packets==1 && !root.operandsReceived);faults++;
 }
 for(unsigned invalid=0;invalid<12;invalid++){
  ProjectionGatherMachine root(argv[2],0,5);auto& d=root.descriptor;
  if(invalid<4)d[32+invalid*2]=d[33+invalid*2]=0;
  if(invalid==4)d[16]=0;if(invalid==5)d[6]=0;if(invalid==6)d[6]=0x7fffffff;
  if(invalid==7)d[3]=7;if(invalid==8)d[4]=0x03fff000;if(invalid==9)d[4]=0xc0001000;
  if(invalid==10)d[42]=65;if(invalid==11)d[43]=14;
  root.SetDescriptor();assert(!root.Run() && !root.packets && !root.nocReads);faults++;
 }
 printf("PASS OFFLINE compositional projection:ACTUAL producers%u/ROOTS%u/changed-input matrices%u/faults%u/maxRootStack%u;ALL3wire types/3widths/7banks/both base edges,all128B tags before operands/sink,inert UNKNOWN;NO_NATIVE_OWNER_CONTROLLER_ALL498MATRICES_OR_MODEL_SPEED_EVIDENCE\n",producers,roots,matrices,faults,0x70000-minStack);
}
#endif
