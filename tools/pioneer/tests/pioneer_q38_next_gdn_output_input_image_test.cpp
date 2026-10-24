/* SPDX-License-Identifier: MIT */
// Actual distinct op6 instructions;ONLY device/Tensix byte endpoints mocked.
// No native producer/coordinator or full projection/inference speed claim.
#ifndef PIONEER_Q38_NEXT_GDN_OUTPUT_REUSE_PROJECTION_PEER
#define PIONEER_Q38_PROJECTION_MACHINE_ONLY
#include "pioneer_q38_projection_persistent_image_test.cpp"
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextGdnOutputInputShape.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputInputShape.h"
struct NextOutputInputPeer:ProjectionPeer {
 NextOutputInputPeer(const char* path,unsigned worker):ProjectionPeer(path,8,6144,worker){}
 void StageGdn(unsigned sequence,unsigned phase=17,unsigned bank=0,bool edge=false,unsigned layer=1,unsigned position=0){
  FfnPersistentPeer::StageActivation(sequence,phase,bank,edge);projection=true;lineWrites=0;observed=false;
  auto& d=descriptor;d[0]=TT_Q38_NEXT_GDN_OUTPUT_INPUT_MARKER;d[10]=TT_Q38_GDN_GATED_OUTPUT+d[5]*1024;d[11]=6144;
  d[12]=TT_Q38_PROJECTION_NATIVE_OFFSET+d[5]*512;d[13]=TT_Q38_PROJECTION_LINES_OFFSET+d[5]*128;
  d[14]=8;d[15]=24;d[40]=position;d[41]=phase-1;d[42]=layer;d[43]=13;d[44]=TT_Q38_GDN_RENDEZVOUS_MARKER;d[45]=0;
  for(unsigned i=0;i<64;i++)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=d[i];
  memory[0x74000]=TT_Q38_NEXT_GDN_OUTPUT_INPUT_MARKER;memory[0x74034]=6;
  assert(PioneerQ38NextGdnOutputInputShape(d.data()) && !PioneerQ38GdnOutputInputShape(d.data()) && !PioneerQ38ProjectionInputShape(d.data()));
 }
 void VerifyGdn(){
  assert(activationPackets==3 && !activatedWrites && nativeWrites==128 && lineWrites==40);
  float values[256];for(unsigned i=0;i<256;i++)values[i]=Float(gate[i]);PioneerQ38ProjectionInput expected;
  ProjectionInputReference(8,values,expected);
  for(unsigned i=0;i<128;i++)assert(native[i]==expected.operand[i]);
  for(unsigned i=0;i<8;i++)assert(line[i]==expected.scale[i]);
  assert(PioneerQ38NextGdnOutputInputLine(line.data(),descriptor.data()) && !PioneerQ38ProjectionInputLine(line.data(),descriptor.data()));
  uint32_t proof[64],job[16],after[8];for(unsigned i=0;i<64;i++)proof[i]=memory.at(0x83f00+i*4);
  for(unsigned i=0;i<16;i++)job[i]=memory.at(0x74000+i*4);for(unsigned i=0;i<8;i++)after[i]=Count(i);
  auto okay=[&](){return PioneerQ38NextGdnOutputInputProof(proof,job,descriptor.data(),source,logicalWorker,selfRoute,independentBefore,after);};
  assert(okay());for(unsigned i=0;i<64;i++){proof[i]^=1;assert(!okay());proof[i]^=1;}
  for(unsigned i=8;i<32;i++){line[i]^=1;assert(!PioneerQ38NextGdnOutputInputLine(line.data(),descriptor.data()));line[i]^=1;}
 }
};
#ifndef PIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);unsigned positives=0,faults=0,minStack=0x70000;
 for(unsigned bank=0;bank<7;bank++)for(bool edge:{false,true}){
  std::vector<float> full(6144);PioneerQ38CohortInput assembled;assembled.type=8;assembled.inputs=6144;
  for(unsigned chunk=0;chunk<24;chunk++){
   NextOutputInputPeer p(argv[1],chunk+1);assert(p.BootNew());p.StageGdn(1,17,bank,edge);
   for(unsigned i=0;i<256;i++){
    float value=float(int((i*37+chunk*19+bank*11)%255)-127)*.25f;if(!chunk)value=Float((i&1)<<31);
    if(chunk==1)value=(i&1?-1:1)*127.f;if(chunk==2)value=float(int(i)-128)*0x1p-25f;if(edge && chunk>2)value=-value;
    p.gate[i]=Bits(value);full[chunk*256+i]=value;
   }
   assert(p.Finish(1));p.VerifyGdn();assert(p.Stop(2));minStack=std::min(minStack,p.minStack);
   memcpy(assembled.activation+chunk*128,p.native.data(),512);for(unsigned i=0;i<8;i++)assembled.scales[chunk*8+i]=p.line[i];positives++;
  }
  PioneerQ38CohortInput expected;assert(PioneerQ38CohortQuantize(8,6144,full.data(),expected) && !memcmp(&assembled,&expected,sizeof(expected)));
  printf("PASS ACTUAL gated6144 source ALL24chunks bank%u edge%u/full12288native bytes+192scales/strict QOI2 ancestry;SIMULATED_ENDPOINTS_NOT_NATIVE_TIME\n",bank,unsigned(edge));
 }
 for(unsigned worker:{1U,24U}){
  NextOutputInputPeer p(argv[1],worker);assert(p.BootNew());p.StageGdn(1);assert(p.Finish(1));p.VerifyGdn();
  assert(p.MatrixNext(2));p.StageGdn(3,18,6,false,62,8191);p.gate[255]^=0x80000000;assert(p.Finish(3));p.VerifyGdn();assert(p.Stop(4));positives++;
 }
 puts("PASS ACTUAL op6->Q8_0/6144 matrix coexistence->changed op6->STOP SAME lifetime;NO reset;ROOT_GATHER_PIPELINE_STILL_REQUIRED");
 for(unsigned packet=1;packet<=3;packet++)for(unsigned fault=0;fault<5;fault++){
  NextOutputInputPeer p(argv[1],1);assert(p.BootNew());p.StageGdn(1);
  if(!fault)p.failPacket=packet;if(fault==1)p.badCounterPacket=packet;if(fault==2)p.foreignCounterPacket=packet;
  if(fault==3)p.sourcePacket=packet;if(fault==4)p.descriptorPacket=packet;
  assert(!p.Finish(1) && p.activationPackets==packet);p.Inert();faults++;
 }
 for(unsigned read=1;read<=30;read++){
  NextOutputInputPeer p(argv[1],1);assert(p.BootNew());p.StageGdn(1);p.badCommandRead=read;
  assert(!p.Finish(1) && p.activationPackets==(read-1)/10);p.Inert();faults++;
 }
 for(unsigned packet:{1U,2U,3U})for(unsigned at:{0x840U,0x240U,0x2bcU}){
  NextOutputInputPeer p(argv[1],1);assert(p.BootNew());p.StageGdn(1);p.busyPacket=packet;p.busyAt=at;
  assert(!p.Finish(1) && p.activationPackets==packet && p.observed);p.Inert();faults++;
 }
 for(unsigned word=0;word<16;word++){
  NextOutputInputPeer p(argv[1],1);assert(p.BootNew());p.StageGdn(1);p.source[word]^=1;
  assert(!p.Finish(1) && p.observed && !p.activationPackets);p.Inert();faults++;
 }
 for(unsigned word=0;word<16;word++){
  NextOutputInputPeer p(argv[1],1);assert(p.BootNew());p.StageGdn(1);p.memory[0x74000+word*4]^=1;
  assert(!p.Finish(1) && !p.activationPackets);p.Inert();faults++;
 }
 for(unsigned word=0;word<64;word++){
  if((word>=16 && word<40) || word==40 || word==45)continue; // source/identity/token DATA still need actual native owner binding
  NextOutputInputPeer p(argv[1],1);assert(p.BootNew());p.StageGdn(1);p.memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+word*4]^=1;
  assert(!p.Finish(1) && !p.activationPackets);p.Inert();faults++;
 }
 for(unsigned worker:{1U,24U})for(unsigned lane:{0U,31U,224U,255U})for(uint32_t bad:{0x7f800000U,0xff800000U,0x7fc00001U}){
  NextOutputInputPeer p(argv[1],worker);assert(p.BootNew());p.StageGdn(1);p.gate[lane]=bad;
  assert(!p.Finish(1) && p.activationPackets==1 && !p.nativeWrites && !p.lineWrites);p.Inert();faults++;
 }
 for(unsigned fault=0;fault<3;fault++){
  NextOutputInputPeer p(argv[1],1);assert(p.BootNew());p.StageGdn(1);
  if(!fault)p.lateJob=true;if(fault==1)p.lateBoot=true;if(fault==2)p.lateDoorbell=true;
  assert(!p.Finish(1) && p.activationPackets==3);p.Inert();faults++;
 }
 for(unsigned op:{3U,4U,5U}){
  NextOutputInputPeer p(argv[1],1);assert(p.BootNew());p.StageGdn(1);p.memory[0x74034]=op;
  assert(!p.Finish(1) && !p.activationPackets);p.Inert();faults++;
 }
 {NextOutputInputPeer p(argv[1],1);assert(p.BootNew());p.StageGdn(1);p.memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+10*4]=0;
  assert(!p.Finish(1) && !p.activationPackets);p.Inert();faults++;}
 for(unsigned image:{2U,3U}){NextOutputInputPeer old(argv[image],1);assert(old.BootNew());old.StageGdn(1);assert(!old.Finish(1) && !old.activationPackets);old.Inert();faults++;}
 for(unsigned layer=1;layer<64;layer++)if(PioneerQ38NextGdnOutputLayer(layer))for(unsigned position:{0U,8191U}){
  NextOutputInputPeer p(argv[1],layer%24+1);assert(p.BootNew());p.StageGdn(1,17,layer%7,false,layer,position);
  assert(p.Finish(1));p.VerifyGdn();assert(p.Stop(2));positives++;
 }
 for(unsigned word:{40U,42U})for(unsigned invalid:{8192U,0xffffffffU}){
  NextOutputInputPeer p(argv[1],1);assert(p.BootNew());p.StageGdn(1);p.memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+word*4]=invalid;
  assert(!p.Finish(1) && !p.activationPackets);p.Inert();faults++;
 }
 for(unsigned invalid:{0U,3U,63U,64U}){
  NextOutputInputPeer p(argv[1],1);assert(p.BootNew());p.StageGdn(1);p.memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+42*4]=invalid;
  assert(!p.Finish(1) && !p.activationPackets);p.Inert();faults++;
 }
 printf("PASS ACTUAL distinct QOI2/op6 positives%u/faults%u/maxDeviceStack%u/ALL24gated offsets/native wire/identity+64proof mutations/fail-first UNKNOWN inert/old3-5 andoldpeer6 refused;NO native grant/ROOT gather/full projection/speed claim\n",positives,faults,0x70000-minStack);
}
#endif
