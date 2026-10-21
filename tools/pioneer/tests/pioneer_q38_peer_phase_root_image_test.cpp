/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SENTINEL_ROOT_IMAGE_MODEL_ONLY
#include "pioneer_q38_sentinel_root_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38PeerPhaseData.h"
// Synthetic clocks are DATA,not a timing/performance model. Every original
// packet/source/counter/PC/DMA observer is inherited without relaxation.
template<class Base> struct PeerPhaseRoot:CycleRoot<Base>{
 using CycleRoot<Base>::CycleRoot;
 unsigned metricReads=0,selected=0,corruptData=16;bool loseMetric=false,badClock=false;
 unsigned pcReads[119]={},resetReads[119]={};uint32_t expected[7]={};
 virtual void MetricWords(unsigned worker,uint32_t (&data)[16]){
  unsigned part=0x74080+(worker-1)*32;
  uint32_t words[]={0x31545051,1,this->memory.at(part+12),worker,this->kind,this->k,this->memory.at(part+8),unsigned(!badClock),
   0x12345678,0x12,0x23456789,0x23,0x3456789a,0x34,0,0};
  for(unsigned w=0;w<16;w++)data[w]=words[w];
  if(badClock)for(unsigned w=8;w<14;w++)data[w]=0;
 }
 void Packet()override{
  bool write=this->command[7]==0x212,pci=(write?this->command[5]:this->command[2])==1555;
  unsigned remote=write?this->command[3]:this->command[0];
  if(!write&&!pci&&remote==0x84500){
   assert(this->WorkCached()&&this->operation==1&&this->AllFinished()&&this->gathers==this->active
    &&this->pending&&!this->unknown&&!this->dmaPackets&&!this->command[1]&&!this->command[4]
    &&!this->command[6]&&!this->command[7]&&!this->command[9]&&this->command[5]==129
    &&this->command[3]==0x75000&&this->command[8]==64);
   for(unsigned w=0;w<this->active;w++)assert(pcReads[w]==2&&resetReads[w]==1);
   unsigned worker=this->Worker(this->command[2]);assert(worker&&worker<=this->active);
   unsigned heaviest=0;for(unsigned w=1;w<this->active;w++)if(this->memory.at(0x74088+w*32)>=this->memory.at(0x74088+heaviest*32))heaviest=w;
   assert(worker==heaviest+1);selected=worker;metricReads++;
   if(loseMetric){this->faultAt=this->packets;return;}
   uint32_t data[16];MetricWords(worker,data);
   assert(PioneerQ38PeerPhaseReadback(data,worker,this->memory.at(0x74080+(worker-1)*32+12),this->kind,this->k,this->memory.at(0x74088+(worker-1)*32),expected));
   if(corruptData<16)data[corruptData]^=1;
   for(unsigned w=0;w<16;w++)this->memory[0x75000+w*4]=data[w];
   this->counts[0]++;this->counts[1]++;this->counts[2]++;this->counts[3]++;this->responses++;this->pending=false;return;
  }
  if(!write&&!pci){unsigned worker=this->Worker(this->command[2]);
   if(remote==0xffb13140)pcReads[worker-1]++;
   if(remote==0xffb121b0)resetReads[worker-1]++;
  }
  CycleRoot<Base>::Packet();
 }
 bool RootDiagnosticTrailerValid()const override{
  if(!this->WorkCached())return CompactReturnMachine<Base>::RootDiagnosticTrailerValid();
  uint32_t stats[64]={};for(unsigned w=57;w<64;w++){stats[w]=this->memory.at(0x7f800+w*4);assert(stats[w]==expected[w-57]);}
  return PioneerQ38PeerPhaseEnvelopeValid(stats);
 }
 void CheckRead(){assert(metricReads==1&&selected&&RootDiagnosticTrailerValid()&&this->reads.size()==12);}
 void FreshObservation(){
  metricReads=selected=this->dmaPackets=0;this->reads.clear();
  for(unsigned w=0;w<119;w++)pcReads[w]=resetReads[w]=0;
  std::fill(this->host.begin(),this->host.end(),0xdeadbeef);
 }
};
struct ActualPeerPhaseRoot:PeerPhaseRoot<CooperativeDispatch>{
 using PeerPhaseRoot<CooperativeDispatch>::PeerPhaseRoot;
 void MetricWords(unsigned worker,uint32_t (&data)[16])override{
  auto& p=*actual.at(worker-1);assert(!p.pending&&!p.rawPending&&p.phaseQueue.empty()&&p.pendingAccumulator.empty());
  p.CheckMetrics(!badClock,peers[worker-1].requested);
  for(unsigned w=0;w<16;w++)data[w]=p.memory.at(0x84500+w*4);
 }
};
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);unsigned cases=0;
 for(unsigned n:{1U,2U,7U,119U})for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U}){
  printf("PEER_PHASE_ROOT workers%u type%u K%u\n",n,type,k);
  CycleRoot<DispatchMachine> old(argv[1],n,type,k);PeerPhaseRoot<DispatchMachine> next(argv[2],n,type,k);
  assert(old.RunRoot()&&next.RunRoot());old.CheckCycles();next.CheckRead();old.VerifyCompactHost();next.VerifyCompactHost();
  assert(old.host==next.host&&next.packets==old.packets+1&&next.responses==old.responses+1&&next.gathers==old.gathers&&next.dmaPackets==old.dmaPackets);
  for(unsigned w=0;w<57;w++){
   unsigned delta=w==6||w==8||w==24||w==25||w==26||w==27||w==30?1:0;
   assert(next.memory.at(0x7f800+w*4)==old.memory.at(0x7f800+w*4)+delta);
  }
  next.Configure(2);next.DmaHeader();next.FreshObservation();assert(next.RunRoot());next.CheckRead();next.VerifyCompactHost();cases+=2;
  // STOP must not touch timing data OR clocks,including inactive peers.
  next.active=119;next.Configure(3,true);next.FreshObservation();
  assert(next.RunRoot()&&!next.metricReads&&next.reads.empty()&&!next.dmaPackets);
  for(unsigned w=57;w<64;w++)assert(!next.memory.at(0x7f800+w*4));cases++;
 }
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U}){
  PeerPhaseRoot<DispatchMachine> m(argv[2],7,type,k);
  for(unsigned w=8;w<13;w++)m.memory[0x74000+w*4]=0;
  assert(m.RunRoot()&&!m.metricReads&&m.reads.empty()&&!m.dmaPackets);
  for(unsigned w=57;w<64;w++)assert(!m.memory.at(0x7f800+w*4));cases++;
 }
 // Invalid clocks are not failed execution;partial/mismatched envelopes ARE
 // rejected before DMA. Timing values themselves stay opaque unsigned DATA.
 for(unsigned field:{0U,1U,2U,3U,4U,5U,6U,7U,14U,15U}){
  PeerPhaseRoot<DispatchMachine> m(argv[2],7);m.corruptData=field;
  assert(!m.RunRoot()&&m.unknown&&!m.pending&&m.metricReads==1&&!m.dmaPackets);m.VerifyFaultInert();
 }
 {PeerPhaseRoot<DispatchMachine> m(argv[2],7);m.badClock=true;assert(m.RunRoot());m.CheckRead();m.VerifyCompactHost();}
 {PeerPhaseRoot<DispatchMachine> m(argv[2],7);m.loseMetric=true;assert(!m.RunRoot()&&m.unknown&&m.pending&&m.metricReads==1&&!m.dmaPackets);m.VerifyFaultInert();}
 for(unsigned field=0;field<64;field++){
  PeerPhaseRoot<DispatchMachine> m(argv[2],7);m.corruptWorker=7;m.corruptProof=field;
  assert(!m.RunRoot()&&m.unknown&&!m.metricReads&&!m.dmaPackets);m.VerifyFaultInert();
 }
 for(unsigned worker:{1U,3U,7U}){
  PeerPhaseRoot<DispatchMachine> m(argv[2],7);m.badPcWorker=worker;
  assert(!m.RunRoot()&&m.unknown&&!m.metricReads&&!m.dmaPackets);m.VerifyFaultInert();
 }
 for(unsigned packet:{1U,20U,40U}){
  PeerPhaseRoot<DispatchMachine> m(argv[2],7);m.faultAt=packet;
  assert(!m.RunRoot()&&m.unknown&&m.pending&&!m.metricReads&&!m.dmaPackets);m.VerifyFaultInert();
 }
 for(unsigned packet:{1U,4U,8U}){
  PeerPhaseRoot<DispatchMachine> m(argv[2],119);m.failDma=packet;
  assert(!m.RunRoot()&&m.unknown&&m.pending&&m.metricReads==1&&m.dmaPackets==packet);m.VerifyFaultInert();
 }
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,17408U}){
  printf("PEER_PHASE_BOTH type%u K%u\n",type,k);
  ActualPeerPhaseRoot m(argv[2],argv[3],2,type,k);assert(m.RunRoot());m.CheckRead();m.VerifyCompactHost();
  for(auto& peer:m.actual)assert(peer->CorrectRows()&&!peer->dotProbeReads);cases++;
 }
 for(unsigned type:{8U,12U,14U}){
  ActualPeerPhaseRoot m(argv[2],argv[3],2,type,5120);m.badClock=true;
  for(auto& peer:m.actual)peer->unstable=true;
  assert(m.RunRoot());m.CheckRead();m.VerifyCompactHost();for(auto& peer:m.actual)assert(peer->CorrectRows());cases++;
 }
 printf("PASS OFFLINE actual peer timing-return root:%ushape+realpeer cases;ONE fixed64B read ONLY AFTER ALL physical status/2PC/reset/proof/output audits,EXACT accounted counters+outputs;identity/NoC/proof/PC failures UNKNOWN/inert beforeDMA;NO native/timing/performance qualification\n",cases);
}
