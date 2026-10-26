/* SPDX-License-Identifier: MIT */
// Actual NEW worker instructions. Global max/inverse and prepared queries
// come from ACTUAL existing QDA/QDP instructions, NEVER CPU oracle feedback.
// The CONTROL sequencer/fabric is synthetic, NOT an implemented native
//96-worker controller or performance result. Reference math observes ONLY.
#define PIONEER_Q38_DENSE_ATTENTION_MACHINE_ONLY
#include "pioneer_q38_dense_attention_image_test.cpp"
#include "../PioneerQ38DensePartitionLayout.h"
#include <memory>
namespace W=PioneerQ38DensePartitionLayout;
struct ProducedGroup {
 std::array<uint32_t,1536> query={};std::array<uint32_t,128> currentK={},currentV={};
 std::array<uint32_t,6> maximum={},inverse={};
 std::vector<uint16_t> keys,values;
 std::array<std::vector<uint32_t>,6> scores,exp,probability;
 std::array<std::array<float,256>,6> output={};
 unsigned tokens,group;
 // Explicit observer construction for joined tests. No arithmetic or
 // emulated processor state is manufactured by this data-only constructor.
 ProducedGroup(unsigned g,unsigned n):tokens(n),group(g){}
 ProducedGroup(const char* image,unsigned g,unsigned n,unsigned trial=0):tokens(n),group(g){
  for(unsigned h=0;h<6;h++){
   DenseAttentionMachine original(image,g*6+h,n,trial);
   if(h){original.priorK=keys;original.priorV=values;}
   else{keys=original.priorK;values=original.priorV;}
   assert(original.Run());original.Verify();
   for(unsigned i=0;i<256;i++){
    query[h*256+i]=original.memory.at(A::HalfQuery+i*4);
    output[h][i]=PioneerAttentionFloat(original.memory.at(A::PreGate+i*4));
   }
   for(unsigned i=0;i<128;i++){
    if(!h){currentK[i]=original.memory.at(A::NewK+i*4);currentV[i]=original.memory.at(A::NewV+i*4);}
    else assert(currentK[i]==original.memory.at(A::NewK+i*4) && currentV[i]==original.memory.at(A::NewV+i*4));
   }
   maximum[h]=original.memory.at(A::Summary+53*4);inverse[h]=original.memory.at(A::Summary+55*4);
   scores[h]=original.actualScores;exp[h]=original.actualExp;probability[h].resize(n);
   for(unsigned t=0;t<n;t++)probability[h][t]=original.memory.at(A::Scores+t*4);
  }
 }
};
struct PartitionMachine:ScalarMachine {
 DenseSfpuModel sfpu;
 const ProducedGroup& produced;
 std::array<uint32_t,64> header={};std::array<uint32_t,16> source={};
 std::array<uint32_t,10> command={};std::array<uint32_t,8> counts={},before={};
 unsigned packets=0,phasePackets=0,phase=0,failPacket=0,counterFault=0,sourceFault=0,headerFault=0;
 unsigned parameterFaultPacket=0,jobFaultPacket=0,privateWrites=0,partialWrites=0,scoreWrites=0;
 unsigned badReadback=0,readbacks=0;
 bool pending=false;
 PartitionMachine(const char* image,const ProducedGroup& input,unsigned ordinal)
  :ScalarMachine(image,0,0),produced(input){
  memory.clear();unsigned worker=1+input.group*24+ordinal;
  const unsigned first[]={W::Magic,1,64,3,input.tokens-1,input.group,ordinal,input.tokens,
   0x10000000,0x20000000,0,1,W::Route(0),W::Route(1),W::Peer(worker),worker};
  for(unsigned i=0;i<16;i++)header[i]=first[i];
  for(unsigned i=0;i<16;i++)header[16+i]=source[i]=i?0x12340000+worker*64+i:0x16005+(worker<<20);
  for(unsigned i=32;i<40;i++)header[i]=i-31;
  header[40]=17;header[41]=256;header[42]=0x3d800000;header[43]=32;header[44]=8192;header[45]=16384;
  header[48]=W::First(input.tokens,ordinal);header[49]=W::Count(input.tokens,ordinal);
  unsigned prior=header[48]>=header[4]?0:std::min(header[49],header[4]-header[48]);
  header[46]=2*((prior+31)>>5);header[47]=prior*16;
  header[50]=3;header[51]=6;header[52]=24;header[53]=384;header[54]=1536;header[55]=W::Magic;
  assert(W::Shape(header.data()));SetHeader();counts.fill(0xfffffff8);before=counts;
  for(unsigned i=0;i<1536;i++)memory[W::HalfQuery+i*4]=input.query[i];
  for(unsigned i=0;i<128;i++){memory[W::NewK+i*4]=input.currentK[i];memory[W::NewV+i*4]=input.currentV[i];}
  for(unsigned i=0;i<16;i++){memory[W::Job+i*4]=0;memory[W::Status+i*4]=0xdeadbeef;memory[W::Parameters+i*4]=0xa5a5a5a5;}
  memory[W::Doorbell]=0;
  for(unsigned a=0x6e000;a<0x70000;a+=4)memory[a]=0x7fc00001;
  for(auto range:std::array<std::pair<unsigned,unsigned>,7>{{{W::PrivateQuery,7168},{W::Summary,256},{W::Local,256},
    {W::Scores,6*384*4},{W::Accumulate,12288},{W::Partial,12288},{W::Chunk,16384}}})
   for(unsigned a=range.first;a<range.first+range.second;a+=4)memory[a]=0x7fc00001;
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
  for(unsigned a:Canaries())memory[a]=0xdecafbad;
 }
 static std::array<unsigned,12> Canaries(){return {W::Header-4,W::Header+256,W::Summary+256,W::Job-4,W::Status+64,
  W::HalfQuery-4,W::PrivateQuery-4,W::PrivateV+512,W::Scores+6*384*4,W::Local+256,W::Partial+12288,W::Chunk+16384};}
 void SetHeader(){for(unsigned i=0;i<64;i++)memory[W::Header+i*4]=header[i];}
 unsigned Index(uint32_t a)override{
  if(a==0x6000)return 0;
  assert(Range(a,W::Code,unsigned(code.size())) && !(a&3));return 1+(a-W::Code)/4;
 }
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,W::Code,unsigned(code.size()));}
 unsigned StepLimit()const override{return 2000000000U;}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));
  if(a==0xffe000fc || Range(a,0xffef0000,224*4) || Range(a,0xffbd8000,12*64))return sfpu.Load(a);
  if(Range(a,0xffb20000,0x2000)){
   unsigned o=a-0xffb20000;if(o<40)return command[o/4]^(++readbacks==badReadback?1U:0U);
   for(unsigned i=0;i<4;i++)if(o==0x40+i*0x800)return i?0:unsigned(pending);
   if(o>=0x240 && o<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(o==DenseAttentionMachine::SourceOffset(i))return source[i];
   const unsigned at[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(o==at[i])return counts[i];assert(!"unqualified partition NoC read");
  }
  if(Range(a,W::Code,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,W::Header,256) || Range(a,W::Summary,256) || Range(a,W::Job,64) || a==W::Doorbell
   || Range(a,W::Status,64) || Range(a,W::Parameters,64) || Range(a,W::HalfQuery,7168) || Range(a,W::PrivateQuery,7168)
   || Range(a,W::Scores,6*384*4) || Range(a,W::Local,256) || Range(a,W::Accumulate,12288)
   || Range(a,W::Partial,12288) || Range(a,W::Chunk,16384) || Range(a,0x17ffc0,12));
  if(Range(a,W::Scores,6*384*4)){unsigned at=(a-W::Scores)/4;assert(at%384<header[49]);}
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));
  if(a==0xffe40000 || a==0xffe000fc || Range(a,0xffef0000,224*4) || Range(a,0xffbd8000,8*64)){sfpu.Store(a,value);return;}
  if(Range(a,0xffb20000,40)){assert(!pending);command[(a-0xffb20000)/4]=value;return;}
  if(a==0xffb20040){assert(value==1 && !pending);Submit();return;}
  assert(Range(a,0x6e000,8192) || Range(a,W::Summary,256) || Range(a,W::Status,64) || Range(a,W::PrivateQuery,7168)
   || Range(a,W::Scores,6*384*4) || Range(a,W::Local,256) || Range(a,W::Accumulate,12288)
   || Range(a,W::Partial,12288) || Range(a,W::Chunk,16384) || Range(a,0x17ffc0,12));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,W::PrivateQuery,7168))privateWrites++;
  if(Range(a,W::Scores,6*384*4)){unsigned at=(a-W::Scores)/4;assert(at%384<header[49]);scoreWrites++;}
  if(Range(a,W::Partial,12288))partialWrites++;
  memory[a]=value;
 }
 void Submit(){
  assert(phase==1 || phase==3);unsigned per=header[46]>>1;assert(phasePackets<per);
  unsigned global=header[48]+phasePackets*32,count=std::min(32U,std::min(header[49]-phasePackets*32,header[4]-global)),bytes=count*512;
  unsigned which=phase==3;const unsigned expected[]={header[8+which]+global*512,0,header[12+which],W::Chunk,0,header[14],0,0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);packets++;phasePackets++;
  if(packets==failPacket){pending=true;return;}
  const auto& data=which?produced.values:produced.keys;
  for(unsigned i=0;i<bytes/4;i++)memory[W::Chunk+i*4]=data[global*256+i*2]|(uint32_t(data[global*256+i*2+1])<<16);
  counts[0]++;counts[1]+=bytes/64;counts[2]++;counts[3]++;counts[6]++;
  if(packets==counterFault)counts[3]++;
  if(packets==sourceFault)source[11]^=1;
  if(packets==headerFault)memory[W::Header+40*4]^=1;
  if(packets==parameterFaultPacket)memory[W::Parameters]^=1;
  if(packets==jobFaultPacket)memory[W::Job+8]^=1;
 }
 bool Wait(unsigned finished){
  for(;;){
   StepScalar();
   if(pc==0x40058){assert(memory.at(0x17ffc8)==W::Fault);return false;}
   if(pc==0x40064 && memory.at(W::Status+4)==finished)break;
  }
  assert(memory.at(W::Status+12)==W::Magic && !memory.at(W::Status+8) && !memory.at(W::Status+16));return true;
 }
 bool Boot(){assert(Wait(0));assert(!packets && !sfpu.commands && !privateWrites);return true;}
 void Stage(unsigned operation){
  phase=operation;phasePackets=0;before=counts;
  const unsigned job[]={W::Magic,operation,operation,header[40],header[15],header[3],header[4],header[5],header[48],header[49],0,0,0,0,0,0};
  for(unsigned i=0;i<16;i++)memory[W::Job+i*4]=job[i];
  if(operation==2 || operation==3){
   for(unsigned i=0;i<16;i++)memory[W::Parameters+i*4]=0;
   for(unsigned h=0;h<6;h++){memory[W::Parameters+h*4]=produced.maximum[h];if(operation==3)memory[W::Parameters+(8+h)*4]=produced.inverse[h];}
  }
  memory[W::Doorbell]=operation;
 }
 bool RunPhase(unsigned operation){Stage(operation);return Wait(operation);}
 void Verify(unsigned operation){
  assert(operation>=1 && operation<=3 && !pending && sfpu.drained && memory.at(W::Local)==operation
   && memory.at(W::Status+20)==operation && memory.at(W::Summary)==W::Magic && memory.at(W::Summary+8)==operation);
  unsigned reads=operation==2?0:header[46]>>1,flits=operation==2?0:header[47]>>1;
  assert(phasePackets==reads && privateWrites==1792 && partialWrites==(operation==3?3072U:0U)
   && scoreWrites==operation*6*header[49]);
  for(unsigned i=0;i<8;i++){
   unsigned delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0;
   assert(uint32_t(counts[i]-before[i])==delta && memory.at(W::Summary+(8+i)*4)==before[i] && memory.at(W::Summary+(16+i)*4)==counts[i]);
  }
  for(unsigned h=0;h<6;h++)for(unsigned t=0;t<header[49];t++){
   uint32_t expected=operation==1?produced.scores[h][header[48]+t]:operation==2?produced.exp[h][header[48]+t]:produced.probability[h][header[48]+t];
   assert(memory.at(W::Scores+(h*384+t)*4)==expected);
  }
  if(operation==2)for(unsigned h=0;h<6;h++){
   double expected=0;for(unsigned t=0;t<header[49];t++)expected+=double(PioneerAttentionFloat(produced.exp[h][header[48]+t]));
   uint32_t hi=memory.at(W::Local+(16+h*4)*4),lo=memory.at(W::Local+(17+h*4)*4);int e=int32_t(memory.at(W::Local+(18+h*4)*4));
   double actual=std::ldexp(double(hi)*4294967296.0+double(lo),e-52);assert(actual==expected);
  }
  for(unsigned i=0;i<1536;i++)assert(memory.at(W::HalfQuery+i*4)==produced.query[i] && memory.at(W::PrivateQuery+i*4)==produced.query[i]);
  for(unsigned i=0;i<128;i++)assert(memory.at(W::NewK+i*4)==produced.currentK[i] && memory.at(W::NewV+i*4)==produced.currentV[i]
   && memory.at(W::PrivateK+i*4)==produced.currentK[i] && memory.at(W::PrivateV+i*4)==produced.currentV[i]);
  for(unsigned i=0;i<64;i++)assert(memory.at(W::Header+i*4)==header[i]);
  for(unsigned a:Canaries())assert(memory.at(a)==0xdecafbad);
  assert(minStack>=0x6e000);
 }
 void Inert(){unsigned p=packets,c=sfpu.commands,w=partialWrites;for(unsigned i=0;i<32;i++)StepScalar();assert(p==packets && c==sfpu.commands && w==partialWrites);}
 void Stop(){
  Stage(4);while(pc!=0x40038 && pc!=0x40058)StepScalar();
  assert(pc==0x40038 && memory.at(0x17ffc8)==W::Magic && memory.at(0x17ffc4)==3
   && memory.at(W::Status+4)==4 && memory.at(W::Status+16)==2);Inert();
 }
};
static double Group(const char* worker,const char* original,unsigned group,unsigned tokens,unsigned trial,unsigned& workers,unsigned& maximumSteps,unsigned& maximumStack){
 ProducedGroup input(original,group,tokens,trial);
 std::vector<std::unique_ptr<PartitionMachine>> pool;
 for(unsigned ordinal=0;ordinal<24;ordinal++){pool.emplace_back(new PartitionMachine(worker,input,ordinal));assert(pool.back()->Boot());}
 for(unsigned phase=1;phase<=3;phase++)for(auto& p:pool){assert(p->RunPhase(phase));p->Verify(phase);p->Inert();}
 double difference=0,norm=0;unsigned priorBytes=0,packets=0;
 for(unsigned h=0;h<6;h++)for(unsigned d=0;d<256;d++){
  double sum=0;for(const auto& p:pool){sum+=double(PioneerAttentionFloat(p->memory.at(W::Partial+(h*512+d*2)*4)));
   sum+=double(PioneerAttentionFloat(p->memory.at(W::Partial+(h*512+d*2+1)*4)));}
  float expected=input.output[h][d];difference+=(sum-expected)*(sum-expected);norm+=double(expected)*expected;
 }
 double error=sqrt(difference/(norm?norm:1));assert(error<2e-5);
 for(auto& p:pool){
  priorBytes+=p->header[47]*64;packets+=p->packets;
  maximumSteps=std::max(maximumSteps,p->steps);maximumStack=std::max(maximumStack,0x70000-p->minStack);
  p->Stop();workers++;
 }
 assert(priorBytes==(tokens-1)*1024);
 printf("PASS ACTUAL QDW1 group%u tokens%u trial%u/24retained workers/6heads,threephases/PRIORbytes%u packets%u vs six independent heads bytes%u/EVERY QK-exp-F16prob bit+local binary64 sum exact/mergedPV L2%.9g;GLOBALPARAMS fromACTUAL_QDA,CONTROL_SYNTHETIC_NOT_NATIVE_OR_TIME\n",
  group,tokens,trial,priorBytes,packets,priorBytes*6,error);return error;
}
static void Geometry(){
 unsigned routes[96]={},ranges=0;
 for(unsigned w=1;w<=96;w++){
  routes[w-1]=W::Peer(w);for(unsigned old=0;old+1<w;old++)assert(routes[old]!=routes[w-1]);
 }
 for(unsigned tokens=1;tokens<=8192;tokens++){
  unsigned cursor=0,prior=0;
  for(unsigned ordinal=0;ordinal<24;ordinal++){
   unsigned first=W::First(tokens,ordinal),count=W::Count(tokens,ordinal);assert(count<=342);
   if(count){assert(first==cursor);cursor+=count;prior+=first>=tokens-1?0:std::min(count,tokens-1-first);}
   else assert(first>=tokens);
   ranges++;
  }
  assert(cursor==tokens && prior==tokens-1);
 }
 printf("PASS partition DATA ranges%u/ALL8192lengths/exact one-cover/no currentKV prefix read/all96unique physical routes/max342tokens;NOT native owner\n",ranges);
}
static unsigned Faults(const char* worker,const char* original){
 puts("START partition refusal suite/three-packet slices/globalparams fromACTUAL_QDA;NOT_SILICON_TIME");
 ProducedGroup input(original,0,1537);unsigned refused=0;
 // Both READ phases must stop at the first observed endpoint/receipt/
 // source/immutable-frame/job/parameter failure; no later packet/output.
 for(unsigned operation:{1U,3U})for(unsigned packet=1;packet<=3;packet++)for(unsigned kind=0;kind<(operation==3?6U:5U);kind++){
  PartitionMachine m(worker,input,0);assert(m.Boot());
  if(operation==3){assert(m.RunPhase(1));m.Verify(1);assert(m.RunPhase(2));m.Verify(2);}
  unsigned at=m.packets+packet;
  if(kind==0)m.failPacket=at;if(kind==1)m.counterFault=at;if(kind==2)m.sourceFault=at;
  if(kind==3)m.headerFault=at;if(kind==4)m.jobFaultPacket=at;if(kind==5)m.parameterFaultPacket=at;
  assert(!m.RunPhase(operation) && m.packets==at && !m.partialWrites
   && m.memory.at(W::Status+4)==operation-1 && m.memory.at(W::Summary)!=W::Magic);
  m.Inert();refused++;
 }
 for(unsigned field=0;field<64;field++){
  PartitionMachine m(worker,input,0);m.header[field]^=0x80000000U;
  if(W::Shape(m.header.data()) && !(field>=16 && field<32))continue;
  m.SetHeader();assert(!m.Wait(0) && !m.packets && !m.sfpu.commands);m.Inert();refused++;
 }
 for(unsigned field=0;field<16;field++){
  PartitionMachine m(worker,input,0);assert(m.Boot());m.Stage(1);m.memory[W::Job+field*4]^=1;
  assert(!m.Wait(1) && !m.packets && !m.sfpu.commands && !m.privateWrites);m.Inert();refused++;
 }
 for(unsigned word=1;word<=10;word++){
  PartitionMachine m(worker,input,0);assert(m.Boot());m.badReadback=word;
  assert(!m.RunPhase(1) && !m.packets && !m.partialWrites);m.Inert();refused++;
 }
 for(unsigned at:std::array<unsigned,4>{W::HalfQuery,W::HalfQuery+1535*4,W::NewK,W::NewV+127*4})for(uint32_t bad:{0x7f800000U,0x3f800001U}){
  PartitionMachine m(worker,input,0);assert(m.Boot());m.memory[at]=at>=W::NewK?0x7c007c00:bad;
  assert(!m.RunPhase(1) && !m.packets && !m.sfpu.commands && !m.partialWrites);m.Inert();refused++;
 }
 for(unsigned kind=0;kind<4;kind++){
  PartitionMachine m(worker,input,0);assert(m.Boot() && m.RunPhase(1));m.Verify(1);m.Stage(2);
  if(!kind)m.memory[W::Parameters]=0x7fc00000;
  if(kind==1)m.memory[W::Parameters]=0xff7fffff;
  if(kind==2)m.memory[W::Parameters+6*4]=1;
  if(kind==3)m.memory[W::Parameters+8*4]=0x3f000000;
  unsigned packets=m.packets;assert(!m.Wait(2) && m.packets==packets && !m.partialWrites);m.Inert();refused++;
 }
 for(uint32_t bad:{0U,0x7f800000U,0x3f800001U,0xbf000000U}){
  PartitionMachine m(worker,input,0);assert(m.Boot() && m.RunPhase(1) && m.RunPhase(2));m.Stage(3);
  m.memory[W::Parameters+8*4]=bad;unsigned packets=m.packets;
  assert(!m.Wait(3) && m.packets==packets && !m.partialWrites);m.Inert();refused++;
 }
 {PartitionMachine m(worker,input,0);assert(m.Boot());m.Stage(1);m.memory[W::Doorbell]=2;
  assert(!m.Wait(1) && !m.packets && !m.sfpu.commands);m.Inert();refused++;}
 {PartitionMachine m(worker,input,0);assert(m.Boot());m.sfpu.dropAck=true;
  assert(!m.RunPhase(1) && !m.packets && !m.partialWrites);m.Inert();refused++;}
 for(bool transpose:{false,true}){
  PartitionMachine m(worker,input,0);assert(m.Boot());m.sfpu.dropProduct=!transpose;m.sfpu.transpose=transpose;
  assert(m.RunPhase(1));bool changed=false;
  for(unsigned h=0;h<6;h++)for(unsigned t=0;t<m.header[49];t++)changed|=m.memory.at(W::Scores+(h*384+t)*4)!=input.scores[h][t];
  assert(changed);m.Inert();refused++; // OFFLINE observer,not CPU validation in Work code
 }
 printf("PASS partition refusals/detections%u/header+private-sequence+job+params+NoC-source-counters-timeouts/no following packets/UNKNOWN inert;numeric corruption caught ONLY by offline observer\n",refused);
 return refused;
}
#ifndef PIONEER_Q38_DENSE_PARTITION_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--full")));(void)&D::Run;
 setvbuf(stdout,nullptr,_IOLBF,0);Geometry();unsigned workers=0,maxSteps=0,maxStack=0;double error=0;
 for(unsigned group=0;group<4;group++)for(unsigned tokens:{1U,33U,65U,257U}){
  printf("START ACTUAL partition group%u tokens%u;NOT_SILICON_TIME\n",group,tokens);
  error=std::max(error,Group(argv[1],argv[2],group,tokens,0,workers,maxSteps,maxStack));
 }
 unsigned refused=Faults(argv[1],argv[2]);
 if(argc==4){
  puts("START ACTUAL FULL8192 partitioned group3/sixheads/24workers,GLOBALPARAMSfromACTUALsixQDAheads;NOT_SILICON_TIME");
  error=std::max(error,Group(argv[1],argv[2],3,8192,0,workers,maxSteps,maxStack));
 }
 printf("PASS OFFLINE ACTUAL QDW1 partition positives%u/refusals%u/maxStepsPerWorker%u/maxStack%u/maxMergedL2%.9g;ALL96bindings/fourKVgroups/sharedsix-headKV/privatephase1-2-3-STOP/NO CPU numeric feedback. Coordinator CONTROL+endpoints SYNTHETIC;ACTUAL96workerRootreduction/native/full64/model/Work/speed STILL OWED\n",workers,refused,maxSteps,maxStack,error);
}
#endif
