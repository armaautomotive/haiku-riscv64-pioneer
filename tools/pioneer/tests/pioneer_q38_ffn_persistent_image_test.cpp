/* SPDX-License-Identifier: MIT */
// ACTUAL persistent matrix/activation instructions. Only NoC/GDDR endpoints
// simulated. No native root/controller/producer authority or timing claim.
#define PIONEER_Q38_DIRECT_SRCB_IMAGE_MODEL_ONLY
#include "pioneer_q38_direct_srcb_image_test.cpp"
#include "pioneer_qwen38_activation_reference.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnPersistentShape.h"
#include <array>
struct FfnPersistentPeer:DirectSrcBImage {
 const unsigned ownedPrefix;
 std::vector<uint8_t> activationCode;
 std::array<uint32_t,64> descriptor={};
 std::array<uint32_t,256> gate={},up={};
 std::array<uint32_t,128> native={};std::array<uint32_t,16> scale={};
 uint32_t matrixJob[16]={},source[16]={},countBase[8]={},independentBefore[8]={};
 bool activation=false;
 unsigned activationPackets=0,failPacket=0,badCounterPacket=0,foreignCounterPacket=0,sourcePacket=0,descriptorPacket=0;
 unsigned nocReads=0,commandReads=0,badCommandRead=0,normalizationWords=0,normalizations=0;
 unsigned activatedWrites=0,nativeWrites=0,scaleWrites=0;
 bool lateJob=false,lateBoot=false,lateDoorbell=false;
 FfnPersistentPeer(const char* path,unsigned type=12,unsigned k=5120,unsigned groups=1,unsigned worker=1,unsigned prefix=12288)
  :DirectSrcBImage(path,type,k,groups,worker),ownedPrefix(prefix),activationCode(Read((std::string(path)+"/ffn.bin").c_str())) {
  assert((ownedPrefix==12288 || ownedPrefix==16384) && !activationCode.empty() && activationCode.size()<=ownedPrefix && !(activationCode.size()&3));
  for(unsigned w=0;w<activationCode.size()/4;w++)l1[Index(0x64000+w*4)]=Word(activationCode,w*4);
  memory[0x74040]=TT_Q38_FFN_PERSISTENT_MARKER;
  for(unsigned i=0;i<16;i++){matrixJob[i]=memory[0x74000+i*4];source[i]=i?0x12340000+i:0x16005;}
  for(auto& c:countBase)c=0xfffffff8;
  for(unsigned i=0;i<256;i++){gate[i]=Bits(float(sin((i+worker*256)*.713))*31.99f);up[i]=Bits(float(cos((i+worker*256)*.219))*10);}
  native.fill(0xa5a5a5a5);scale.fill(0xa5a5a5a5);
 }
 unsigned Index(uint32_t at)override {
  // Explicit NEW-image fixture ONLY. Immutable ROM bytes use a fetch slot;
  // this does NOT enlarge the native12KiB mapping/owner or old fixture.
  if(ownedPrefix==16384 && Range(at,0x64000,unsigned(activationCode.size()))){assert(!(at&3));l1[8191]=Word(activationCode,at-0x64000);return 8191;}
  if(Range(at,0x64000,12288)){assert(!(at&3));return 5120+(at-0x64000)/4;}
  return DirectSrcBImage::Index(at);
 }
 bool CodeAddress(uint32_t at)const override{return Range(at,0x64000,unsigned(activationCode.size())) || DirectSrcBImage::CodeAddress(at);}
 static unsigned CounterOffset(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
 uint32_t Count(unsigned i)const {
  unsigned extra=0;if(!activation){if(i==0 || i==2 || i==3 || i==6)extra=responses;
   if(i==1)for(unsigned p=0;p<responses;p++)extra+=packets.at(p).bytes/64;}
  return countBase[i]+extra;
 }
 void ClearTensorState()const {assert(drained && !rawPending && phaseQueue.empty() && pendingAccumulator.empty() && !recordRemaining);}
 uint32_t Load(uint32_t at)override {
  if(Range(at,0x64000,unsigned(activationCode.size())))return l1[Index(at)];
  if(Range(at,TT_Q38_FFN_RESIDENT_DESCRIPTOR,384))return memory.at(at);
  if(activation && Range(at,0x90000,3648))return memory.at(at);
  if(Range(at,0xffb20000,0x2000)) {
   nocReads++;unsigned offset=at-0xffb20000;
   for(unsigned i=0;i<8;i++)if(offset==CounterOffset(i))return Count(i);
   for(unsigned i=0;i<16;i++)if(offset==BlackholeDramSourceOffset(i))return source[i];
   if(activation) {
    if(offset<40){unsigned i=offset/4;commandReads++;return command[i]^(commandReads==badCommandRead?1U:0U);}
    for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800)return i?0:unsigned(pending);
    if(offset>=0x240 && offset<0x2c0)return 0;
   }
  }
  return DirectSrcBImage::Load(at);
 }
 virtual void SubmitActivation() {
  ClearTensorState();assert(activationPackets<4);unsigned p=activationPackets++;bool write=p>=2;
  const unsigned local[]={0x90000,0x90400,0x90c00,0x90e00},bytes[]={1024,1024,512,64};
  unsigned remote=descriptor[4]+descriptor[10+p];
  const uint32_t expected[]={write?local[p]:remote,0,write?descriptor[9]:descriptor[8],write?remote:local[p],0,
   write?descriptor[8]:descriptor[9],0,write?0x212U:0,bytes[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(activationPackets==failPacket){pending=true;return;}
  if(!write)assert(!activatedWrites && !nativeWrites && !scaleWrites);
  else assert(activatedWrites==256 && nativeWrites==128 && scaleWrites==31);
  for(unsigned i=0;i<bytes[p]/4;i++) {
   if(p==0)memory[local[p]+i*4]=gate[i];if(p==1)memory[local[p]+i*4]=up[i];
   if(p==2)native[i]=memory.at(local[p]+i*4);if(p==3)scale[i]=memory.at(local[p]+i*4);
  }
  unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=bytes[p]/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
  if(activationPackets==badCounterPacket)countBase[at+3]++;
  if(activationPackets==foreignCounterPacket)countBase[write?0:4]++;
  if(activationPackets==sourcePacket)source[11]^=1;
  if(activationPackets==descriptorPacket)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4]^=1;
  if(p==3){if(lateJob)memory[0x74000+4]^=1;if(lateBoot)memory[0x74040+4]^=1;if(lateDoorbell)memory[0x74100]++;}
 }
 void Store(uint32_t at,uint32_t value)override {
  if(Range(at,0xffb20000,40) && (activation || Range(pc,0x64000,unsigned(activationCode.size())))) {
   ClearTensorState();assert(!pending && !(at&3));unsigned i=(at-0xffb20000)/4;
   if(!activation){assert(i==normalizationWords && value==(i==5?selfRoute:0));if(++normalizationWords==10){normalizationWords=0;normalizations++;}}
   command[i]=value;return;
  }
  if(activation && at==0xffb20040){assert(value==1 && !pending);SubmitActivation();return;}
  if(activation && (Range(at,0x90800,1600) || Range(at,TT_Q38_FFN_RESIDENT_SUMMARY,128))) {
   ClearTensorState();assert(!pending && !(at&3));
   if(Range(at,0x90800,1024))activatedWrites++;if(Range(at,0x90c00,512))nativeWrites++;if(Range(at,0x90e00,64))scaleWrites++;
   memory[at]=value;return;
  }
  if(Range(at,0x74140,64) || Range(at,0x83f00,256)) {
   if(at!=0x74148 || value!=TT_Q38_FFN_PERSISTENT_FAULT){ClearTensorState();assert(!pending);}
   memory[at]=value;return;
  }
  DirectSrcBImage::Store(at,value);
 }
 static float Flush(float f){uint32_t b=Bits(f);return b&0x7f800000?f:Float(b&0x80000000);}
 void StepBlock()override {
  if(!Range(pc,0x64000,unsigned(activationCode.size()))){DirectSrcBImage::StepBlock();return;}
  uint32_t i=l1[Index(pc)],op=i&127;
  if(op!=7 && op!=0x27 && op!=0x53){DirectSrcBImage::StepBlock();return;}
  assert(++steps<StepLimit());unsigned d=(i>>7)&31,f=(i>>12)&7,s=(i>>15)&31,t=(i>>20)&31,hi=i>>25;
  if(op==7){assert(f==2);fp[d]=Load(regs[s]+Sign(i>>20,12));}
  else if(op==0x27){assert(f==2);Store(regs[s]+Sign((hi<<5)|d,12),fp[t]);}
  else {float a=Flush(Float(fp[s])),b=Flush(Float(fp[t]));switch(hi) {
   case 0:assert(f==7 || !f);fp[d]=Bits(Flush(a+b));break;
   case 4:assert(f==7 || !f);fp[d]=Bits(Flush(a-b));break;
   case 8:assert(f==7 || !f);fp[d]=Bits(Flush(a*b));break;
   case 0x10:assert(f<=2);fp[d]=f==0?((fp[s]&0x7fffffff)|(fp[t]&0x80000000)):f==1?((fp[s]&0x7fffffff)|((~fp[t])&0x80000000)):(fp[s]^(fp[t]&0x80000000));break;
   case 0x50:assert(f<=2);if(d)regs[d]=f==0?a<=b:f==1?a<b:a==b;break;
   case 0x68:assert(t<=1);fp[d]=Bits(t?float(regs[s]):float(int32_t(regs[s])));break;
   case 0x70:assert(!t && !f);if(d)regs[d]=fp[s];break;
   case 0x78:assert(!t && !f);fp[d]=regs[s];break;
   default:fprintf(stderr,"UNQUALIFIED FFN peer scalar instruction%08x PC%08x\n",i,pc);assert(false);
  }}regs[0]=0;pc+=4;
 }
 bool BootNew(){PoisonDst();Poison();while(pc!=TT_Q38_PEER_WAIT_LOAD && pc!=TT_Q38_PEER_FAULT_PARK)StepBlock();
  return pc==TT_Q38_PEER_WAIT_LOAD && memory.at(0x7414c)==TT_Q38_FFN_PERSISTENT_MARKER;}
 bool Finish(unsigned sequence,bool stopped=false) {
  for(;;){StepBlock();if(pc==TT_Q38_PEER_FAULT_PARK){assert(memory.at(0x74148)==TT_Q38_FFN_PERSISTENT_FAULT);return false;}
   if(pc==TT_Q38_PEER_STOP || (pc==TT_Q38_PEER_WAIT_LOAD && memory.at(0x74144)==sequence))break;}
  assert(!pending && minStack>=0x6e000 && memory.at(0x74140)==sequence && memory.at(0x74144)==sequence
   && memory.at(0x7414c)==TT_Q38_FFN_PERSISTENT_MARKER && !memory.at(0x74148));ClearTensorState();
  return stopped?pc==TT_Q38_PEER_STOP && memory.at(0x74150)==2:pc==TT_Q38_PEER_WAIT_LOAD && !memory.at(0x74150);
 }
 void StageActivation(unsigned sequence,unsigned phase=17,unsigned bank=0,bool edge=false) {
  assert(!pending);activation=true;activationPackets=commandReads=activatedWrites=nativeWrites=scaleWrites=0;
  for(unsigned i=0;i<8;i++)independentBefore[i]=Count(i);
  auto& d=descriptor;d={};d[0]=TT_Q38_FFN_RESIDENT_MARKER;d[1]=1;d[2]=67108864;d[3]=bank;d[4]=edge?0xc0000000:0x04000000;
  d[5]=logicalWorker-1;d[6]=phase;d[7]=logicalWorker;d[8]=PioneerQ38SpanRoute(bank);d[9]=selfRoute;
  d[10]=30848+d[5]*1024;d[11]=100480+d[5]*1024;d[12]=239744+d[5]*512;d[13]=295360+d[5]*64;d[14]=256;
  for(unsigned i=0;i<16;i++)d[16+i]=source[i];for(unsigned i=32;i<40;i++)d[i]=0x13570000+i;
  d[40]=123;d[41]=7;d[42]=logicalWorker%64;
  for(unsigned i=0;i<64;i++)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=d[i];
  const uint32_t j[]={TT_Q38_FFN_PERSISTENT_JOB,256,576,phase,0,0,0,0,0,0,TT_Q38_FFN_PERSISTENT_MARKER,logicalWorker,sequence,3,0,0};
  for(unsigned i=0;i<16;i++)memory[0x74000+i*4]=j[i];memory[0x74100]=sequence;
 }
 bool Matrix(unsigned sequence) {
  activation=false;Poison();PoisonTail();for(unsigned i=0;i<16;i++)memory[0x74000+i*4]=matrixJob[i];
  memory[0x74028]=TT_Q38_FFN_PERSISTENT_MARKER;memory[0x7402c]=logicalWorker;memory[0x74030]=sequence;
  memory[0x74034]=1;memory[0x74038]=1;memory[0x74100]=sequence;
  if(!Finish(sequence))return false;
  assert(requests==packets.size() && !dotProbeReads && CorrectRows() && memory.at(0x83f4c)==TT_Q38_FFN_PERSISTENT_MARKER);
  uint32_t next[8];for(unsigned i=0;i<8;i++)next[i]=Count(i);for(unsigned i=0;i<8;i++)countBase[i]=next[i];
  NextPackets();return true;
 }
 bool Stop(unsigned sequence){for(unsigned i=0;i<16;i++)memory[0x74000+i*4]=0;
  memory[0x74028]=TT_Q38_FFN_PERSISTENT_MARKER;memory[0x7402c]=logicalWorker;memory[0x74030]=sequence;
  memory[0x74034]=2;memory[0x74100]=sequence;return Finish(sequence,true);}
 void VerifyActivation() {
  assert(activationPackets==4 && activatedWrites==256 && nativeWrites==128 && scaleWrites==31);
  float actual[256];uint32_t q[73];
  for(unsigned i=0;i<256;i++){actual[i]=Float(memory.at(0x90800+i*4));float reference=(Float(gate[i])/(1+expf(-Float(gate[i]))))*Float(up[i]);
   assert(isfinite(actual[i]) && fabsf(actual[i]-reference)<=3e-6f*fabsf(reference));}
  Q38ActivationReference(actual,q);for(unsigned i=0;i<128;i++) {
   uint32_t word=0;for(unsigned lane=0;lane<2;lane++){unsigned at=i*2+lane,byte=(q[1+at/4]>>((at%4)*8))&255;
    int value=int(byte)-int((byte&128)*2);word|=(0x4000U|(value<0?0x8000U|unsigned(-value):unsigned(value)))<<(lane*16);}
   assert(native[i]==word);
  }
  assert(scale[0]==q[0] && PioneerQ38FfnResidentScale(scale.data(),descriptor.data(),logicalWorker-1));
  uint32_t proof[64],job[16],after[8];for(unsigned i=0;i<64;i++)proof[i]=memory.at(0x83f00+i*4);
  for(unsigned i=0;i<16;i++)job[i]=memory.at(0x74000+i*4);for(unsigned i=0;i<8;i++)after[i]=Count(i);
  assert(PioneerQ38FfnPersistentActivationProof(proof,job,descriptor.data(),source,logicalWorker,selfRoute,independentBefore,after));
  for(unsigned i=0;i<64;i++){proof[i]^=1;assert(!PioneerQ38FfnPersistentActivationProof(proof,job,descriptor.data(),source,logicalWorker,selfRoute,independentBefore,after));proof[i]^=1;}
  for(unsigned i=0;i<8;i++)assert(memory.at(0x83f00+i*4)==memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+(8+i)*4)
   && memory.at(0x83f20+i*4)==memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+(16+i)*4));
  assert(memory.at(0x83fc0)==3 && memory.at(0x83fc4)==descriptor[6] && memory.at(0x83ffc)==1);
 }
 void Inert(){unsigned reads=nocReads,packets=activationPackets,old=requests,math=matrixInstructions;
  for(unsigned i=0;i<64;i++)StepBlock();assert(reads==nocReads && packets==activationPackets && old==requests && math==matrixInstructions);}
};
#ifndef PIONEER_Q38_FFN_PERSISTENT_MACHINE_ONLY
int main(int argc,char** argv){assert(argc==2 || argc==3 || (argc==4 && !strcmp(argv[3],"--wide")));setvbuf(stdout,nullptr,_IOLBF,0);unsigned positive=0,faults=0,minStack=0x70000;
 if(argc==4){
  for(unsigned type:{12U,14U})for(unsigned k:{5120U,17408U})for(unsigned groups:{3U,16U}) {
   printf("START WIDE ACTUAL QFP2 type%u K%u groups%u;NOT_SILICON_TIME\n",type,k,groups);
   FfnPersistentPeer m(argv[1],type,k,groups,68);assert(m.BootNew() && m.Matrix(1));
   m.StageActivation(2);assert(m.Finish(2));m.VerifyActivation();FreshInputs(m);
   assert(m.Matrix(3) && m.Stop(4));minStack=std::min(minStack,m.minStack);positive++;
   printf("PASS WIDE ACTUAL QFP2 type%u K%u rows%u changed-input matrix/activation/matrix/STOP;EXACT FP32+independent FP64/native bytes/NO CPUref in program\n",type,k,groups*16);
  }
  printf("PASS OFFLINE WIDE actual persistent FFN cases%u/maxObservedDeviceStackBytes%u;NO_NATIVE_MODEL_OR_SPEED_QUALIFICATION\n",positive,0x70000-minStack);return 0;
 }
 for(unsigned type:{12U,14U})for(unsigned worker:{1U,35U,68U}) {
  FfnPersistentPeer m(argv[1],type,5120,1,worker);assert(m.BootNew() && m.Matrix(1));
  m.StageActivation(2);assert(m.Finish(2));m.VerifyActivation();
  FreshInputs(m);assert(m.Matrix(3));for(unsigned i=0;i<256;i++)m.up[i]=Bits(Float(m.up[i])*2);
  m.StageActivation(4,19);assert(m.Finish(4));m.VerifyActivation();m.Inert();assert(m.Stop(5));
  assert(m.normalizations==2 && !m.dotProbeReads);positive++;
  minStack=std::min(minStack,m.minStack);
  printf("ACTUAL QFP2 type%u worker%u matrix/activation/matrix/activation/STOP;EXACT matrices+native bytes/NO resets PASS\n",type,worker);
 }
 for(unsigned worker=1;worker<=68;worker++) {
  FfnPersistentPeer m(argv[1],12,5120,1,worker);assert(m.BootNew());m.StageActivation(1,17,worker%7,(worker&1)!=0);
  if(worker==1){m.gate.fill(0);m.up.fill(0);}
  assert(m.Finish(1));m.VerifyActivation();m.Inert();assert(m.Stop(2));positive++;minStack=std::min(minStack,m.minStack);
 }
 puts("PASS ALL68 activation worker bindings,all7 banks/both capacity edges,zero and changed data,ALL64 proof corruptions");
 for(unsigned p=1;p<=4;p++)for(unsigned fault=0;fault<5;fault++) {
  FfnPersistentPeer m(argv[1]);assert(m.BootNew());m.StageActivation(1);
  if(!fault)m.failPacket=p;if(fault==1)m.badCounterPacket=p;if(fault==2)m.foreignCounterPacket=p;
  if(fault==3)m.sourcePacket=p;if(fault==4)m.descriptorPacket=p;
  assert(!m.Finish(1) && m.activationPackets==p);m.Inert();faults++;
 }
 for(unsigned word=1;word<=40;word++){FfnPersistentPeer m(argv[1]);assert(m.BootNew());m.StageActivation(1);m.badCommandRead=word;
  assert(!m.Finish(1) && m.activationPackets==(word-1)/10);m.Inert();faults++;}
 for(unsigned fault=0;fault<3;fault++){FfnPersistentPeer m(argv[1]);assert(m.BootNew());m.StageActivation(1);
  if(!fault)m.lateJob=true;if(fault==1)m.lateBoot=true;if(fault==2)m.lateDoorbell=true;
  assert(!m.Finish(1) && m.activationPackets==4);m.Inert();faults++;}
 for(unsigned word=0;word<16;word++){FfnPersistentPeer m(argv[1]);assert(m.BootNew());m.StageActivation(1);m.memory[0x74000+word*4]^=1;
  assert(!m.Finish(1) && !m.activationPackets);m.Inert();faults++;}
 for(unsigned word=0;word<64;word++){if(word>=32 && word<=42)continue;
  FfnPersistentPeer m(argv[1]);assert(m.BootNew());m.StageActivation(1);m.memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+word*4]^=1;
  assert(!m.Finish(1) && !m.activationPackets);m.Inert();faults++;}
 for(unsigned source=0;source<16;source++){FfnPersistentPeer m(argv[1]);assert(m.BootNew());m.StageActivation(1);m.source[source]^=1;
  assert(!m.Finish(1) && !m.activationPackets);m.Inert();faults++;}
 for(unsigned sequence:{2U,0x7fffffffU}){FfnPersistentPeer m(argv[1]);assert(m.BootNew());m.StageActivation(sequence);
  assert(!m.Finish(sequence) && !m.activationPackets);m.Inert();faults++;}
 for(unsigned worker:{69U,119U}){FfnPersistentPeer m(argv[1],12,5120,1,worker);assert(m.BootNew());m.StageActivation(1);
  assert(!m.Finish(1) && !m.activationPackets);m.Inert();faults++;}
 for(unsigned which=0;which<2;which++){FfnPersistentPeer m(argv[1]);
  if(!which){m.memory[0x74040]=0x32503851;assert(!m.BootNew());}
  else {assert(m.BootNew());m.StageActivation(1);m.memory[0x74028]=0x32503851;assert(!m.Finish(1));}
  assert(!m.activationPackets);m.Inert();faults++;}
 for(uint32_t bad:{1U,0x2b7fffffU,0x42000001U,0x7f800000U,0x7fc00001U}) {
  FfnPersistentPeer m(argv[1]);assert(m.BootNew());m.StageActivation(1);m.gate[255]=bad;
  assert(!m.Finish(1) && m.activationPackets==2 && !m.activatedWrites && !m.nativeWrites);m.Inert();faults++;
 }
 {FfnPersistentPeer m(argv[1]);assert(m.BootNew());m.StageActivation(1);assert(m.Finish(1));m.VerifyActivation();
  m.badReadback=true;assert(!m.Matrix(2) && !m.requests && !m.matrixInstructions && m.normalizations==1);m.Inert();faults++;}
 if(argc==3){DirectSrcBImage old(argv[2],12,5120,1,1);assert(old.Boot() && !old.Job(1,3) && !old.requests && !old.matrixInstructions);faults++;}
 printf("PASS OFFLINE ACTUAL persistent FFN peer:positive%u/faults%u/maxObservedDeviceStackBytes%u;QFP2 distinct ABI/Work noCPUref/native GDDR sink/sequence retained/UNKNOWN inert;NO_NATIVE_COORDINATOR_MODEL_OR_SPEED_QUALIFICATION\n",positive,faults,0x70000-minStack);
}
#endif
