/* SPDX-License-Identifier: MIT */
// ACTUAL ROOT + ALL96 ACTUAL retained workers + FOUR ACTUAL QDP producers.
// NoC/fixed physical endpoints are MODELED;fabric copies bytes ONLY.
// Original whole-head QDA is an OBSERVER/ORACLE,not a parameter producer.
#define PIONEER_Q38_DENSE_PARTITION_MACHINE_ONLY
#include "pioneer_q38_dense_partition_image_test.cpp"
#include "../PioneerQ38DensePositiveSum.h"
#include "../PioneerQ38DensePartitionRootLayout.h"
namespace R=PioneerQ38DensePartitionRootLayout;
struct PartitionRootMachine:ScalarMachine {
 std::array<uint32_t,64> header={};std::array<uint32_t,16> source={};
 std::array<uint32_t,10> command={};std::array<uint32_t,8> counts={};
 std::array<std::unique_ptr<ProducedGroup>,4> oracle;
 std::array<std::unique_ptr<PartitionMachine>,96> child;
 std::array<bool,96> resetRead={};
 unsigned packets=0,writes=0,reads=0,doorbells=0,outputWords=0,qdpRuns=0,failPacket=0;
 unsigned counterFaultPacket=0,sourceFaultPacket=0,headerFaultPacket=0,frameFaultPacket=0,readbacks=0,badReadback=0;
 unsigned badWorker=0,badPhase=0,badKind=0,heldPCReads=0;
 bool pending=false;
 PartitionRootMachine(const char* root,const char* worker,const char* qdp,const char* qda,unsigned tokens,unsigned trial=0)
  :ScalarMachine(root,0,0){
  memory.clear();const unsigned start[]={R::Magic,1,64,3,tokens-1,129,17,96,4,24,256,3,R::Frames,R::Shadow,R::Reports,R::Proofs};
  for(unsigned i=0;i<16;i++)header[i]=start[i];
  for(unsigned i=0;i<16;i++)header[16+i]=source[i]=i?0x76540000+i:0x16005;
  for(unsigned i=32;i<40;i++)header[i]=i-31;
  header[40]=R::Input;header[41]=R::Gate;header[42]=R::Output;header[43]=R::Accumulate;
  header[44]=R::Scratch;header[45]=12288;header[46]=6144;header[47]=7168;assert(R::Shape(header.data()));
  for(unsigned i=0;i<64;i++)memory[R::Header+i*4]=header[i];counts.fill(0xfffffff8);
  for(unsigned group=0;group<4;group++){
   oracle[group]=std::make_unique<ProducedGroup>(qda,group,tokens,trial);
   DensePrepareMachine producer(qdp,group,tokens-1,trial);assert(producer.Run());producer.Verify();qdpRuns++;
   for(unsigned i=0;i<1536;i++){
    memory[R::Query+(group*1536+i)*4]=producer.memory.at(L::Query+i*4);
    memory[R::Gate+(group*1536+i)*4]=producer.memory.at(L::Gate+i*4);
   }
   for(unsigned i=0;i<128;i++){
    memory[R::NewK+group*512+i*4]=producer.memory.at(L::PackedK+i*4);
    memory[R::NewV+group*512+i*4]=producer.memory.at(L::PackedV+i*4);
   }
   for(unsigned ordinal=0;ordinal<24;ordinal++){
    unsigned w=group*24+ordinal;child[w]=std::make_unique<PartitionMachine>(worker,*oracle[group],ordinal);
    auto& p=*child[w];p.header[8]+=group*4194304;p.header[9]+=group*4194304;p.SetHeader();assert(p.Boot());
    // Force ACTUAL ROOT distribution/rounding. Never seed child inputs or
    // global parameters with an oracle product,ready flag or expected data.
    for(unsigned i=0;i<1792;i++)p.memory[W::HalfQuery+i*4]=0x7fc00001;
    for(unsigned i=0;i<64;i++)memory[R::Frames+w*256+i*4]=p.header[i];
   }
  }
  for(auto range:std::array<std::pair<unsigned,unsigned>,11>{{{R::Summary,256},{R::Scratch,16384},{R::Shadow,24576},
   {R::Reports,24576},{R::Proofs,24576},{R::Maxima,2304},{R::Parameters,256},{R::Counters,3072},
   {R::Output,24576},{R::Accumulate,49152},{R::InputShadow,28672}}})
   for(unsigned a=range.first;a<range.first+range.second;a+=4)memory[a]=0x7fc00001;
  for(unsigned i=0;i<6144;i++)memory[R::GateShadow+i*4]=0x7fc00001;
  for(unsigned a=0x6e000;a<0x70000;a+=4)memory[a]=0x7fc00001;
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
  for(unsigned a:Canaries())memory[a]=0xdecafbad;
 }
 static std::array<unsigned,12> Canaries(){return {R::Header-4,R::Header+256,R::Summary+256,R::Scratch-4,R::Scratch+16384,
  R::Frames+24576,R::Shadow+24576,R::Reports+24576,R::Proofs+24576,R::Maxima+2304,R::Counters+3072,R::End};}
 unsigned Index(uint32_t a)override{if(a==0x6000)return 0;assert(Range(a,R::Code,unsigned(code.size())) && !(a&3));return 1+(a-R::Code)/4;}
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,R::Code,unsigned(code.size()));}
 unsigned StepLimit()const override{return 2000000000U;}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(Range(a,R::Code,unsigned(code.size())))return l1[Index(a)];
  if(Range(a,0xffb20000,0x2000)){
   unsigned o=a-0xffb20000;if(o<40)return command[o/4]^(++readbacks==badReadback?1U:0U);
   for(unsigned i=0;i<4;i++)if(o==0x40+i*0x800)return i?0:unsigned(pending);
   if(o>=0x240 && o<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(o==DenseAttentionMachine::SourceOffset(i))return source[i];
   const unsigned at[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(o==at[i])return counts[i];assert(!"unqualified ROOT NoC register read");
  }
  assert(Range(a,0x6e000,8192) || Range(a,R::Header,256) || Range(a,R::Summary,256) || Range(a,R::Scratch,16384)
   || Range(a,R::Frames,24576) || Range(a,R::Shadow,24576) || Range(a,R::Reports,24576) || Range(a,R::Proofs,24576)
   || Range(a,R::Maxima,2304) || Range(a,R::Parameters,256) || Range(a,R::Counters,3072) || Range(a,R::Gate,24576)
   || Range(a,R::Output,24576) || Range(a,R::Accumulate,49152) || Range(a,R::Input,28672)
   || Range(a,R::InputShadow,28672) || Range(a,R::GateShadow,24576) || Range(a,0x17ffc0,12));
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));
  if(Range(a,0xffb20000,40)){assert(!pending);command[(a-0xffb20000)/4]=value;return;}
  if(a==0xffb20040){assert(value==1 && !pending);Submit();return;}
  assert(Range(a,0x6e000,8192) || Range(a,R::Summary,256) || Range(a,R::Scratch,16384) || Range(a,R::Shadow,24576)
   || Range(a,R::Reports,24576) || Range(a,R::Proofs,24576) || Range(a,R::Maxima,2304) || Range(a,R::Parameters,256)
   || Range(a,R::Counters,3072) || Range(a,R::Output,24576) || Range(a,R::Accumulate,49152)
   || Range(a,R::InputShadow,28672) || Range(a,R::GateShadow,24576) || Range(a,0x17ffc0,12));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,R::Output,24576))outputWords++;
  memory[a]=value; // NO code/raw input/frame/reset/PCI/canonical KV stores
 }
 unsigned Worker(unsigned route){for(unsigned w=0;w<96;w++)if(W::Peer(w+1)==route)return w;assert(!"foreign NoC route");return 0;}
 void Advance(unsigned w){
  auto& p=*child[w];
  for(unsigned step=0;step<10000;step++){
   if(p.pc==0x40058 || p.pc==0x40038 || ((p.pc==0x40064 || p.pc==0x40068) && p.memory.at(W::Doorbell)==p.memory.at(W::Status+4)))break;
   p.StepScalar();
  }
 }
 void Submit(){
  bool write=command[7]==0x212;assert(write || !command[7]);unsigned w=Worker(write?command[5]:command[2]);
  unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(local==R::Scratch+(remote&31) && bytes && local+bytes<=R::Scratch+16384 && !((local^remote)&31) && !(bytes&3) && !command[1] && !command[4] && !command[6] && !command[9]
   && (write?command[2]:command[5])==129);packets++;
  if(packets==failPacket){pending=true;return;}
  auto& p=*child[w];Advance(w);
  if(write){
   assert((remote==W::HalfQuery && bytes==6144) || ((remote==W::NewK || remote==W::NewV) && bytes==512)
    || ((remote==W::Parameters || remote==W::Job) && bytes==64) || (remote==W::Doorbell && bytes==4));
   for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
   if(remote==W::Doorbell){
    // Reset OBSERVER packet accounting only. No PC/register/private nonce/
    // tensor/physical counter is set;emitted worker instructions do all work.
    p.phase=p.memory.at(W::Job+4);if(p.phase<4){p.phasePackets=0;p.before=p.counts;}doorbells++;
   }
   writes++;
  }else{
   reads++;
   if(remote==0xffb121b0){assert(bytes==4);memory[local]=badWorker==w+1 && badPhase==p.phase && badKind==1?0x46801:0x46800;resetRead[w]=true;}
   else if(remote==0xffb13140){assert(bytes==4 && resetRead[w]);
    if(badWorker==w+1 && badPhase==p.phase && badKind==1)heldPCReads++;
    memory[local]=badWorker==w+1 && badPhase==p.phase && badKind==2?0xdeadbeec:p.pc;resetRead[w]=false;}
   else if(Range(remote,0xffb20000,0x2000)){
    assert(bytes==4);unsigned o=remote-0xffb20000;bool found=false;
    for(unsigned i=0;i<16;i++)if(o==DenseAttentionMachine::SourceOffset(i)){memory[local]=p.source[i];found=true;}
    const unsigned at[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
    for(unsigned i=0;i<8;i++)if(o==at[i]){memory[local]=p.counts[i];found=true;}
    assert(found);
    if(badWorker==w+1 && badPhase==p.phase && badKind==3 && o==DenseAttentionMachine::SourceOffset(11))memory[local]^=1;
   }else{
    assert((remote==W::Status && bytes==64) || (remote==W::Header && bytes==256) || (remote==W::Job && bytes==64)
     || (remote==W::Doorbell && bytes==4) || (remote==W::Summary && bytes==256) || (remote==W::Parameters && bytes==64)
     || (remote==W::Local && bytes==256) || (remote==W::Partial && bytes==12288));
    for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=p.memory.at(remote+i*4);
    if(badWorker==w+1 && badPhase==p.phase){
     if(badKind==4 && remote==W::Status)memory[local+15*4]^=1;
     if(badKind==5 && remote==W::Header)memory[local+40*4]^=1;
     if(badKind==6 && remote==W::Summary)memory[local]^=1;
     if(badKind==7 && remote==W::Parameters)memory[local]^=1;
     if(badKind==8 && remote==W::Local){memory[local+16*4]=0;memory[local+17*4]=1;}
     if(badKind==9 && remote==W::Partial)memory[local]=0x7fc00000;
     if(badKind==10 && remote==W::Doorbell)memory[local]++;
    }
   }
  }
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)>>6;counts[write?7:3]++;counts[2]++;counts[6]++;
  if(packets==counterFaultPacket)counts[3]++;
  if(packets==sourceFaultPacket)source[11]^=1;
  if(packets==headerFaultPacket)memory[R::Header+6*4]^=1;
  if(packets==frameFaultPacket)memory[R::Frames+w*256+40*4]^=1;
 }
 bool Run(){
  while(pc!=0x50038 && pc!=0x50058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  if(pc==0x50058){assert(memory.at(0x17ffc8)==R::Fault);return false;}
  assert(memory.at(0x17ffc8)==R::Magic && memory.at(0x17ffc4)==6144);return true;
 }
 double Verify(){
  assert(!pending && qdpRuns==4 && doorbells==384 && outputWords==6144 && memory.at(R::Summary)==R::Magic);
  double delta=0,norm=0;
  for(unsigned group=0;group<4;group++){
   for(unsigned h=0;h<6;h++)assert(memory.at(R::Parameters+(group*16+h)*4)==oracle[group]->maximum[h]
    && memory.at(R::Parameters+(group*16+8+h)*4)==oracle[group]->inverse[h]);
   for(unsigned h=0;h<6;h++)for(unsigned dimension=0;dimension<256;dimension++){
    unsigned index=group*1536+h*256+dimension;float gate=PioneerAttentionFloat(memory.at(R::Gate+index*4));
    double e=exp(-fabs(double(gate))),sigmoid=gate>=0?1/(1+e):e/(1+e);
    float expected=oracle[group]->output[h][dimension]*float(sigmoid),actual=PioneerAttentionFloat(memory.at(R::Output+index*4));
    assert(std::isfinite(actual));delta+=(double(actual)-expected)*(double(actual)-expected);norm+=double(expected)*expected;
   }
  }
  for(auto& p:child){
   assert(p->pc==0x40038 && p->memory.at(W::Status+4)==4 && p->memory.at(W::Status+16)==2);
   p->Verify(3); // final F16 probabilities/counters/retained inputs;phase1/2 bits have separate worker tests
  }
  for(unsigned a:Canaries())assert(memory.at(a)==0xdecafbad);
  double error=sqrt(delta/(norm?norm:1));assert(error<4e-6);return error;
 }
 void Inert(){unsigned before=packets,words=outputWords;for(unsigned i=0;i<32;i++)StepScalar();assert(packets==before && outputWords==words);}
};
static void SumMath(){
 uint32_t seed=0x13575319;unsigned sums=0,inverses=0;
 auto random=[&](){seed=seed*1664525U+1013904223U;return seed;};
 for(unsigned trial=0;trial<30000;trial++){
  PioneerQ38DensePositiveSum sum;double expected=0;
  for(unsigned j=0;j<24;j++){
   uint32_t hi=0x00100000|(random()&0xfffff),lo=random();int e=int(random()%123)-115;
   double term=std::ldexp(double(hi)*4294967296.0+double(lo),e-52);
   assert(sum.Add(hi,lo,e));expected+=term;
   assert(std::ldexp(double(sum.high)*4294967296.0+double(sum.low),sum.exponent-52)==expected);sums++;
  }
  if(expected>=1 && expected<=8192){float got;assert(sum.Inverse(got) && PioneerAttentionBits(got)==PioneerAttentionBits(float(1.0/expected)));inverses++;}
 }
 for(unsigned e=0;e<=13;e++){
  PioneerQ38DensePositiveSum sum;assert(sum.Add(0x100000,0,int(e)));float inverse;
  assert(sum.Inverse(inverse) && inverse==std::ldexp(1.f,-int(e)));inverses++;
 }
 printf("PASS paired-u32 ROOT sum additions%u/exact binary64 RNE/inverses%u/exact F32 reciprocal vs independent observer;NO native authority\n",sums,inverses);
}
static unsigned RootFaults(const char* root,const char* worker,const char* qdp,const char* qda){
 unsigned refused=0;puts("START actual ROOT refusal suite/real96workers+4QDP/NO native IO");
 for(unsigned kind=0;kind<5;kind++){
  auto m=std::make_unique<PartitionRootMachine>(root,worker,qdp,qda,1);
  if(!kind)m->failPacket=1;if(kind==1)m->counterFaultPacket=1;if(kind==2)m->sourceFaultPacket=1;
  if(kind==3)m->headerFaultPacket=1;if(kind==4)m->frameFaultPacket=1;
  assert(!m->Run() && m->packets==1 && !m->outputWords && m->memory.at(R::Summary)!=R::Magic);m->Inert();refused++;
 }
 for(unsigned word=1;word<=10;word++){
  auto m=std::make_unique<PartitionRootMachine>(root,worker,qdp,qda,1);m->badReadback=word;
  assert(!m->Run() && !m->packets && !m->outputWords);m->Inert();refused++;
 }
 for(unsigned kind=1;kind<=10;kind++){
  auto m=std::make_unique<PartitionRootMachine>(root,worker,qdp,qda,1);m->badWorker=1;m->badKind=kind;
  m->badPhase=kind==6?1:kind==7 || kind==8?2:kind==9?3:0;
  assert(!m->Run() && m->memory.at(R::Summary)!=R::Magic && !m->heldPCReads);m->Inert();refused++;
 }
 for(unsigned field:std::array<unsigned,12>{0,2,3,4,5,6,12,40,42,44,47,63}){
  auto m=std::make_unique<PartitionRootMachine>(root,worker,qdp,qda,1);m->memory[R::Header+field*4]^=0x80000000U;
  assert(!m->Run() && !m->packets && !m->outputWords);m->Inert();refused++;
 }
 for(unsigned kind=0;kind<4;kind++){
  auto m=std::make_unique<PartitionRootMachine>(root,worker,qdp,qda,1);
  if(!kind)m->memory[R::Frames+95*256+40*4]^=1;
  if(kind==1)m->memory[R::Input]=0x7fc00000;
  if(kind==2)m->memory[R::Gate+6143*4]=0x7f800000;
  if(kind==3)m->memory[R::NewV+511*4]=0x7c007c00;
  assert(!m->Run() && !m->packets && !m->outputWords);m->Inert();refused++;
 }
 printf("PASS actual ROOT refusals%u/UNKNOWN inert/no following packets or success marker;held-reset rejected BEFORE PC read/physical-source-status-header-proof-params-sum-PV-job-source/immutable inputs\n",refused);return refused;
}
int main(int argc,char** argv){
 assert(argc==5 || (argc==6 && !strcmp(argv[5],"--full")));(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;
 setvbuf(stdout,nullptr,_IOLBF,0);SumMath();unsigned cases=0,maxRoot=0,maxPeer=0;double maximum=0;
 for(unsigned tokens:{1U,33U,65U,257U}){
  printf("START ACTUAL QDR1 ROOT+ALL96workers+FOUR QDP N%u;NOT_SILICON_TIME\n",tokens);
  auto m=std::make_unique<PartitionRootMachine>(argv[1],argv[2],argv[3],argv[4],tokens);
  assert(m->Run());maximum=std::max(maximum,m->Verify());m->Inert();cases++;maxRoot=std::max(maxRoot,m->steps);
  for(auto& p:m->child)maxPeer=std::max(maxPeer,p->steps);
  printf("PASS ACTUAL ROOT+ALL96+FOUR QDP N%u/ROOTgenerated max+inverse+barriers+PVmerge+gate/ALL96STOP/noCPU numeric feedback;NOT_NATIVE_TIME\n",tokens);
 }
 if(argc==6){
  puts("START ACTUAL ROOT+ALL96 FULL8192 ALL24heads/FOUR QDP;NOT_SILICON_TIME");
  auto m=std::make_unique<PartitionRootMachine>(argv[1],argv[2],argv[3],argv[4],8192);
  assert(m->Run());maximum=std::max(maximum,m->Verify());m->Inert();cases++;maxRoot=std::max(maxRoot,m->steps);
  for(auto& p:m->child)maxPeer=std::max(maxPeer,p->steps);
 }
 unsigned refused=RootFaults(argv[1],argv[2],argv[3],argv[4]);
 printf("PASS OFFLINE ACTUAL QDR1 ROOT96-worker pipeline cases%u/refusals%u/maxROOTsteps%u/maxPeerSteps%u/maxGatedL2%.9g;FOUR actualQDP producers,actual globalmax/sum/inverse/query-rounding/distribution/privatephases/PVmerge/gate/ALL96STOP;NO host products/Stage/ready setters. ENDPOINTS+RAWWEIGHTS synthetic;NOT native/full64/originalGGUF/Work/inference-speed qualification\n",cases,refused,maxRoot,maxPeer,maximum);
}
