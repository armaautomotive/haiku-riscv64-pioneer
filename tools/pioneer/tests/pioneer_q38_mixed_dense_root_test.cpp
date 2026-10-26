/* SPDX-License-Identifier: MIT */
// Actual persistent ROOT + SAME119 mixed workers + four actual QDP.
// Only byte endpoints/physical register addresses are modeled. No CPU
// result/global-max/inverse/readiness data are supplied to the controller.
#define PIONEER_Q38_MIXED_WORKER_MACHINE_ONLY
#include "pioneer_q38_mixed_worker_test.cpp"
#define PIONEER_GRAPH_MATH_BASE_ALREADY_INCLUDED
#define PIONEER_Q38_DENSE_PARTITION_MACHINE_ONLY
#include "pioneer_q38_dense_partition_image_test.cpp"
#include "../PioneerQ38MixedDenseRootLayout.h"
#include <sys/wait.h>
#include <unistd.h>
namespace R=PioneerQ38MixedDenseRootLayout;
struct RetainedPeer:MixedPeer {
 const ProducedGroup* input=nullptr;
 unsigned denseFramePackets=0;
 using MixedPeer::MixedPeer;
 void SubmitDense()override{
  assert(input && (phase==1 || phase==3));const auto& f=denseHeader;
  unsigned global=f[48]+phasePackets*32,count=std::min(32U,std::min(f[49]-phasePackets*32,f[4]-global)),bytes=count*512,which=phase==3;
  const uint32_t expected[]={f[8+which]+global*512,0,f[12+which],W::Chunk,0,selfRoute,0,0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  densePackets++;denseFramePackets++;phasePackets++;
  const auto& raw=which?input->values:input->keys;
  for(unsigned i=0;i<bytes/4;i++)memory[W::Chunk+i*4]=raw[global*256+i*2]|(uint32_t(raw[global*256+i*2+1])<<16);
  countBase[0]++;countBase[1]+=bytes/64;countBase[2]++;countBase[3]++;countBase[6]++;
 }
};
struct MixedRootMachine:ScalarMachine {
 std::array<uint32_t,64> header={},boot={};std::array<uint32_t,16> source={};
 std::array<uint32_t,10> command={};std::array<uint32_t,8> counts={};
 std::array<std::unique_ptr<RetainedPeer>,119> child;
 std::array<std::unique_ptr<ProducedGroup>,4> oracle;
 std::array<uint32_t,119> firstNonce={},firstPackets={};
 std::array<bool,119> resetRead={};
 const char *qdpImage,*qdaImage;
 unsigned sequence=0,packets=0,writes=0,reads=0,doorbells=0,outputWords=0,qdpRuns=0;
 unsigned failPacket=0,counterFaultPacket=0,sourceFaultPacket=0,frameFaultPacket=0,bootFaultPacket=0,headerFaultPacket=0;
 unsigned badWorker=0,badPhase=0,badKind=0,heldPCReads=0,badReadback=0,readbacks=0;
 bool pending=false;
 MixedRootMachine(const char* image,const char* worker,const char* qdp,const char* qda):ScalarMachine(image,0,0),qdpImage(qdp),qdaImage(qda){
  memory.clear();counts.fill(0xfffffff8);
  boot[0]=R::Magic;boot[1]=1;boot[2]=64;boot[3]=129;boot[4]=0x16005;boot[5]=119;boot[6]=8192;boot[7]=64;
  for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;
  for(unsigned i=0;i<16;i++)boot[16+i]=source[i]=i?0x76540000+i:0x16005;
  for(unsigned i=0;i<64;i++){memory[R::Boot+i*4]=boot[i];memory[R::Header+i*4]=0;}
  memory[R::Doorbell]=0;for(unsigned i=0;i<16;i++)memory[R::Status+i*4]=0xdeadbeef;
  for(unsigned w=0;w<119;w++){
   child[w]=std::make_unique<RetainedPeer>(worker,w+1,w%3==0?8:w%3==1?12:14,w%3==2?6144:5120);
   auto& p=*child[w];assert(p.BootMixed());
   // Actual arithmetic closes different nonzero private job histories.
   // No nonce/PC/register/status or tensor array is set by this fixture.
   for(unsigned i=0;i<1+w%3;i++){if(i)FreshInputs(p);assert(p.MatrixMixed());}
  }
  for(unsigned head:{0U,31U,47U}){
   auto& a=*child[head*2];auto& b=*child[head*2+1];
   // Bring the pair to the SAME real sequence using actual extra matrix
   // jobs only; never write the worker's private/current sequence.
   while(a.sequence<b.sequence)assert(a.MatrixMixed());while(b.sequence<a.sequence)assert(b.MatrixMixed());
   for(auto* p:{&a,&b}){unsigned next=p->sequence+1;p->StageGdn(next,1000+next);p->StageMixed(8,0,0,1000+next);}
   assert(RunPersistentGdnPair(a,b,a.sequence));a.VerifyGdn(a.sequence);b.VerifyGdn(b.sequence);
  }
  for(auto range:std::array<std::pair<unsigned,unsigned>,17>{{{R::Summary,256},{R::Scratch,16384},{R::Shadow,24576},
   {R::Reports,24576},{R::Proofs,24576},{R::Maxima,2304},{R::Parameters,256},{R::Counters,119*32},
   {R::Nonces,119*4},{R::PeerBoots,119*256},{R::PeerJobs,119*256},{R::Output,24576},{R::Accumulate,49152},
   {R::InputShadow,28672},{R::GateShadow,24576},{0x6e000,8192},{0x17ffc0,12}}})
   for(unsigned a=range.first;a<range.first+range.second;a+=4)memory[a]=0x7fc00001;
  for(unsigned a:Canaries())memory[a]=0xdecafbad;
 }
 static std::array<unsigned,15> Canaries(){return {R::Header-4,R::Header+256,R::Summary+256,R::Scratch-4,R::Scratch+16384,
  R::Frames+24576,R::Shadow+24576,R::Reports+24576,R::Proofs+24576,R::Maxima+2304,R::Counters+119*32,
  R::Nonces+119*4,R::PeerBoots+119*256,R::PeerJobs+119*256,R::Boot-4};}
 unsigned Index(uint32_t a)override{if(a==0x6000)return 0;assert(Range(a,R::Code,unsigned(code.size())) && !(a&3));return 1+(a-R::Code)/4;}
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,R::Code,unsigned(code.size()));}
 unsigned StepLimit()const override{return 4000000000U;}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(Range(a,R::Code,unsigned(code.size())))return l1[Index(a)];
  if(Range(a,0xffb20000,0x2000)){
   unsigned o=a-0xffb20000;if(o<40)return command[o/4]^(++readbacks==badReadback?1U:0U);
   for(unsigned i=0;i<4;i++)if(o==0x40+i*0x800)return i?0:unsigned(pending);
   if(o>=0x240 && o<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(o==DenseAttentionMachine::SourceOffset(i))return source[i];
   for(unsigned i=0;i<8;i++)if(o==FfnPersistentPeer::CounterOffset(i))return counts[i];assert(!"foreign ROOT NoC register");
  }
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));
  if(Range(a,0xffb20000,40)){assert(!pending);command[(a-0xffb20000)/4]=value;return;}
  if(a==0xffb20040){assert(value==1 && !pending);Submit();return;}
  bool writable=Range(a,0x6e000,8192) || Range(a,R::Summary,256) || Range(a,R::Status,64) || Range(a,R::Scratch,16384)
   || Range(a,R::Shadow,24576) || Range(a,R::Reports,24576) || Range(a,R::Proofs,24576) || Range(a,R::Maxima,2304)
   || Range(a,R::Parameters,256) || Range(a,R::Counters,119*32) || Range(a,R::Nonces,119*4) || Range(a,R::PeerBoots,119*256)
   || Range(a,R::PeerJobs,119*256) || Range(a,R::Output,24576) || Range(a,R::Accumulate,49152)
   || Range(a,R::InputShadow,28672) || Range(a,R::GateShadow,24576);
  assert(writable);if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,R::Output,24576))outputWords++;memory[a]=value;
 }
 unsigned Worker(unsigned route){for(unsigned w=0;w<119;w++)if(M::Peer(w+1)==route)return w;assert(false);return 0;}
 void Advance(unsigned w){
  auto& p=*child[w];for(unsigned i=0;i<10000;i++){
   if(p.GdnFaulted() || p.pc==p.stopPC || ((p.pc==p.waitPC || p.pc==p.waitPC+4) && p.memory.at(M::Doorbell)==p.memory.at(M::Status+4)))break;
   p.StepBlock();
  }
 }
 void Submit(){
  bool write=command[7]==0x212;assert(write || !command[7]);unsigned w=Worker(write?command[5]:command[2]);
  unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(local==R::Scratch+(remote&31) && bytes && local+bytes<=R::Scratch+16384 && !((local^remote)&31) && !(bytes&3)
   && !command[1] && !command[4] && !command[6] && !command[9] && (write?command[2]:command[5])==129);
  packets++;if(packets==failPacket){pending=true;return;}auto& p=*child[w];Advance(w);
  if(write){
   assert(w<96 && ((remote==W::Header && bytes==256) || (remote==W::HalfQuery && bytes==6144)
    || ((remote==W::NewK || remote==W::NewV) && bytes==512) || (remote==W::Parameters && bytes==64)
    || (remote==M::Job && bytes==256) || (remote==M::Doorbell && bytes==4)));
   for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
   if(remote==M::Doorbell){
    // OBSERVER selection only. No private/history/PC/tensor/counter state
    // changes. The actual worker reads its own job and owns its sequence.
    unsigned op=p.memory.at(M::Job+8);assert(op>=9 && op<=11);p.phase=op-8;p.phasePackets=0;
    p.denseActive=p.activation=true;p.gdnActive=p.convActive=p.outputActive=false;
    for(unsigned i=0;i<64;i++)p.denseHeader[i]=p.memory.at(W::Header+i*4);doorbells++;
   }writes++;
  }else{
   reads++;
   if(remote==0xffb121b0){assert(bytes==4);memory[local]=badWorker==w+1 && badPhase==p.phase && badKind==1?0x46801:0x46800;resetRead[w]=true;}
   else if(remote==0xffb13140){assert(bytes==4 && resetRead[w]);
    if(badWorker==w+1 && badPhase==p.phase && badKind==1)heldPCReads++;
    memory[local]=badWorker==w+1 && badPhase==p.phase && badKind==2?0x20060:p.pc;resetRead[w]=false;
   }else if(Range(remote,0xffb20000,0x2000)){
    assert(bytes==4);unsigned o=remote-0xffb20000;bool found=false;
    for(unsigned i=0;i<16;i++)if(o==DenseAttentionMachine::SourceOffset(i)){memory[local]=p.source[i];found=true;}
    for(unsigned i=0;i<8;i++)if(o==FfnPersistentPeer::CounterOffset(i)){memory[local]=p.Count(i);found=true;break;}
    assert(found);if(badWorker==w+1 && badPhase==p.phase && badKind==3 && o==DenseAttentionMachine::SourceOffset(11))memory[local]^=1;
   }else{
    assert((remote==M::Status && bytes==64) || (remote==M::Boot && bytes==256) || (remote==M::Job && bytes==256) || (remote==0x83f00 && bytes==256)
     || (remote==M::Doorbell && bytes==4) || (remote==W::Header && bytes==256) || (remote==W::Job && bytes==64)
     || (remote==W::Doorbell && bytes==4) || (remote==W::Summary && bytes==256) || (remote==W::Parameters && bytes==64)
     || (remote==W::Local && bytes==256) || (remote==W::Partial && bytes==12288));
    for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=p.memory.at(remote+i*4);
    if(badWorker==w+1 && badPhase==p.phase){
     if(badKind==4 && remote==M::Status)memory[local+15*4]^=1;
     if(badKind==5 && remote==M::Boot)memory[local+8*4]^=1;
     if(badKind==6 && remote==M::Job)memory[local+4]^=1;
     if(badKind==7 && remote==W::Header)memory[local+40*4]^=1;
     if(badKind==8 && remote==W::Summary)memory[local+4]^=1;
     if(badKind==9 && remote==W::Parameters)memory[local]^=1;
     if(badKind==10 && remote==W::Local){memory[local+16*4]=0;memory[local+17*4]=1;}
     if(badKind==11 && remote==W::Partial)memory[local]=0x7fc00000;
    }
   }
  }
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)>>6;counts[write?7:3]++;counts[2]++;counts[6]++;
  if(packets==counterFaultPacket)counts[3]++;
  if(packets==sourceFaultPacket)source[11]^=1;
  if(packets==headerFaultPacket)memory[R::Header+48*4]^=1;
  if(packets==bootFaultPacket)memory[R::Boot+8*4]^=1;
  if(packets==frameFaultPacket)memory[R::Frames+(w<96?w:0)*256+40*4]^=1;
 }
 bool Wait(unsigned n){
  while(pc!=0x5009c && !(pc==0x5004c && memory.at(R::Status+4)==n))StepScalar();
  if(pc==0x5009c){assert(memory.at(R::Status+8)==R::Fault);return false;}
  assert(memory.at(R::Status+12)==R::Magic && !memory.at(R::Status+8) && !memory.at(R::Status+16));return true;
 }
 void Stage(unsigned tokens,unsigned layer=3,unsigned trial=0){
  sequence++;outputWords=doorbells=qdpRuns=0;
  const unsigned first[]={R::Magic,1,64,layer,tokens-1,129,1000+sequence,96,4,24,256,3,R::Frames,R::Shadow,R::Reports,R::Proofs};
  for(unsigned i=0;i<16;i++){header[i]=first[i];header[16+i]=source[i];}
  for(unsigned i=0;i<8;i++)header[32+i]=boot[8+i];
  header[40]=R::Input;header[41]=R::Gate;header[42]=R::Output;header[43]=R::Accumulate;header[44]=R::Scratch;header[45]=12288;header[46]=6144;header[47]=7168;
  header[48]=sequence;header[49]=119;header[50]=R::PeerBoots;header[51]=R::PeerJobs;header[52]=R::Nonces;
  assert(R::Shape(header.data()));for(unsigned i=0;i<64;i++)memory[R::Header+i*4]=header[i];
  for(unsigned g=0;g<4;g++){
   oracle[g]=std::make_unique<ProducedGroup>(qdaImage,g,tokens,trial);
   DensePrepareMachine producer(qdpImage,g,tokens-1,trial);producer.header[3]=layer;producer.SetHeader();assert(producer.Run());producer.Verify();qdpRuns++;
   for(unsigned i=0;i<1536;i++){memory[R::Query+(g*1536+i)*4]=producer.memory.at(L::Query+i*4);memory[R::Gate+(g*1536+i)*4]=producer.memory.at(L::Gate+i*4);}
   for(unsigned i=0;i<128;i++){memory[R::NewK+g*512+i*4]=producer.memory.at(L::PackedK+i*4);memory[R::NewV+g*512+i*4]=producer.memory.at(L::PackedV+i*4);}
   for(unsigned o=0;o<24;o++){
    unsigned w=g*24+o;auto& p=*child[w];p.input=oracle[g].get();p.phase=0;p.denseFramePackets=0;
    std::array<uint32_t,64> f={};const unsigned start[]={W::Magic,1,64,layer,tokens-1,g,o,tokens,
     0x10000000+g*4194304,0x20000000+g*4194304,0,1,W::Route(0),W::Route(1),p.selfRoute,w+1};
    for(unsigned i=0;i<16;i++){f[i]=start[i];f[16+i]=p.source[i];}for(unsigned i=0;i<8;i++)f[32+i]=boot[8+i];
    f[40]=header[6];f[41]=256;f[42]=0x3d800000;f[43]=32;f[44]=8192;f[45]=16384;f[48]=W::First(tokens,o);f[49]=W::Count(tokens,o);
    unsigned prior=f[48]>=f[4]?0:std::min(f[49],f[4]-f[48]);f[46]=2*((prior+31)>>5);f[47]=prior*16;
    f[50]=3;f[51]=6;f[52]=24;f[53]=384;f[54]=1536;f[55]=W::Magic;assert(W::Shape(f.data()));
    for(unsigned i=0;i<64;i++)memory[R::Frames+w*256+i*4]=f[i];
    for(unsigned i=0;i<1792;i++)p.memory[W::HalfQuery+i*4]=0x7fc00001;
   }
  }
  for(unsigned w=0;w<119;w++){firstNonce[w]=child[w]->memory.at(M::Status+4);firstPackets[w]=child[w]->densePackets;}
  memory[R::Doorbell]=sequence;
 }
 double Verify(){
  assert(qdpRuns==4 && doorbells==288 && outputWords==6144 && memory.at(R::Summary)==R::Magic && memory.at(R::Summary+52*4)==sequence);
  double delta=0,norm=0;
  for(unsigned g=0;g<4;g++)for(unsigned h=0;h<6;h++){
   assert(memory.at(R::Parameters+(g*16+h)*4)==oracle[g]->maximum[h] && memory.at(R::Parameters+(g*16+8+h)*4)==oracle[g]->inverse[h]);
   for(unsigned dim=0;dim<256;dim++){
    unsigned index=g*1536+h*256+dim;float gate=Float(memory.at(R::Gate+index*4));double e=exp(-fabs(double(gate))),sigmoid=gate>=0?1/(1+e):e/(1+e);
    float expected=oracle[g]->output[h][dim]*float(sigmoid),actual=Float(memory.at(R::Output+index*4));
    assert(std::isfinite(actual));delta+=(double(actual)-expected)*(double(actual)-expected);norm+=double(expected)*expected;
   }
  }
  for(unsigned w=0;w<119;w++){
   auto& p=*child[w];assert(p.pc==p.waitPC || p.pc==p.waitPC+4);
   assert(p.memory.at(M::Status+4)==firstNonce[w]+(w<96?3:0) && !p.memory.at(M::Status+8) && !p.memory.at(M::Status+16) && !p.memory.at(M::Status+20));
   if(w<96){const auto& f=p.denseHeader;assert(p.denseFramePackets==f[46]);
    for(unsigned h=0;h<6;h++)for(unsigned t=0;t<f[49];t++)assert(p.memory.at(W::Scores+(h*384+t)*4)==p.input->probability[h][f[48]+t]);
   }else assert(p.densePackets==firstPackets[w]);
  }
  for(unsigned a:Canaries())assert(memory.at(a)==0xdecafbad);
  double error=sqrt(delta/(norm?norm:1));assert(error<4e-6);return error;
 }
 void Interlude(){
  for(unsigned w:{0U,62U,95U,118U}){
   auto& p=*child[w];p.sequence=p.memory.at(M::Status+4);FreshInputs(p);assert(p.MatrixMixed());
  }
 }
 void Inert(){unsigned p=packets,o=outputWords;for(unsigned i=0;i<32;i++)StepScalar();assert(p==packets && o==outputWords);}
};
static unsigned Refusals(MixedRootMachine& base){
 unsigned refused=0;
 auto branch=[&](unsigned kind,unsigned arg){
  // TEST ONLY OS snapshot of the ALREADY EXECUTED whole VM world. This
  // never resets/copies emulated tensor state at a production transition,
  // invents a private nonce, or supplies a numerical expected product.
  fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){
   auto& m=base;m.Stage(1);
   if(kind==0)m.failPacket=1;
   if(kind==1)m.counterFaultPacket=1;
   if(kind==2)m.sourceFaultPacket=1;
   if(kind==3)m.headerFaultPacket=1;
   if(kind==4)m.bootFaultPacket=1;
   if(kind==5)m.frameFaultPacket=1;
   if(kind==6)m.badReadback=arg;
   if(kind==7){
    m.badWorker=1;m.badKind=arg;m.badPhase=arg==7 || arg==8?1:arg==9 || arg==10?2:arg==11?3:0;
   }
   if(kind==8){m.badWorker=119;m.badKind=arg;m.badPhase=0;}
   if(kind==9)m.memory[R::Header+arg*4]^=0x80000000U;
   if(kind==10)m.child[118]->countBase[arg]++; // BEFORE admission: must not absorb as baseline
   if(kind==11)m.memory[R::Frames+95*256+40*4]^=1;
   if(kind==12)m.memory[R::Input]=0x7fc00000;
   if(kind==13)m.memory[R::Gate+6143*4]=0x7f800000;
   if(kind==14)m.memory[R::NewV+511*4]=0x7c007c00;
   bool okay=m.Wait(m.sequence);
   if(okay || m.memory.at(R::Summary)==R::Magic || m.heldPCReads)fprintf(stderr,"MIXED_ROOT_FAULT kind%u arg%u okay%u packets%u\n",kind,arg,unsigned(okay),m.packets);
   assert(!okay && m.memory.at(R::Summary)!=R::Magic && !m.heldPCReads);
   if(kind<=5)assert(m.packets==1 && !m.outputWords);
   if(kind==6 || (kind>=9 && kind!=10))assert(!m.packets && !m.outputWords);
   m.Inert();_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));refused++;
 };
 for(unsigned kind=0;kind<=5;kind++)branch(kind,0);
 for(unsigned word=1;word<=10;word++)branch(6,word);
 for(unsigned kind=1;kind<=11;kind++)branch(7,kind);
 for(unsigned kind=1;kind<=6;kind++)branch(8,kind);
 for(unsigned word:{0U,2U,3U,4U,5U,6U,12U,40U,42U,44U,47U,48U,49U,50U,51U,52U,63U})branch(9,word);
 for(unsigned c=0;c<8;c++)if(c!=6)branch(10,c);
 for(unsigned kind=11;kind<=14;kind++)branch(kind,0);
 printf("PASS ACTUAL mixed ROOT refusals%u/from EXECUTED119-worker world/no following IO/success/held-reset rejected before PC/closed-receipt counters cannot be absorbed\n",refused);return refused;
}
#ifndef PIONEER_Q38_MIXED_DENSE_ROOT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==5 || (argc==6 && !strcmp(argv[5],"--full")));(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;
 setvbuf(stdout,nullptr,_IOLBF,0);auto m=std::make_unique<MixedRootMachine>(argv[1],argv[2],argv[3],argv[4]);assert(m->Wait(0));
 unsigned refusals=Refusals(*m);double maximum=0;unsigned cases=0;
 for(unsigned n:{1U,33U,65U,257U,8192U}){
  if(n==8192 && argc!=6)continue;
  m->Stage(n,n==1?3:n==33?7:n==65?11:63);assert(m->Wait(m->sequence));maximum=std::max(maximum,m->Verify());m->Inert();cases++;
  printf("PASS ACTUAL persistent ROOT+SAME119 mixed workers/FOUR QDP frame%u N%u ALL96 dynamic nonces/global reductions/23 idle preserved/NO STOP or reboot\n",m->sequence,n);
  m->Interlude();
 }
 printf("PASS NEW retained mixed graph frames%u/refusals%u/maxRelativeL2%.12g/ROOTstack%u;synthetic byte/physical endpoints,NO native/fullmodel/throughput grant\n",cases,refusals,maximum,0x70000-m->minStack);
}
#endif
