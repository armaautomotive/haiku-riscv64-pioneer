/* SPDX-License-Identifier: MIT */
// ACTUAL persistent producers -> simulated GDDR -> ACTUAL QFS2 root gather.
// No native root dispatch/owner/physical completion or model-speed claim.
#define PIONEER_Q38_FFN_INPUT_MACHINE_ONLY
#include "pioneer_q38_ffn_input_image_test.cpp"
#define PIONEER_Q38_FFN_GATHER_MACHINE_ONLY
#include "pioneer_q38_ffn_resident_gather_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGather2Shape.h"
struct Gather2Machine:GatherMachine {
 uint32_t independentBefore[8]={};
 Gather2Machine(const char* path,unsigned producers=20,unsigned bank=0,bool edge=false,unsigned layer=0)
  :GatherMachine(path,bank,edge,layer) {
  auto& d=descriptor;d[0]=TT_Q38_FFN_GATHER2_MARKER;d[5]=producers;d[10]=producers==20?20480:239744;
  d[12]=producers==20?30720:274560;d[13]=producers==20?10240:34816;d[14]=producers==20?128:320;
  SetDescriptor();assert(PioneerQ38FfnGather2Shape(d.data()));for(unsigned i=0;i<8;i++)independentBefore[i]=counts[i];
  for(unsigned block=0;block<producers;block++)if(producers==20)scaleLines[block][1]=TT_Q38_FFN_INPUT_MARKER;
 }
 void Store(uint32_t at,uint32_t value)override {
  if(Range(at,0xab000,320)){assert(Range(at,0xab000,descriptor[14]) && !(at&3));compactLocalWrites++;memory[at]=value;return;}
  if(at==0xffb20040){assert(value==1 && !pending);SubmitGather2();return;}
  GatherMachine::Store(at,value);
 }
 void SubmitGather2() {
  unsigned n=descriptor[5],chunks=n==20?1:3,total=n+chunks+1;assert(packets<total);unsigned p=packets++;
  bool write=p==total-1;unsigned local,remote,size;
  if(p<n){local=0x90e00;remote=descriptor[4]+descriptor[11]+p*64;size=64;}
  else if(!write){unsigned chunk=p-n;local=0xa0000+chunk*16384;remote=descriptor[4]+descriptor[10]+chunk*16384;
   size=n==20?10240:chunk==2?2048:16384;}
  else{local=0xab000;remote=descriptor[4]+descriptor[12];size=descriptor[14];}
  const uint32_t expected[]={write?local:remote,0,write?129U:descriptor[8],write?remote:local,0,write?descriptor[8]:129U,0,write?0x212U:0,size,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(packets==failPacket){pending=true;return;}assert(!activatedWrites && !operandWrites && !nativeScaleWrites);
  for(unsigned i=0;i<size/4;i++) {
   if(p<n)memory[local+i*4]=scaleLines[p][i];
   else if(!write){memory[local+i*4]=nativeOperands[(p-n)*4096+i];rootOperandReceived++;}
   else compact[i]=memory.at(local+i*4);
  }
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=size/64;counts[at+3]++;counts[2]++;counts[6]=counts[2];
  if(packets==badCounter)counts[at+3]++;if(packets==foreignCounter)counts[write?0:4]++;
  if(packets==sourceFault)seals[11]^=1;if(packets==descriptorFault)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4]^=1;
 }
 bool RunGather2() {
  while(pc!=0x64038 && pc!=0x64058)StepScalar();assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  if(pc==0x64058) {
   assert(memory.at(0x17ffc8)==TT_Q38_FFN_GATHER2_FAULT && !memory.at(0x17ffc4));
   unsigned old=packets,reads=nocReads,writes=commandWrites,local=compactLocalWrites;
   for(unsigned i=0;i<32;i++)StepScalar();assert(old==packets && reads==nocReads && writes==commandWrites && local==compactLocalWrites);return false;
  }
  assert(memory.at(0x17ffc8)==TT_Q38_FFN_GATHER2_MARKER && memory.at(0x17ffc0)==0x6400c);
  return memory.at(0x17ffc4)==descriptor[5];
 }
 void VerifyGather2() {
  unsigned n=descriptor[5],nativeWords=descriptor[13]/4,scaleWords=descriptor[14]/4;
  assert(packets==n+(n==20?1:3)+1 && !pending && rootOperandReceived==nativeWords && compactLocalWrites==scaleWords+n);
  for(unsigned i=0;i<nativeWords;i++)assert(memory.at(0xa0000+i*4)==nativeOperands[i]);
  for(unsigned i=0;i<scaleWords;i++)assert(compact[i]==(i<n?scaleLines[i][0]:0) && memory.at(0xab000+i*4)==compact[i]);
  for(unsigned i=scaleWords;i<80;i++)assert(compact[i]==0xdeadbeef && memory.at(0xab000+i*4)==0xdeadbeef);
  uint32_t s[32];for(unsigned i=0;i<32;i++)s[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  assert(PioneerQ38FfnGather2Summary(s,descriptor.data(),independentBefore,counts));
  for(unsigned i=0;i<32;i++){s[i]^=1;assert(!PioneerQ38FfnGather2Summary(s,descriptor.data(),independentBefore,counts));s[i]^=1;}
 }
};
int main(int argc,char** argv){assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);unsigned producers=0,gathers=0,faults=0,minStack=0x70000;
 for(unsigned n:{20U,68U})for(unsigned bank=0;bank<7;bank++)for(bool edge:{false,true}) {
  Gather2Machine root(argv[2],n,bank,edge,bank*9+unsigned(edge));std::vector<float> values(n*256);
  for(unsigned block=0;block<n;block++) {
   FfnInputPeer peer(argv[1],12,5120,1,block+1);assert(peer.BootNew());
   if(n==20)peer.StageInput(1,root.descriptor[6],bank,edge);else peer.StageActivation(1,root.descriptor[6],bank,edge);
   peer.descriptor[42]=root.descriptor[42];for(unsigned i=0;i<64;i++)peer.memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=peer.descriptor[i];
   assert(peer.Finish(1));if(n==20)peer.VerifyInput();else peer.VerifyActivation();
   for(unsigned i=0;i<256;i++)values[block*256+i]=Float(n==20?peer.gate[i]:peer.memory.at(0x90800+i*4));
   memcpy(root.nativeOperands.data()+block*128,peer.native.data(),512);memcpy(root.scaleLines[block].data(),peer.scale.data(),64);
   assert(peer.Stop(2));producers++;
  }
  assert(root.RunGather2());root.VerifyGather2();gathers++;minStack=std::min(minStack,root.minStack);
  PioneerQ38CohortInput expected;assert(PioneerQ38CohortQuantize(12,n*256,values.data(),expected));
  for(unsigned i=0;i<root.descriptor[13]/4;i++)assert(root.memory.at(0xa0000+i*4)==expected.activation[i]);
  for(unsigned i=0;i<root.descriptor[14]/4;i++)assert(root.memory.at(0xab000+i*4)==expected.scales[i]);
  printf("PASS ACTUAL QFP2 producers%u->GDDR->ACTUAL QFS2ROOT bank%u edge%u;native payload+compact scales+padding EXACT/no CPUintermediate products\n",n,bank,unsigned(edge));
 }
 for(unsigned n:{20U,68U})for(unsigned block=0;block<n;block++)for(unsigned word=0;word<16;word++) {
  Gather2Machine root(argv[2],n);if(!word)root.scaleLines[block][0]=0x7fc00001;else root.scaleLines[block][word]^=1;
  auto saved=root.compact;assert(!root.RunGather2() && root.packets==block+1 && !root.rootOperandReceived && root.compact==saved);faults++;
 }
 for(unsigned n:{20U,68U})for(unsigned p:{1U,n/2,n,n+1,n+(n==20?1:3)+1})for(unsigned fault=0;fault<5;fault++) {
  Gather2Machine root(argv[2],n);if(!fault)root.failPacket=p;if(fault==1)root.badCounter=p;if(fault==2)root.foreignCounter=p;
  if(fault==3)root.sourceFault=p;if(fault==4)root.descriptorFault=p;
  assert(!root.RunGather2() && root.packets==p);faults++;
 }
 for(unsigned n:{20U,68U})for(unsigned p:{1U,n,n+1,n+(n==20?1:3)+1})for(unsigned word=0;word<10;word++) {
  Gather2Machine root(argv[2],n);root.badCommandRead=(p-1)*10+word+1;assert(!root.RunGather2() && root.packets==p-1);faults++;
 }
 for(unsigned n:{20U,68U})for(unsigned source=0;source<16;source++){Gather2Machine root(argv[2],n);root.seals[source]^=1;assert(!root.RunGather2() && !root.packets);faults++;}
 for(unsigned n:{20U,68U})for(unsigned field:{0U,1U,2U,5U,7U,8U,9U,10U,11U,12U,13U,14U,15U}) {
  Gather2Machine root(argv[2],n);root.descriptor[field]^=1;root.SetDescriptor();assert(!root.RunGather2() && !root.packets && !root.nocReads);faults++;
 }
 for(unsigned n:{20U,68U})for(unsigned field=43;field<64;field++) {
  Gather2Machine root(argv[2],n);root.descriptor[field]=1;root.SetDescriptor();assert(!root.RunGather2() && !root.packets && !root.nocReads);faults++;
 }
 for(unsigned n:{20U,68U})for(unsigned fault=0;fault<13;fault++) {
  Gather2Machine root(argv[2],n);auto& d=root.descriptor;
  if(!fault)d[3]=7;if(fault==1)d[4]=0x04000001;if(fault==2)d[4]=0x03fff000;if(fault==3)d[4]=0xc0001000;
  if(fault==4)d[6]=0;if(fault==5)d[6]=0x7fffffff;if(fault==6)d[16]=0;if(fault==7)d[42]=64;
  if(fault>=8 && fault<12)d[32+(fault-8)*2]=d[33+(fault-8)*2]=0;if(fault==12)d[0]=TT_Q38_FFN_GATHER_MARKER;
  root.SetDescriptor();assert(!root.RunGather2() && !root.packets && !root.nocReads);faults++;
 }
 for(unsigned n:{20U,68U})for(unsigned offset:{0x40U,0x840U,0x1040U,0x1840U,0x240U,0x2bcU}) {
  Gather2Machine root(argv[2],n);root.busyOffset=offset;assert(!root.RunGather2() && !root.packets);faults++;
 }
 printf("PASS OFFLINE QFP2->QFS2 actual input/activation gathers:producers%u/roots%u/faults%u/rootMaxObservedDeviceStackBytes%u;ALL fresh tags BEFORE operand read/compact sink/ALL5120or17408 lanes exact;NO_NATIVE_LIFETIME_COORDINATOR_MODEL_OR_SPEED_AUTHORITY\n",producers,gathers,faults,0x70000-minStack);
}
