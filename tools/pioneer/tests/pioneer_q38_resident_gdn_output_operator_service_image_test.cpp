/* SPDX-License-Identifier: MIT */
// ACTUAL emitted op6+matrix+GDN instructions, same tensor arrays/lifetime.
// Byte endpoints/time only are synthetic. This is NOT native throughput.
#define PIONEER_Q38_RESIDENT_GDN_OPERATOR_SERVICE_MACHINE_ONLY
#include "pioneer_q38_resident_gdn_operator_service_image_test.cpp"
#include "pioneer_q38_projection_input_reference.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnOutputShape.h"
struct ResidentGdnOutputPeer:ResidentGdnOperatorPeer {
 bool outputActive=false;
 std::array<uint32_t,32> line={};
 unsigned lineWrites=0,descriptorWord=40,busyPacket=0,busyOffset=0;
 unsigned receiptCounterPacket=0,receiptCommandPacket=0,receiptReads=0,packetCommandReads=0;
 ResidentGdnOutputPeer(const char* path,unsigned worker=1,unsigned panels=1)
  :ResidentGdnOperatorPeer(path,worker,8,6144,panels){
  line.fill(0xa5a5a5a5);
  // An op6->op7 job-bit corruption reaches the existing Conv entry, which
  // snapshots this exact fixed local table before rejecting its job tag.
  // Model that already-used mailbox explicitly, never arbitrary L1 reads.
  for(unsigned i=0;i<384;i++)memory[TT_Q38_CONV_BATCH_TABLE+i*4]=0x7fc00000+i;
 }
 uint32_t Load(uint32_t at)override{
  if(outputActive){
   if(Range(at,TT_Q38_CONV_BATCH_TABLE,1536))return memory.at(at);
   if(Range(at,0x90000,0xe80))return memory.at(at);
   if(busyPacket && activationPackets==busyPacket && at==0xffb20000+busyOffset){nocReads++;return 1;}
   if(Range(at,0xffb20000,40)){
    nocReads++;unsigned n=++receiptReads;uint32_t value=command[(at-0xffb20000)/4];
    if(n==badCommandRead)value^=1;
    if(activationPackets && ++packetCommandReads==10){
     if(receiptCounterPacket==activationPackets)countBase[4]++;
     if(receiptCommandPacket==activationPackets)command[1]^=1;
    }
    return value;
   }
  }
  return ResidentGdnOperatorPeer::Load(at);
 }
 void SubmitActivation()override{
  if(!outputActive){FfnPersistentPeer::SubmitActivation();return;}
  ClearTensorState();assert(activationPackets<3);unsigned p=activationPackets++;bool write=p!=0;
  const unsigned local[]={0x90000,0x90c00,0x90e00},bytes[]={1024,512,128},index[]={10,12,13};
  unsigned remote=descriptor[4]+descriptor[index[p]];
  const uint32_t expected[]={write?local[p]:remote,0,write?selfRoute:descriptor[8],write?remote:local[p],0,
   write?descriptor[8]:selfRoute,0,write?0x212U:0,bytes[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  packetCommandReads=0;
  if(activationPackets==failPacket){pending=true;return;}
  assert(!activatedWrites);if(!write)assert(!nativeWrites && !lineWrites);else assert(nativeWrites==128 && lineWrites==40);
  for(unsigned i=0;i<bytes[p]/4;i++){
   if(!p){assert(memory.at(local[p]+i*4)==0x7fc90000+i);memory[local[p]+i*4]=gate[i];}
   if(p==1)native[i]=memory.at(local[p]+i*4);
   if(p==2)line[i]=memory.at(local[p]+i*4);
  }
  unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=bytes[p]/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
  if(activationPackets==badCounterPacket)countBase[at+3]++;
  if(activationPackets==foreignCounterPacket)countBase[write?0:4]++;
  if(activationPackets==sourcePacket)source[11]^=1;
  if(activationPackets==descriptorPacket)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+descriptorWord*4]^=1;
  if(lateJob && activationPackets==3)memory[0x74000+4]^=1;
  if(lateBoot && activationPackets==3)memory[0x74040+4]^=1;
  if(lateDoorbell && activationPackets==3)memory[0x74100]++;
 }
 void Store(uint32_t at,uint32_t value)override{
  if(outputActive && (Range(at,0x90000,1024) || Range(at,0x90c00,640))){
   ClearTensorState();assert(!pending && !(at&3));
   if(Range(at,0x90c00,512))nativeWrites++;
   if(Range(at,0x90e00,128))lineWrites++;
   memory[at]=value;return;
  }
  ResidentGdnOperatorPeer::Store(at,value);
 }
 void StageOutput(unsigned sequence,unsigned layer=0,unsigned position=0,unsigned phase=19,unsigned bank=0,bool edge=false){
  ClearTensorState();gdnActive=convActive=false;FfnPersistentPeer::StageActivation(sequence,phase,bank,edge);
  outputActive=true;lineWrites=receiptReads=packetCommandReads=0;
  auto& d=descriptor;d[0]=TT_Q38_RESIDENT_GDN_OUTPUT_MARKER;d[10]=TT_Q38_GDN_GATED_OUTPUT+d[5]*1024;d[11]=6144;
  d[12]=TT_Q38_PROJECTION_NATIVE_OFFSET+d[5]*512;d[13]=TT_Q38_PROJECTION_LINES_OFFSET+d[5]*128;
  d[14]=8;d[15]=24;d[40]=position;d[41]=phase-1;d[42]=layer;d[43]=13;d[44]=TT_Q38_GDN_RENDEZVOUS_MARKER;d[45]=248319;
  for(unsigned i=0;i<64;i++)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=d[i];
  memory[0x74000]=TT_Q38_RESIDENT_GDN_OUTPUT_MARKER;memory[0x74034]=6;
 }
 void VerifyOutput(){
  assert(activationPackets==3 && !activatedWrites && nativeWrites==128 && lineWrites==40 && receiptReads==90);
  float values[256];for(unsigned i=0;i<256;i++)values[i]=Float(gate[i]);PioneerQ38ProjectionInput expected,host;
  ProjectionInputReference(8,values,expected);
  assert(PioneerQ38ProjectionQuantize(8,values,256,&host) && !memcmp(&host,&expected,sizeof(host)));
  for(unsigned i=0;i<128;i++)assert(native[i]==expected.operand[i]);
  for(unsigned i=0;i<8;i++)assert(line[i]==expected.scale[i]);
  assert(PioneerQ38ResidentGdnOutputLine(line.data(),descriptor.data()) && !PioneerQ38GdnOutputInputLine(line.data(),descriptor.data()));
  for(unsigned i=8;i<32;i++){line[i]^=1;assert(!PioneerQ38ResidentGdnOutputLine(line.data(),descriptor.data()));line[i]^=1;}
  uint32_t proof[64],job[16],after[8];
  for(unsigned i=0;i<64;i++)proof[i]=memory.at(0x83f00+i*4);
  for(unsigned i=0;i<16;i++)job[i]=memory.at(0x74000+i*4);
  for(unsigned i=0;i<8;i++)after[i]=Count(i);
  auto valid=[&](){return PioneerQ38ResidentGdnOutputProof(proof,job,descriptor.data(),source,logicalWorker,selfRoute,independentBefore,after);};
  assert(valid());for(unsigned i=0;i<64;i++){proof[i]^=1;assert(!valid());proof[i]^=1;}
 }
 bool MatrixAfterOutput(unsigned sequence){outputActive=false;return MatrixAfterGdn(sequence);}
 void OutputInert(){unsigned reads=nocReads,packets=activationPackets,math=matrixInstructions,writes=nativeWrites+lineWrites;
  for(unsigned i=0;i<64;i++)StepBlock();assert(reads==nocReads && packets==activationPackets && math==matrixInstructions && writes==nativeWrites+lineWrites);}
};
#ifndef PIONEER_Q38_RESIDENT_GDN_OUTPUT_SERVICE_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--smoke")));setvbuf(stdout,nullptr,_IOLBF,0);bool smoke=argc==4;
 unsigned positives=0,faults=0,minStack=0x70000;
 // All 24 chunks for every recurrent layer, both legal token-position
 // boundaries, all banks/base edges, exact independently packed Q8_0.
 for(unsigned layer=0;layer<64;layer++){
  if(layer%4==3 || (smoke && layer))continue;
  for(unsigned chunk=0;chunk<24;chunk++){
   ResidentGdnOutputPeer m(argv[1],chunk+1);assert(m.BootNew());
   for(unsigned token=0;token<2;token++){
    m.StageOutput(token+1,layer,token?8191:0,19+token,(layer+chunk)%7,token!=0);
    for(unsigned i=0;i<256;i++){
     float x=float(int((i*37+chunk*19+layer*11)%255)-127)*.25f;
     if(!chunk)x=Float((i&1)<<31);if(chunk==1)x=(i&1?-1:1)*127.f;
     if(chunk==2)x=float(int(i)-128)*0x1p-25f;if(token && chunk>2)x=-x;
     m.gate[i]=Bits(x);
    }
    assert(m.Finish(token+1));m.VerifyOutput();m.OutputInert();positives++;
   }
   assert(m.Stop(3));minStack=std::min(minStack,m.minStack);
  }
  printf("PASS ACTUAL QOL1 layer%u ALL24chunks/two positions/changed operands/monotonic lifetime/no reset/independent native bytes+scales\n",layer);
 }
 // Four real GDN halves produce the complete 256 lanes consumed by op6.
 // No CPU result or precomputed gate value is fed into this transition.
 // A subsequent K6144 matrix uses the SAME config/thread/LRegs/Dst state;
 // only its untested remaining chunks are explicit synthetic wire inputs.
 for(unsigned layer:{0U,30U,62U}){
  if(smoke && layer)continue;
  std::array<std::unique_ptr<ResidentGdnOutputPeer>,4> p;
  for(unsigned i=0;i<4;i++){
   p[i]=std::make_unique<ResidentGdnOutputPeer>(argv[1],i+1,2);assert(p[i]->BootNew());
   p[i]->StageGdn(1,17,layer,layer%7,layer?8191:0,1,2);
  }
  for(unsigned i=0;i<4;i+=2){assert(RunPersistentGdnPair(*p[i],*p[i+1],1));p[i]->VerifyGdn(1);p[i+1]->VerifyGdn(1);}
  auto& m=*p[0];
  auto bytes=[](const auto& value){const uint8_t* first=reinterpret_cast<const uint8_t*>(&value);return std::vector<uint8_t>(first,first+sizeof(value));};
  auto cfg=bytes(m.config),lr=bytes(m.lr),threads=bytes(m.thread),dst=bytes(m.physical),integers=bytes(m.dstInt);
  m.StageOutput(2,layer,layer?8191:0,18,layer%7);
  for(unsigned i=0;i<256;i++)m.gate[i]=p[i/64]->gatedOutput[i%64];
  assert(m.Finish(2));m.VerifyOutput();
  assert(cfg==bytes(m.config) && lr==bytes(m.lr) && threads==bytes(m.thread) && dst==bytes(m.physical) && integers==bytes(m.dstInt));
  for(unsigned i=0;i<256;i++){
   uint32_t word=m.native[i/2];m.vector[i]=uint16_t(word>>((i%2)*16));m.root[0xa0000+i/2*4]=word;
  }
  for(unsigned i=0;i<8;i++){m.scales[i]=m.line[i];m.root[0xab000+i*4]=m.line[i];}
  assert(m.MatrixAfterOutput(3));m.StageOutput(4,layer,1,20,6);assert(m.Finish(4));m.VerifyOutput();
  assert(m.Stop(5));for(unsigned i=1;i<4;i++)assert(p[i]->Stop(2));
  minStack=std::min(minStack,m.minStack);positives++;
  printf("PASS ACTUAL 4GDNhalves->op6 native wire->K6144/Q8 matrix->changed op6/STOP SAME config/thread/LRegs/Dst lifetime layer%u;partial synthetic matrix NOT full projection\n",layer);
 }
 if(smoke){printf("PASS SMOKE actual QOL1 positives%u/maxStack%u;NO native/fullmodel/speed grant\n",positives,0x70000-minStack);return 0;}
 puts("CHECK QOL1 packet loss/counter/source/descriptor faults");
 for(unsigned packet=1;packet<=3;packet++)for(unsigned kind=0;kind<5;kind++){
  ResidentGdnOutputPeer m(argv[1]);assert(m.BootNew());m.StageOutput(1);
  if(!kind)m.failPacket=packet;if(kind==1)m.badCounterPacket=packet;if(kind==2)m.foreignCounterPacket=packet;
  if(kind==3)m.sourcePacket=packet;if(kind==4)m.descriptorPacket=packet;
  assert(!m.Finish(1) && m.activationPackets==packet);m.OutputInert();faults++;
 }
 for(unsigned packet:{1U,2U})for(unsigned kind=0;kind<2;kind++){
  ResidentGdnOutputPeer m(argv[1]);assert(m.BootNew());m.StageOutput(1);
  if(kind)m.receiptCommandPacket=packet;else m.receiptCounterPacket=packet;
  assert(!m.Finish(1) && m.activationPackets==packet);m.OutputInert();faults++;
 }
 puts("CHECK QOL1 ALL90 command readbacks");
 for(unsigned read=1;read<=90;read++){
  ResidentGdnOutputPeer m(argv[1]);assert(m.BootNew());m.StageOutput(1);m.badCommandRead=read;
  assert(!m.Finish(1));m.OutputInert();faults++;
 }
 for(unsigned word=0;word<64;word++){
  ResidentGdnOutputPeer m(argv[1]);assert(m.BootNew());m.StageOutput(1);m.descriptorPacket=1;m.descriptorWord=word;
  assert(!m.Finish(1) && m.activationPackets==1 && !m.nativeWrites && !m.lineWrites);m.OutputInert();faults++;
 }
 puts("CHECK QOL1 ALL job/source bit corruptions, including op6->op7 discrimination");
 for(unsigned word=0;word<16;word++)for(unsigned kind=0;kind<2;kind++){
  ResidentGdnOutputPeer m(argv[1]);assert(m.BootNew());m.StageOutput(1);
  if(kind)m.source[word]^=1;else m.memory[0x74000+word*4]^=1;
  assert(!m.Finish(1) && !m.activationPackets);m.OutputInert();faults++;
 }
 for(unsigned packet:{1U,2U,3U})for(unsigned at:{0x840U,0x240U,0x2bcU}){
  ResidentGdnOutputPeer m(argv[1]);assert(m.BootNew());m.StageOutput(1);m.busyPacket=packet;m.busyOffset=at;
  assert(!m.Finish(1) && m.activationPackets==packet);m.OutputInert();faults++;
 }
 puts("CHECK QOL1 late job/boot/doorbell and non-finite/unsupported-scale operands");
 for(unsigned kind=0;kind<3;kind++){
  ResidentGdnOutputPeer m(argv[1]);assert(m.BootNew());m.StageOutput(1);
  if(!kind)m.lateJob=true;if(kind==1)m.lateBoot=true;if(kind==2)m.lateDoorbell=true;
  assert(!m.Finish(1) && m.activationPackets==3);m.OutputInert();faults++;
 }
 for(unsigned lane:{0U,31U,224U,255U})for(uint32_t bad:{1U,0x00800000U,0x7f7fffffU,0x7f800000U,0xff800000U,0x7fc00001U}){
  ResidentGdnOutputPeer m(argv[1]);assert(m.BootNew());m.StageOutput(1);m.gate.fill(bad);m.gate[lane]=bad;
  assert(!m.Finish(1) && m.activationPackets==1 && !m.nativeWrites && !m.lineWrites);m.OutputInert();faults++;
 }
 for(unsigned layer:{3U,63U,64U}){
  ResidentGdnOutputPeer m(argv[1]);assert(m.BootNew());m.StageOutput(1,layer);
  assert(!m.Finish(1) && !m.activationPackets);m.OutputInert();faults++;
 }
 {ResidentGdnOutputPeer m(argv[1]);assert(m.BootNew());m.StageOutput(1,0,8192);assert(!m.Finish(1) && !m.activationPackets);m.OutputInert();faults++;}
 {ResidentGdnOperatorPeer old(argv[2]);ResidentGdnOutputPeer data(argv[1]);assert(old.BootNew());data.StageOutput(1);
  for(unsigned i=0;i<16;i++)old.memory[0x74000+i*4]=data.memory.at(0x74000+i*4);old.memory[0x74100]=1;
  assert(!old.Finish(1) && !old.requests && !old.activationPackets && !old.gdnPackets && !old.matrixInstructions);old.GdnInert();faults++;}
 {ResidentGdnOutputPeer m(argv[1]);assert(m.BootNew());m.StageOutput(1);m.memory[0x74000]=TT_Q38_GDN_OUTPUT_INPUT_MARKER;
  assert(!m.Finish(1) && !m.activationPackets);m.OutputInert();faults++;}
 printf("PASS ACTUAL QOL1 positives%u/faults%u/maxStack%u/ALL48layers+24chunks+2positions/independent Q8 wire/64proof+64late-descriptor mutations/counter-chain+command-readback/UNKNOWN inert/old op6 refused;NO native/ROOT/fullmodel or silicon speed grant\n",positives,faults,0x70000-minStack);
}
#endif
