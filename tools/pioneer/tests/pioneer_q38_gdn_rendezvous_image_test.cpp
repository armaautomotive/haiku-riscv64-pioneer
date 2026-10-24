/* SPDX-License-Identifier: MIT */
// Actual QGT1 RV32/SFPU/CSR instructions. FABRICATED clock,not silicon time.
#define PIONEER_Q38_GDN_GATED_RESIDENT_MACHINE_ONLY
#include "pioneer_q38_gdn_gated_resident_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnRendezvousShape.h"
struct RendezvousMachine:GatedResidentMachine {
 std::vector<std::pair<unsigned,uint32_t>> cycles;
 uint64_t cycleEpoch=UINT64_C(1)<<40,sampleTime=0,sampleGap=1000000;
 unsigned clockFault=0;bool rollover=false;
 RendezvousMachine(const char* image,const PioneerQ38CanonicalModelPlan& plan,unsigned layer,unsigned half,unsigned bank,unsigned trial=1)
  :GatedResidentMachine(image,plan,layer,half,bank,trial){
  gatedDescriptor[0]=TT_Q38_GDN_RENDEZVOUS_MARKER;gatedDescriptor[51]=TT_Q38_GDN_RENDEZVOUS_POLLS;
  gatedDescriptor[52]=uint32_t(PioneerQ38GdnRendezvousBudget());gatedDescriptor[53]=uint32_t(PioneerQ38GdnRendezvousBudget()>>32);
  gatedDescriptor[54]=4;gatedDescriptor[55]=TT_Q38_GDN_RENDEZVOUS_CLOCK;
  assert(PioneerQ38GdnRendezvousShape(gatedDescriptor.data(),inputDescriptor.data(),descriptor.data()));SetGatedDescriptor();
  for(unsigned i=0;i<16;i++)memory[TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4]=0xdeadbeef;
 }
 uint32_t StageMarker()const override{return TT_Q38_GDN_RENDEZVOUS_MARKER;}
 uint32_t StageFault()const override{return TT_Q38_GDN_RENDEZVOUS_FAULT;}
 unsigned MaxReadyPolls()const override{return TT_Q38_GDN_RENDEZVOUS_POLLS;}
 uint32_t ReadyWord(unsigned i)const override{return PioneerQ38GdnRendezvousReadyWord(gatedDescriptor.data(),i,false);}
 bool CheckSummary(const uint32_t* s,const uint32_t* before,const uint32_t* after)const override{
  return PioneerQ38GdnRendezvousSummary(s,gatedDescriptor.data(),inputDescriptor.data(),descriptor.data(),before,after);
 }
 uint32_t ReadCycleCsr(unsigned csr)override{
  assert(csr==0xb00 || csr==0xb80);
  unsigned at=unsigned(cycles.size());
  if(at%3==0)sampleTime=cycleEpoch+uint64_t(steps)*19+uint64_t(at/3)*sampleGap;
  uint64_t now=sampleTime;
  if(clockFault==2 && at>=3)now-=UINT64_C(1)<<39;
  if(clockFault==3)now=cycleEpoch;
  if(clockFault==4 && phase==4)now-=UINT64_C(1)<<39;
  uint32_t value=csr==0xb00?uint32_t(now):uint32_t(now>>32);
  if(rollover && !at)value--;
  if(clockFault==1 && csr==0xb80)value+=at;
  cycles.emplace_back(csr,value);return value;
 }
 uint32_t Load(uint32_t at)override{
  if(Range(at,TT_Q38_GDN_RENDEZVOUS_CLOCK,64))return memory.at(at);
  return GatedResidentMachine::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  if(Range(at,TT_Q38_GDN_RENDEZVOUS_CLOCK,64)){
   assert(!gatedPackets || phase==5);memory[at]=value;return;
  }
  GatedResidentMachine::Store(at,value);
 }
 void VerifyClock(){
  uint32_t receipt[16];for(unsigned i=0;i<16;i++)receipt[i]=memory.at(TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4);
  assert(PioneerQ38GdnRendezvousClockReceipt(receipt,gatedDescriptor.data(),readyPolls));
  std::vector<uint64_t> samples;
  assert(cycles.size()%3==0);
  for(unsigned i=0;i<cycles.size();i+=3){
   assert(cycles[i].first==0xb80 && cycles[i+1].first==0xb00 && cycles[i+2].first==0xb80);
   if(cycles[i].second==cycles[i+2].second)samples.push_back(uint64_t(cycles[i].second)<<32|cycles[i+1].second);
  }
  assert(samples.size()==readyPolls+2 && (uint64_t(receipt[2])<<32|receipt[1])==samples.front()
   && (uint64_t(receipt[4])<<32|receipt[3])==samples.back());
  for(unsigned i=1;i<samples.size();i++)assert(PioneerQ38GdnRendezvousClockValue(samples.front(),samples[i-1],samples[i]));
  // Begin/end are DATA constrained by ordering/budget,not a checksum.
  // Exact raw CSR samples above independently verify those four words.
  for(unsigned i=0;i<16;i++)if(!i || i>=5){receipt[i]^=1;assert(!PioneerQ38GdnRendezvousClockReceipt(receipt,gatedDescriptor.data(),readyPolls));receipt[i]^=1;}
  uint32_t old[4];for(unsigned i=0;i<4;i++)old[i]=receipt[1+i];
  receipt[3]=receipt[1];receipt[4]=receipt[2];assert(!PioneerQ38GdnRendezvousClockReceipt(receipt,gatedDescriptor.data(),readyPolls));
  uint64_t expired=samples.front()+PioneerQ38GdnRendezvousBudget();receipt[3]=uint32_t(expired);receipt[4]=uint32_t(expired>>32);
  assert(!PioneerQ38GdnRendezvousClockReceipt(receipt,gatedDescriptor.data(),readyPolls));
  for(unsigned i=0;i<4;i++)receipt[1+i]=old[i];
 }
};
#ifndef PIONEER_Q38_GDN_RENDEZVOUS_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&TestWorker;
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 unsigned positive=0,negative=0,maxSteps=0,maxStack=0;
 for(unsigned head=0;head<48;head++){
  unsigned layer=head/3*4+head%3;
  RendezvousMachine a(argv[1],*plan,layer,head*2,head%7,head%4+1),b(argv[1],*plan,layer,head*2+1,head%7,head%4+1);
  RunGatedPair(a,b,head%2,32);a.VerifyGated();b.VerifyGated();a.VerifyClock();b.VerifyClock();positive+=2;
  for(auto* m:{&a,&b}){maxSteps=std::max(maxSteps,m->steps);maxStack=std::max(maxStack,0x70000-m->minStack);}
 }
 {GatedResidentMachine a(argv[2],*plan,0,0,0),b(argv[2],*plan,0,1,0);a.peer=&b;b.peer=&a;
  while(!a.Terminal())a.StepCombined();assert(!a.Passed() && a.readyPolls==256 && !a.gatedWrites && !b.steps);
  puts("PASS ACTUAL old QGH1 NEGATIVE:worker2 not yet released,worker1 exhausts256completed polls and faults BEFORE any gated write");}
 for(unsigned delayed:{512U,2048U}){
  RendezvousMachine a(argv[1],*plan,0,0,0),b(argv[1],*plan,0,1,0);a.peer=&b;b.peer=&a;
  while(a.readyPolls<delayed && !a.Terminal())a.StepCombined();assert(!a.Terminal() && !b.steps && a.readyPolls==delayed);
  RunGatedPair(a,b);a.VerifyGated();b.VerifyGated();a.VerifyClock();b.VerifyClock();positive+=2;
  printf("PASS ACTUAL QGT1 late peer release after%ureads (>old256);ALL state/raw/gated values and captured clock receipts exact;NO host arithmetic\n",delayed);
 }
 {RendezvousMachine a(argv[1],*plan,0,0,0),b(argv[1],*plan,0,1,0);a.rollover=true;
  RunGatedPair(a,b);a.VerifyGated();b.VerifyGated();a.VerifyClock();b.VerifyClock();positive+=2;}
 for(unsigned kind=1;kind<=4;kind++){
  RendezvousMachine a(argv[1],*plan,0,0,0),b(argv[1],*plan,0,1,0);a.clockFault=kind;b.sampleGap=4000000000ULL;
  RunGatedPair(a,b);assert(!a.Passed() && !a.memory.at(TT_Q38_GDN_RENDEZVOUS_CLOCK));
  if(kind==1)assert(!a.packets && !a.unit.ops && a.cycles.size()==12);
  if(kind==4)assert(a.gatedWrites==64 && a.phase==4);else assert(!a.gatedWrites);
  negative++;
 }
 {RendezvousMachine a(argv[1],*plan,0,0,0),b(argv[1],*plan,0,1,0);a.neverReady=true;a.sampleGap=4000000000ULL;
  RunGatedPair(a,b);assert(!a.Passed() && a.readyPolls<256 && !a.gatedWrites && !a.memory.at(TT_Q38_GDN_RENDEZVOUS_CLOCK));negative++;}
 for(unsigned word=0;word<64;word++)for(unsigned bit=0;bit<32;bit++){
  RendezvousMachine a(argv[1],*plan,0,0,0);a.gatedDescriptor[word]^=1U<<bit;
  if(PioneerQ38GdnRendezvousShape(a.gatedDescriptor.data(),a.inputDescriptor.data(),a.descriptor.data()))continue;
  a.SetGatedDescriptor();while(!a.Terminal())a.StepCombined();assert(!a.Passed() && !a.packets && !a.nocReads && a.cycles.empty() && !a.unit.ops);negative++;
 }
 printf("PASS ACTUAL QGT1 positives%u/negatives%u/all96half metadata/late launch/CSR rollovers/unstable-backward-stalled-final clock/deadline/ALL malformed descriptor bits BEFORE MMIO orCSR;maxSteps%u/maxStack%u;FABRICATED CLOCK_NOT_NATIVE_TIME_OR_SPEED\n",positive,negative,maxSteps,maxStack);
}
#endif
