/* SPDX-License-Identifier: MIT */
// ACTUAL new QFP3 instructions,simulated NoC/GDDR and math tile bytes.
// Explicit16KiB immutable ROM fixture,NOT the old12KiB native owner grant.
#define PIONEER_Q38_FFN_INPUT_MACHINE_ONLY
#include "pioneer_q38_ffn_input_image_test.cpp"
#include "pioneer_q38_projection_input_reference.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
struct ProjectionPeer:FfnInputPeer {
 bool projection=false,observed=false,initialBusy=false;
 unsigned lineWrites=0,busyPacket=0,busyAt=0,aliasReads=0,badAliasRead=0;
 std::array<uint32_t,32> line={};
 ProjectionPeer(const char* path,unsigned type,unsigned inputs,unsigned worker):FfnInputPeer(path,type,inputs,1,worker,16384){line.fill(0xa5a5a5a5);}
 void SubmitActivation()override{
  if(!projection){FfnInputPeer::SubmitActivation();return;}
  ClearTensorState();assert(activationPackets<3);unsigned p=activationPackets++,write=p!=0;
  const unsigned local[]={0x90000,0x90c00,0x90e00},bytes[]={1024,512,128},index[]={10,12,13};
  unsigned remote=descriptor[4]+descriptor[index[p]];
  const uint32_t expected[]={write?local[p]:remote,0,write?descriptor[9]:descriptor[8],write?remote:local[p],0,
   write?descriptor[8]:descriptor[9],0,write?0x212U:0,bytes[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(activationPackets==failPacket){pending=true;return;}
  assert(!activatedWrites);if(!write)assert(!nativeWrites && !lineWrites);else assert(nativeWrites==128 && lineWrites==40);
  for(unsigned i=0;i<bytes[p]/4;i++){
   if(p==0)memory[local[p]+i*4]=gate[i];if(p==1)native[i]=memory.at(local[p]+i*4);if(p==2)line[i]=memory.at(local[p]+i*4);
  }
  unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=bytes[p]/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
  if(activationPackets==badCounterPacket)countBase[at+3]++;
  if(activationPackets==foreignCounterPacket)countBase[write?0:4]++;
  if(activationPackets==sourcePacket)source[11]^=1;
  if(activationPackets==descriptorPacket)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4]^=1;
  if(p==2){if(lateJob)memory[0x74000+4]^=1;if(lateBoot)memory[0x74040+4]^=1;if(lateDoorbell)memory[0x74100]++;}
 }
 uint32_t Load(uint32_t a)override{
  if(projection && Range(a,0xffb20000,0x2000)){
   assert(!observed);
   if(((busyPacket && activationPackets==busyPacket) || (initialBusy && !activationPackets)) && a==0xffb20000+busyAt){observed=true;nocReads++;return 1;}
   if(a==0xffb20210 && ++aliasReads==badAliasRead){observed=true;nocReads++;return Count(2)^1;}
  }
  uint32_t v=projection && Range(a,0x90e00,128)?memory.at(a):FfnInputPeer::Load(a);
  if(projection)for(unsigned i=0;i<16;i++)if(a==0xffb20000+BlackholeDramSourceOffset(i) && v!=descriptor[16+i])observed=true;
  if(projection && descriptorPacket && activationPackets==descriptorPacket && a==TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4)observed=true;
  if(projection && badCommandRead && commandReads==badCommandRead && Range(a,0xffb20000,40))observed=true;
  return v;
 }
 void Store(uint32_t a,uint32_t v)override{
  if(projection && Range(a,0xffb20000,0x2000))assert(!observed);
  if(projection && Range(a,0x90c00,640)){
   ClearTensorState();assert(!pending && !(a&3));
   if(a<0x90e00)nativeWrites++;else lineWrites++;
   memory[a]=v;return;
  }
  FfnInputPeer::Store(a,v);
 }
 void Stage(unsigned sequence,unsigned layer,unsigned role,unsigned phase=17,unsigned bank=0,bool edge=false){
  FfnPersistentPeer::StageActivation(sequence,phase,bank,edge);projection=true;lineWrites=0;observed=false;
  unsigned type=0,k=0,n=0;assert(PioneerQ38ProjectionGeometry(layer,role,type,k,n));
  auto& d=descriptor;d[0]=TT_Q38_PROJECTION_INPUT_MARKER;d[10]=d[5]*1024;d[11]=k;
  d[12]=TT_Q38_PROJECTION_NATIVE_OFFSET+d[5]*512;d[13]=TT_Q38_PROJECTION_LINES_OFFSET+d[5]*128;
  d[14]=type;d[15]=k/256;d[42]=layer;d[43]=role;
  for(unsigned i=0;i<64;i++)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=d[i];
  memory[0x74000]=TT_Q38_PROJECTION_INPUT_MARKER;memory[0x74034]=5;
  assert(PioneerQ38ProjectionInputShape(d.data()));
 }
 bool MatrixNext(unsigned sequence){projection=false;return FfnInputPeer::Matrix(sequence);}
 void Verify(){
  assert(activationPackets==3 && !activatedWrites && nativeWrites==128 && lineWrites==40);
  float values[256];for(unsigned i=0;i<256;i++)values[i]=Float(gate[i]);
  PioneerQ38ProjectionInput expected,host;ProjectionInputReference(descriptor[14],values,expected);
  assert(PioneerQ38ProjectionQuantize(descriptor[14],values,256,&host) && !memcmp(&host,&expected,sizeof(host)));
  for(unsigned i=0;i<128;i++)assert(native[i]==expected.operand[i]);
  for(unsigned i=0;i<8;i++)assert(line[i]==expected.scale[i]);
  assert(PioneerQ38ProjectionInputLine(line.data(),descriptor.data()));
  uint32_t proof[64],job[16],after[8];for(unsigned i=0;i<64;i++)proof[i]=memory.at(0x83f00+i*4);
  for(unsigned i=0;i<16;i++)job[i]=memory.at(0x74000+i*4);for(unsigned i=0;i<8;i++)after[i]=Count(i);
  auto okay=[&](){return PioneerQ38ProjectionInputProof(proof,job,descriptor.data(),source,logicalWorker,selfRoute,independentBefore,after);};
  assert(okay());for(unsigned i=0;i<64;i++){proof[i]^=1;assert(!okay());proof[i]^=1;}
  for(unsigned i=8;i<32;i++){line[i]^=1;assert(!PioneerQ38ProjectionInputLine(line.data(),descriptor.data()));line[i]^=1;}
 }
};
#ifndef PIONEER_Q38_PROJECTION_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);unsigned positive=0,faults=0,minStack=0x70000;
 for(unsigned bank=0;bank<7;bank++)assert(PioneerQ38ProjectionRoute(bank)==PioneerQ38SpanRoute(bank));
 for(unsigned chunk=0;chunk<68;chunk++)assert(PioneerQ38ProjectionPeer(chunk)==PioneerQ38FfnResidentPeer(chunk));
 const unsigned cases[][2]={{0,5},{0,13},{0,2},{0,4},{3,10},{64,1},{64,0}};
 for(const auto& selected:cases){
  unsigned type=0,k=0,n=0;assert(PioneerQ38ProjectionGeometry(selected[0],selected[1],type,k,n));
  std::vector<float> full(k);PioneerQ38CohortInput assembled;assembled.type=type;assembled.inputs=k;
  for(unsigned chunk=0;chunk<k/256;chunk++){
   ProjectionPeer p(argv[1],type,k,chunk+1);assert(p.BootNew());p.Stage(1,selected[0],selected[1],17,chunk%7,chunk&1);
   for(unsigned i=0;i<256;i++){float x=float(int((i*37+chunk*19)%255)-127)*.25f;if(!chunk)x=Float((i&1)<<31);p.gate[i]=Bits(x);full[chunk*256+i]=x;}
   assert(p.Finish(1));p.Verify();p.Inert();assert(p.Stop(2));minStack=std::min(minStack,p.minStack);
   memcpy(assembled.activation+chunk*128,p.native.data(),512);
   for(unsigned i=0;i<(type==8?8U:1U);i++)assembled.scales[chunk*(type==8?8:1)+i]=p.line[i];positive++;
  }
  PioneerQ38CohortInput expected;assert(PioneerQ38CohortQuantize(type,k,full.data(),expected) && !memcmp(&assembled,&expected,sizeof(assembled)));
  printf("PASS ACTUAL op5 layer%u role%u type%u input%u ALL chunks/native wire/scales;SIMULATED_ENDPOINTS_NOT_NATIVE_TIME\n",selected[0],selected[1],type,k);
 }
 for(const auto& selected:cases){
  unsigned type=0,k=0,n=0;assert(PioneerQ38ProjectionGeometry(selected[0],selected[1],type,k,n));
  ProjectionPeer p(argv[1],type,k,1);assert(p.BootNew());p.Stage(1,selected[0],selected[1]);assert(p.Finish(1));p.Verify();
  assert(p.MatrixNext(2));p.Stage(3,selected[0],selected[1],18);p.gate[255]^=0x80000000;assert(p.Finish(3));p.Verify();assert(p.Stop(4));positive++;
 }
 puts("PASS ACTUAL op5->existing type8/12/14 matrix on independent fixture operands->changed op5->STOP SAME lifetime/NO reset;COEXISTENCE_NOT_YET_ROOT_GATHER_PIPELINE");
 for(unsigned packet=1;packet<=3;packet++)for(unsigned fault=0;fault<5;fault++){
  ProjectionPeer p(argv[1],8,5120,1);assert(p.BootNew());p.Stage(1,0,5);
  if(fault==0)p.failPacket=packet;if(fault==1)p.badCounterPacket=packet;if(fault==2)p.foreignCounterPacket=packet;
  if(fault==3)p.sourcePacket=packet;if(fault==4)p.descriptorPacket=packet;
  assert(!p.Finish(1) && p.activationPackets==packet);p.Inert();faults++;
 }
 for(unsigned read=1;read<=30;read++){
  ProjectionPeer p(argv[1],8,5120,1);assert(p.BootNew());p.Stage(1,0,5);p.badCommandRead=read;
  assert(!p.Finish(1) && p.activationPackets==(read-1)/10);p.Inert();faults++;
 }
 for(unsigned packet:{1U,2U,3U})for(unsigned at:{0x840U,0x240U,0x2bcU}){
  ProjectionPeer p(argv[1],8,5120,1);assert(p.BootNew());p.Stage(1,0,5);p.busyPacket=packet;p.busyAt=at;
  assert(!p.Finish(1) && p.activationPackets==packet && p.observed);p.Inert();faults++;
 }
 for(unsigned read:{2U,4U,8U,12U,16U}){
  ProjectionPeer p(argv[1],8,5120,1);assert(p.BootNew());p.Stage(1,0,5);p.badAliasRead=read;
  assert(!p.Finish(1) && p.observed && p.aliasReads==read && p.activationPackets==(read<4?0U:read/4-1));p.Inert();faults++;
 }
 for(unsigned word=0;word<16;word++){
  ProjectionPeer p(argv[1],8,5120,1);assert(p.BootNew());p.Stage(1,0,5);p.source[word]^=1;
  assert(!p.Finish(1) && p.observed && !p.activationPackets);p.Inert();faults++;
 }
 for(unsigned at:{0x840U,0x240U,0x2bcU}){
  ProjectionPeer p(argv[1],8,5120,1);assert(p.BootNew());p.Stage(1,0,5);p.initialBusy=true;p.busyAt=at;
  assert(!p.Finish(1) && p.observed && !p.activationPackets);p.Inert();faults++;
 }
 for(unsigned word=0;word<16;word++){
  ProjectionPeer p(argv[1],8,5120,1);assert(p.BootNew());p.Stage(1,0,5);p.memory[0x74000+word*4]^=1;
  assert(!p.Finish(1) && !p.activationPackets);p.Inert();faults++;
 }
 for(unsigned word=0;word<64;word++){
  if(word>=32 && word<=43)continue; // identity/semantic DATA need native owner binding,not a fabricated peer grant
  ProjectionPeer p(argv[1],8,5120,1);assert(p.BootNew());p.Stage(1,0,5);p.memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+word*4]^=1;
  assert(!p.Finish(1) && !p.activationPackets);p.Inert();faults++;
 }
 for(unsigned type:{8U,12U,14U})for(unsigned lane:{0U,31U,224U,255U}){
  unsigned layer=type==14?3:0,role=type==8?5:type==12?2:10,k=type==14?6144:5120;
  ProjectionPeer p(argv[1],type,k,1);assert(p.BootNew());p.Stage(1,layer,role);p.gate[lane]=0x7fc00001;
  assert(!p.Finish(1) && p.activationPackets==1 && !p.nativeWrites && !p.lineWrites);p.Inert();faults++;
 }
 for(unsigned fault=0;fault<3;fault++){
  ProjectionPeer p(argv[1],8,5120,1);assert(p.BootNew());p.Stage(1,0,5);
  if(!fault)p.lateJob=true;if(fault==1)p.lateBoot=true;if(fault==2)p.lateDoorbell=true;
  assert(!p.Finish(1) && p.activationPackets==3);p.Inert();faults++;
 }
 // Old QFP2 must reject this op5 even though the boot marker is shared.
 FfnInputPeer old(argv[2],8,5120,1,1);assert(old.BootNew());old.StageInput(1);old.memory[0x74034]=5;
 assert(!old.Finish(1) && !old.activationPackets);old.Inert();faults++;
 printf("PASS ACTUAL new projection persistent peer positives%u/faults%u/maxDeviceStack%u;ALL native Q8_0/K bytes,128B identity lines,64wordproof corruption,UNKNOWN inert/no next packet,legacy op5 refused;EXPLICIT16KiB ROM_NOT_NATIVE12KiB_OWNER_GRANT_OR_MODEL_SPEED\n",positive,faults,0x70000-minStack);
}
#endif
