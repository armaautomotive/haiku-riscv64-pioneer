/* SPDX-License-Identifier: MIT */
// ACTUAL QFP2 input quantizer+matrix+activation instructions. No native
// coordinator/producer ownership or speed claim;NoC/GDDR endpoints simulated.
#define PIONEER_Q38_FFN_PERSISTENT_MACHINE_ONLY
#include "pioneer_q38_ffn_persistent_image_test.cpp"
#include "../PioneerQwen38CohortHost.h"
struct FfnInputPeer:FfnPersistentPeer {
 using FfnPersistentPeer::FfnPersistentPeer;
 bool input=false;
 void SubmitActivation()override {
  if(!input){FfnPersistentPeer::SubmitActivation();return;}
  ClearTensorState();assert(activationPackets<3);unsigned p=activationPackets++;bool write=p!=0;
  const unsigned local[]={0x90000,0x90c00,0x90e00},bytes[]={1024,512,64},index[]={10,12,13};
  unsigned remote=descriptor[4]+descriptor[index[p]];
  const uint32_t expected[]={write?local[p]:remote,0,write?descriptor[9]:descriptor[8],write?remote:local[p],0,
   write?descriptor[8]:descriptor[9],0,write?0x212U:0,bytes[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(activationPackets==failPacket){pending=true;return;}
  assert(!activatedWrites);if(!write)assert(!nativeWrites && !scaleWrites);else assert(nativeWrites==128 && scaleWrites==31);
  for(unsigned i=0;i<bytes[p]/4;i++) {
   if(p==0)memory[local[p]+i*4]=gate[i];if(p==1)native[i]=memory.at(local[p]+i*4);if(p==2)scale[i]=memory.at(local[p]+i*4);
  }
  unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=bytes[p]/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
  if(activationPackets==badCounterPacket)countBase[at+3]++;
  if(activationPackets==foreignCounterPacket)countBase[write?0:4]++;
  if(activationPackets==sourcePacket)source[11]^=1;
  if(activationPackets==descriptorPacket)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4]^=1;
  if(p==2){if(lateJob)memory[0x74000+4]^=1;if(lateBoot)memory[0x74040+4]^=1;if(lateDoorbell)memory[0x74100]++;}
 }
 void StageActivation(unsigned sequence,unsigned phase=17,unsigned bank=0,bool edge=false) {
  input=false;FfnPersistentPeer::StageActivation(sequence,phase,bank,edge);
 }
 bool Matrix(unsigned sequence){input=false;return FfnPersistentPeer::Matrix(sequence);}
 void StageInput(unsigned sequence,unsigned phase=17,unsigned bank=0,bool edge=false) {
  FfnPersistentPeer::StageActivation(sequence,phase,bank,edge);input=true;
  auto& d=descriptor;d[0]=TT_Q38_FFN_INPUT_MARKER;d[10]=d[5]*1024;d[11]=0;d[12]=20480+d[5]*512;
  for(unsigned i=0;i<64;i++)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=d[i];
  memory[0x74000]=TT_Q38_FFN_INPUT_MARKER;memory[0x74034]=TT_Q38_FFN_PERSISTENT_INPUT;
 }
 void VerifyInput() {
  assert(activationPackets==3 && !activatedWrites && nativeWrites==128 && scaleWrites==31);
  float values[256];uint32_t q[73];for(unsigned i=0;i<256;i++)values[i]=Float(gate[i]);Q38ActivationReference(values,q);
  for(unsigned i=0;i<128;i++) {
   uint32_t word=0;for(unsigned lane=0;lane<2;lane++){unsigned at=i*2+lane,byte=(q[1+at/4]>>((at%4)*8))&255;
    int value=int(byte)-int((byte&128)*2);word|=(0x4000U|(value<0?0x8000U|unsigned(-value):unsigned(value)))<<(lane*16);}
   assert(native[i]==word);
  }
  assert(scale[0]==q[0] && PioneerQ38FfnInputScale(scale.data(),descriptor.data(),logicalWorker-1));
  uint32_t proof[64],job[16],after[8];for(unsigned i=0;i<64;i++)proof[i]=memory.at(0x83f00+i*4);
  for(unsigned i=0;i<16;i++)job[i]=memory.at(0x74000+i*4);for(unsigned i=0;i<8;i++)after[i]=Count(i);
  auto valid=[&](){return PioneerQ38FfnPersistentInputProof(proof,job,descriptor.data(),source,logicalWorker,selfRoute,independentBefore,after);};
  assert(valid());for(unsigned i=0;i<64;i++){proof[i]^=1;assert(!valid());proof[i]^=1;}
  for(unsigned i=0;i<16;i++){auto saved=scale[i];scale[i]^=1;assert(!PioneerQ38FfnInputScale(scale.data(),descriptor.data(),logicalWorker-1) || !i);scale[i]=saved;}
 }
};
#ifndef PIONEER_Q38_FFN_INPUT_MACHINE_ONLY
int main(int argc,char** argv){assert(argc==2 || argc==3);setvbuf(stdout,nullptr,_IOLBF,0);unsigned positives=0,faults=0,minStack=0x70000;
 for(unsigned bank=0;bank<7;bank++)for(bool edge:{false,true}) {
  float values[5120];PioneerQ38CohortInput assembled;assembled.type=12;assembled.inputs=5120;
  for(unsigned block=0;block<20;block++) {
   FfnInputPeer m(argv[1],12,5120,1,block+1);assert(m.BootNew());m.StageInput(1,17,bank,edge);
   for(unsigned i=0;i<256;i++) {
    // Changes per bank/edge/block;includes ties,first-signed-max ties,
    // normal exponent spread,signed zero and complete all-zero blocks.
    float v=float(int((i*37+block*19)%255)-127)*float(bank+1)*0.25f;
    if(block==0)v=Float((i&1)<<31);if(block==1)v=(i&1?-1:1)*127.0f;
    if(block==2)v=float(int(i)-128)*0x1p-20f;if(edge && block>2)v=-v;
    m.gate[i]=Bits(v);values[block*256+i]=v;
   }
   assert(m.Finish(1));m.VerifyInput();m.Inert();assert(m.Stop(2));
   memcpy(assembled.activation+block*128,m.native.data(),512);assembled.scales[block]=m.scale[0];
   minStack=std::min(minStack,m.minStack);positives++;
  }
  PioneerQ38CohortInput expected;assert(PioneerQ38CohortQuantize(12,5120,values,expected));
  assert(!memcmp(assembled.activation,expected.activation,sizeof(expected.activation)) && !memcmp(assembled.scales,expected.scales,sizeof(expected.scales)));
  printf("PASS ACTUAL input ALL20blocks/5120lanes bank%u edge%u;complete native payload+scale padding equal independent host wire\n",bank,unsigned(edge));
 }
 for(unsigned type:{12U,14U})for(unsigned worker:{1U,20U}) {
  FfnInputPeer m(argv[1],type,5120,1,worker);assert(m.BootNew());m.StageInput(1);assert(m.Finish(1));m.VerifyInput();
  assert(m.Matrix(2));m.StageActivation(3,18);assert(m.Finish(3));m.VerifyActivation();FreshInputs(m);assert(m.Matrix(4));
  m.StageInput(5,19);for(unsigned i=0;i<256;i++)m.gate[i]=Bits(float(int(i)-128)*.5f);
  assert(m.Finish(5));m.VerifyInput();assert(m.Stop(6));minStack=std::min(minStack,m.minStack);positives++;
 }
 puts("PASS ACTUAL input/matrix/activation/matrix/changed-input/STOP,Q4_K/Q6_K,NO reset");
 for(uint32_t magnitude:{0x2f800000U,0x47800000U})for(bool negative:{false,true}) {
  FfnInputPeer m(argv[1]);assert(m.BootNew());m.StageInput(1);
  float selected=Float(magnitude)*127.0f;for(unsigned i=0;i<256;i++)m.gate[i]=Bits((negative?-1:1)*selected);
  assert(m.Finish(1));m.VerifyInput();assert((m.scale[0]&0x7fffffff)==magnitude && m.Stop(2));positives++;
 }
 for(unsigned fault=0;fault<13;fault++) {
  FfnInputPeer m(argv[1]);assert(m.BootNew());m.StageInput(1);auto& d=m.descriptor;
  if(!fault)d[3]=7;if(fault==1)d[4]=0x04000001;if(fault==2)d[4]=0x03fff000;if(fault==3)d[4]=0xc0001000;
  if(fault==4)d[6]=0;if(fault==5)d[6]=0x7fffffff;if(fault==6)d[16]=0;if(fault==7)d[42]=64;
  if(fault>=8 && fault<12)d[32+(fault-8)*2]=d[33+(fault-8)*2]=0;if(fault==12)d[5]=20;
  for(unsigned i=0;i<64;i++)m.memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=d[i];
  assert(!m.Finish(1) && !m.activationPackets);m.Inert();faults++;
 }
 for(unsigned p=1;p<=3;p++)for(unsigned fault=0;fault<5;fault++) {
  FfnInputPeer m(argv[1]);assert(m.BootNew());m.StageInput(1);
  if(!fault)m.failPacket=p;if(fault==1)m.badCounterPacket=p;if(fault==2)m.foreignCounterPacket=p;if(fault==3)m.sourcePacket=p;if(fault==4)m.descriptorPacket=p;
  assert(!m.Finish(1) && m.activationPackets==p);m.Inert();faults++;
 }
 for(unsigned word=1;word<=30;word++){FfnInputPeer m(argv[1]);assert(m.BootNew());m.StageInput(1);m.badCommandRead=word;
  assert(!m.Finish(1) && m.activationPackets==(word-1)/10);m.Inert();faults++;}
 for(unsigned word=0;word<16;word++){FfnInputPeer m(argv[1]);assert(m.BootNew());m.StageInput(1);m.memory[0x74000+word*4]^=1;
  assert(!m.Finish(1) && !m.activationPackets);m.Inert();faults++;}
 for(unsigned word=0;word<64;word++){if(word>=32 && word<=42)continue;
  FfnInputPeer m(argv[1]);assert(m.BootNew());m.StageInput(1);m.memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+word*4]^=1;
  assert(!m.Finish(1) && !m.activationPackets);m.Inert();faults++;}
 for(unsigned source=0;source<16;source++){FfnInputPeer m(argv[1]);assert(m.BootNew());m.StageInput(1);m.source[source]^=1;
  assert(!m.Finish(1) && !m.activationPackets);m.Inert();faults++;}
 for(unsigned fault=0;fault<3;fault++){FfnInputPeer m(argv[1]);assert(m.BootNew());m.StageInput(1);
  if(!fault)m.lateJob=true;if(fault==1)m.lateBoot=true;if(fault==2)m.lateDoorbell=true;
  assert(!m.Finish(1) && m.activationPackets==3);m.Inert();faults++;}
 for(unsigned worker:{21U,68U,119U}){FfnInputPeer m(argv[1],12,5120,1,worker);assert(m.BootNew());m.StageInput(1);
  assert(!m.Finish(1) && !m.activationPackets);m.Inert();faults++;}
 for(uint32_t marker:{0x32503851U,0x31504651U})for(unsigned mode=0;mode<2;mode++) {
  FfnInputPeer m(argv[1]);if(!mode){m.memory[0x74040]=marker;assert(!m.BootNew());}
  else{assert(m.BootNew());m.StageInput(1);m.memory[0x74028]=marker;assert(!m.Finish(1));}
  assert(!m.activationPackets);m.Inert();faults++;
 }
 for(uint32_t bad:{1U,0x007fffffU,0x00800000U,0x7f7fffffU,0x7f800000U,0x7fc00001U}) {
  FfnInputPeer m(argv[1]);assert(m.BootNew());m.StageInput(1);m.gate.fill(bad);
  assert(!m.Finish(1) && m.activationPackets==1 && !m.nativeWrites && !m.scaleWrites);m.Inert();faults++;
 }
 if(argc==3){DirectSrcBImage old(argv[2],12,5120,1,1);assert(old.Boot() && !old.Job(1,4) && !old.requests && !old.matrixInstructions);faults++;}
 printf("PASS OFFLINE ACTUAL QFP2 initial quantizer:positive%u/faults%u/maxObservedDeviceStackBytes%u;ALL5120lanes/native bytes/all64proof corruption/UNKNOWN inert;NO_NATIVE_OWNER_COORDINATOR_MODEL_OR_SPEED_QUALIFICATION\n",positives,faults,0x70000-minStack);
}
#endif
