/* SPDX-License-Identifier: MIT */
// ACTUAL emitted persistent service instructions. ONLY endpoints simulated.
// SYNTHETIC primitive inputs/weights,NOT original-model/native/speed proof.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_FFN_PERSISTENT_MACHINE_ONLY
#include "pioneer_q38_ffn_persistent_image_test.cpp"
#include <memory>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvBatchPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentConvServiceShape.h"
struct ResidentOperatorPeer:FfnPersistentPeer {
 std::vector<uint8_t> service;
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical;
 std::array<uint32_t,384> table={};
 struct Group {std::array<uint32_t,16> input={},output={};std::array<uint32_t,64> weight={};std::array<uint32_t,48> previous={},next={};};
 Group groups[6];
 bool convActive=false,jobLate=false,bootLate=false,doorbellLate=false;
 unsigned convPackets=0,convWrites=0,convFailPacket=0,convSourcePacket=0,convCounterPacket=0;
 unsigned convTablePacket=0,convTableWord=0;
 ResidentOperatorPeer(const char* path,unsigned worker=1,unsigned type=12)
  :FfnPersistentPeer(path,type,5120,1,worker,16384),service(Read((std::string(path)+"/service.bin").c_str())),
   canonical(new PioneerQ38CanonicalModelPlan){
  assert(!service.empty() && !(service.size()&3) && service.size()<=0x6000 && canonical->Build());
 }
 unsigned Index(uint32_t at)override{
  if(Range(at,0x68000,unsigned(service.size()))){assert(!(at&3));l1[8190]=Word(service,at-0x68000);return 8190;}
  return FfnPersistentPeer::Index(at);
 }
 bool CodeAddress(uint32_t at)const override{return Range(at,0x68000,unsigned(service.size())) || FfnPersistentPeer::CodeAddress(at);}
 uint32_t Load(uint32_t at)override{
  if(Range(at,0x68000,unsigned(service.size())))return Word(service,at-0x68000);
  if(convActive && Range(at,0xffb20000,0x2000)){
   nocReads++;unsigned offset=at-0xffb20000;
   for(unsigned i=0;i<8;i++)if(offset==CounterOffset(i))return Count(i);
   for(unsigned i=0;i<16;i++)if(offset==BlackholeDramSourceOffset(i))return source[i];
   if(offset<40){commandReads++;return command[offset/4];}
   for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800)return i?0:unsigned(pending);
   if(offset>=0x240 && offset<0x2c0)return 0;
  }
  if(convActive && (Range(at,0x80000,0x1100) || Range(at,TT_Q38_CONV_BATCH_TABLE,1536)
    || Range(at,TT_Q38_FFN_RESIDENT_SUMMARY,128)))return memory.at(at);
  return FfnPersistentPeer::Load(at);
 }
 void SubmitConv(){
  ClearTensorState();unsigned n=PioneerQ38ConvBatchCount(logicalWorker);assert(convPackets<n*5);
  unsigned j=convPackets/5,p=convPackets%5;convPackets++;bool write=p>=3;
  const auto* d=table.data()+j*64;
  const unsigned local[]={0x80000,0x80400,0x80800,0x80c00,0x81000},bytes[]={64,256,192,192,64};
  const unsigned remote[]={d[4]+d[45],d[12],d[15],d[4]+d[42],d[4]+d[43]},route[]={d[8],d[13],d[41],d[8],d[8]};
  const uint32_t expected[]={write?local[p]:remote[p],0,write?d[9]:route[p],write?remote[p]:local[p],0,write?route[p]:d[9],0,write?0x212U:0,bytes[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(convPackets==convFailPacket){pending=true;return;}
  auto& g=groups[j];
  for(unsigned i=0;i<bytes[p]/4;i++){
   if(!write)assert(memory.at(local[p]+i*4)==0x7fc50000+p*1024+i);
   if(p==0)memory[local[p]+i*4]=g.input[i];if(p==1)memory[local[p]+i*4]=g.weight[i];
   if(p==2)memory[local[p]+i*4]=g.previous[i];
   if(p==3){g.next[i]=memory.at(local[p]+i*4);convWrites++;}
   if(p==4){g.output[i]=memory.at(local[p]+i*4);convWrites++;}
  }
  unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=bytes[p]/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
  if(convPackets==convCounterPacket)countBase[write?0:4]++;
  if(convPackets==convSourcePacket)source[11]^=1;
  if(convPackets==convTablePacket)memory[TT_Q38_CONV_BATCH_TABLE+convTableWord*4]^=1;
  if(convPackets==n*5){if(jobLate)memory[0x74000+4]^=1;if(bootLate)memory[0x74040+4]^=1;if(doorbellLate)memory[0x74100]++;}
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!Range(at,0x68000,unsigned(service.size()))); // proposed code ROM,not an expanded native grant
  if(convActive){
   if(Range(at,0xffb20000,40)){ClearTensorState();assert(!pending && !(at&3));command[(at-0xffb20000)/4]=value;return;}
   if(at==0xffb20040){assert(value==1 && !pending);SubmitConv();return;}
   if(Range(at,0x80000,0x1100) || Range(at,TT_Q38_FFN_RESIDENT_SUMMARY,128)){assert(!pending);memory[at]=value;return;}
  }
  FfnPersistentPeer::Store(at,value);
 }
 void StepBlock()override{
  if(!Range(pc,0x68000,unsigned(service.size()))){FfnPersistentPeer::StepBlock();return;}
  uint32_t i=Word(service,pc-0x68000),op=i&127;
  if(op!=7 && op!=0x27 && op!=0x53){FfnPersistentPeer::StepBlock();return;}
  assert(++steps<StepLimit());unsigned d=(i>>7)&31,f=(i>>12)&7,s=(i>>15)&31,t=(i>>20)&31,hi=i>>25;
  if(op==7){assert(f==2);fp[d]=Load(regs[s]+Sign(i>>20,12));}
  else if(op==0x27){assert(f==2);Store(regs[s]+Sign((hi<<5)|d,12),fp[t]);}
  else {float a=Flush(Float(fp[s])),b=Flush(Float(fp[t]));switch(hi){
   case 0:assert(f==7 || !f);fp[d]=Bits(Flush(a+b));break;
   case 4:assert(f==7 || !f);fp[d]=Bits(Flush(a-b));break;
   case 8:assert(f==7 || !f);fp[d]=Bits(Flush(a*b));break;
   case 0x10:assert(f<=2);fp[d]=f==0?((fp[s]&0x7fffffff)|(fp[t]&0x80000000)):f==1?((fp[s]&0x7fffffff)|((~fp[t])&0x80000000)):(fp[s]^(fp[t]&0x80000000));break;
   case 0x50:assert(f<=2);if(d)regs[d]=f==0?a<=b:f==1?a<b:a==b;break;
   case 0x68:assert(t<=1);fp[d]=Bits(t?float(regs[s]):float(int32_t(regs[s])));break;
   case 0x70:assert(!t && !f);if(d)regs[d]=fp[s];break;
   case 0x78:assert(!t && !f);fp[d]=regs[s];break;
   default:fprintf(stderr,"UNQUALIFIED resident service scalar instruction%08x PC%08x\n",i,pc);assert(false);
  }}regs[0]=0;pc+=4;
 }
 void StageConv(unsigned sequence,unsigned phase=17,unsigned layer=0,unsigned bank=0,unsigned position=0){
  assert(!pending);convActive=activation=true;convPackets=convWrites=commandReads=0;
  auto batch=std::make_unique<PioneerQ38ConvBatchPlan>();PioneerQ38GdnIdentity id={1,2,3,4};
  assert(batch->Build(*canonical,layer,position,bank,logicalWorker,id,source,phase));
  for(unsigned i=0;i<384;i++)memory[TT_Q38_CONV_BATCH_TABLE+i*4]=table[i]=batch->Records()[i];
  for(unsigned j=0;j<batch->Count();j++){
   auto& g=groups[j];for(unsigned c=0;c<16;c++){
    g.input[c]=Bits(float(int((c+j*16+logicalWorker)%17)-8)*.125f);
    for(unsigned t=0;t<4;t++)g.weight[c*4+t]=Bits(float(int((c+t+j)%9)-4)*.0625f);
    for(unsigned t=0;t<3;t++)g.previous[c*3+t]=Bits(float(int((c+t+j+layer)%13)-6)*.25f);
   }g.output.fill(0x7fc00000);g.next.fill(0x7fc00000);
  }
  const uint32_t job[]={TT_Q38_RESIDENT_CONV_SERVICE_JOB,batch->Count(),1536,phase,0,0,0,0,0,0,
   TT_Q38_FFN_PERSISTENT_MARKER,logicalWorker,sequence,TT_Q38_RESIDENT_CONV_SERVICE_OPERATION,0,0};
  for(unsigned i=0;i<16;i++)memory[0x74000+i*4]=job[i];memory[0x74100]=sequence;
  for(unsigned i=0;i<8;i++)independentBefore[i]=Count(i);
 }
 void VerifyConv(){
  unsigned n=PioneerQ38ConvBatchCount(logicalWorker);assert(convPackets==n*5 && convWrites==n*64);
  for(unsigned j=0;j<n;j++)for(unsigned c=0;c<16;c++){
   const auto& g=groups[j];float sum=0;
   for(unsigned t=0;t<4;t++){float product=Float(t==3?g.input[c]:g.previous[c*3+t])*Float(g.weight[c*4+t]);sum+=product;}
   double x=sum,reference=x>=0?x/(1+exp(-x)):(x*exp(x))/(1+exp(x));
   double actual=Float(g.output[c]);assert(isfinite(actual) && fabs(actual-reference)<=3e-6*(1+fabs(reference)));
   assert(g.next[c*3]==g.previous[c*3+1] && g.next[c*3+1]==g.previous[c*3+2] && g.next[c*3+2]==g.input[c]);
  }
  uint32_t p[64],job[16],after[8],summary[32];for(unsigned i=0;i<64;i++)p[i]=memory.at(0x83f00+i*4);
  for(unsigned i=0;i<16;i++)job[i]=memory.at(0x74000+i*4);for(unsigned i=0;i<8;i++)after[i]=Count(i);
  for(unsigned i=0;i<32;i++)summary[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  assert(PioneerQ38ResidentConvServiceProof(p,job,table.data(),source,logicalWorker,selfRoute,independentBefore,after,summary));
  for(unsigned i=0;i<64;i++){p[i]^=1;assert(!PioneerQ38ResidentConvServiceProof(p,job,table.data(),source,logicalWorker,selfRoute,independentBefore,after,summary));p[i]^=1;}
 }
 void ServiceInert(){unsigned reads=nocReads,packets=convPackets,matrices=matrixInstructions,old=requests,writes=convWrites;
  for(unsigned i=0;i<64;i++)StepBlock();assert(reads==nocReads && packets==convPackets && matrices==matrixInstructions && old==requests && writes==convWrites);}
 bool MatrixAfterConv(unsigned sequence){convActive=false;return Matrix(sequence);}
};
#ifndef PIONEER_Q38_RESIDENT_OPERATOR_SERVICE_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--smoke")));setvbuf(stdout,nullptr,_IOLBF,0);
 bool smoke=argc==4;unsigned positives=0,faults=0,groups=0,minStack=0x70000;
 for(unsigned worker:{1U,63U,119U})for(unsigned type:{12U,14U}){
  if(smoke && (worker!=1 || type!=12))continue;
  ResidentOperatorPeer m(argv[1],worker,type);assert(m.BootNew() && m.MatrixAfterConv(1));
  m.StageConv(2);assert(m.Finish(2));m.VerifyConv();m.ServiceInert();
  FreshInputs(m);assert(m.MatrixAfterConv(3));m.StageConv(4,19,1,6,8191);assert(m.Finish(4));m.VerifyConv();
  m.ServiceInert();assert(m.Stop(5));minStack=std::min(minStack,m.minStack);positives++;
  printf("PASS emitted persistent matrix/Conv/matrix/changed-layer-Conv/STOP worker%u type%u retainedSeq1..5 NO reset/host code reload\n",worker,type);
 }
 if(smoke){printf("PASS SMOKE actual persistent service/maxObservedDeviceStackBytes%u;SYNTHETIC weights;NO native/full-model/card speed grant\n",0x70000-minStack);return 0;}
 for(unsigned worker=1;worker<120;worker++){
  ResidentOperatorPeer m(argv[1],worker);assert(m.BootNew());m.StageConv(1,17,worker%3,worker%7,(worker&1)?8191:0);
  assert(m.Finish(1));m.VerifyConv();m.ServiceInert();assert(m.Stop(2));groups+=PioneerQ38ConvBatchCount(worker);positives++;
  minStack=std::min(minStack,m.minStack);
 }
 assert(groups==634);puts("PASS ALL119 peer bindings/634groups (ROOT0 six groups NOT provided)/layers0..2/positions0+8191/all7banks/ALL64 proof mutations");
 for(unsigned word=0;word<16;word++){
  ResidentOperatorPeer m(argv[1]);assert(m.BootNew());m.StageConv(1);m.memory[0x74000+word*4]^=1;
  assert(!m.Finish(1) && !m.convPackets);m.ServiceInert();faults++;
 }
 for(unsigned word=0;word<384;word++){
  ResidentOperatorPeer m(argv[1]);assert(m.BootNew());m.StageConv(1);m.convTablePacket=1;m.convTableWord=word;
  assert(!m.Finish(1) && m.convPackets==1 && !m.convWrites);m.ServiceInert();faults++;
 }
 for(unsigned fault=0;fault<3;fault++)for(unsigned packet:{1U,15U,30U}){
  ResidentOperatorPeer m(argv[1]);assert(m.BootNew());m.StageConv(1);
  if(fault==0)m.convSourcePacket=packet;if(fault==1)m.convCounterPacket=packet;if(fault==2)m.convFailPacket=packet;
  assert(!m.Finish(1) && m.convPackets==packet);m.ServiceInert();faults++;
 }
 for(unsigned fault=0;fault<3;fault++){
  ResidentOperatorPeer m(argv[1]);assert(m.BootNew());m.StageConv(1);
  if(fault==0)m.jobLate=true;if(fault==1)m.bootLate=true;if(fault==2)m.doorbellLate=true;
  assert(!m.Finish(1) && m.convPackets==30);m.ServiceInert();faults++;
 }
 {FfnPersistentPeer old(argv[2],12,5120,1,1,16384);assert(old.BootNew());
  ResidentOperatorPeer data(argv[1]);data.StageConv(1);for(unsigned i=0;i<16;i++)old.memory[0x74000+i*4]=data.memory.at(0x74000+i*4);
  old.memory[0x74100]=1;assert(!old.Finish(1) && !old.requests && !old.matrixInstructions);old.Inert();faults++;}
 printf("PASS OFFLINE actual persistent service positive%u/fault%u/maxObservedDeviceStackBytes%u;no reset/CPUref in program/old peer refusesop7/UNKNOWN inert;SYNTHETIC endpoints and weights ONLY,NO native ROOT/model/card timing qualification\n",positives,faults,0x70000-minStack);
}
#endif
