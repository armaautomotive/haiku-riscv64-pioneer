/* SPDX-License-Identifier: MIT */
// ACTUAL member/Fail source, explicitly MOCK lifecycle/source/HOLD/clock
// authority. This is not a real native owner, model or card lifetime.
#include "../PioneerQ38SelectedPhaseProfileData.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "actual_phase_result.inc"
namespace Q=PioneerQ38SelectedPhaseProfileData;
#ifdef PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE
#include "pioneer_q38_fast_decode_profile_metric_fixture.h"
#endif
namespace PioneerQ38TokenPrograms {enum {TokenPhaseProfile=1,PhaseProfileAddress=0x75710,PhaseProfileWords=60};}
struct PioneerResidentBanks {enum {Pages=16};};
struct PioneerQ38TokenReadResult {unsigned word=0;};
struct ReadOwner;
struct ReadIO {
 ReadOwner* owner=nullptr;unsigned touches=0,failedAt=0,failAt=0,reads=0,mismatch=0;
 unsigned bootReads=0,frontiers=0,guards=0,retains=0;uint64_t clock=100;
 std::array<uint32_t,Q::Words> raw={};
 bool Touch();uint64_t Q38CohortNow();
 bool TokenMemoryReadChecked(unsigned,uint32_t,uint32_t&);
};
struct SmallOwned {unsigned data=0;};
struct Startup:SmallOwned {
 ReadIO* io=nullptr;bool allowOutput=true;
 bool PublishedOutput(const void*,size_t bytes)const{return allowOutput && bytes==256;}
 bool ReadBoot(unsigned offset,uint32_t* out){assert(!offset);io->bootReads++;if(!io->Touch())return false;
  for(unsigned i=0;i<64;i++)out[i]=0x13570000+i;return true;}
};
struct ReadOwner {
 SmallOwned memoryObject,cohortObject,banksObject,proofObject,canonicalObject;
 SmallOwned* memory=&memoryObject;SmallOwned* cohort=&cohortObject;SmallOwned* banks=&banksObject;
 SmallOwned* proof=&proofObject;SmallOwned* canonical=&canonicalObject;
 Startup startupObject;Startup* startup=&startupObject;
 std::array<uint8_t,PioneerResidentBanks::Pages> ledgerObject={};const uint8_t* ledger=ledgerObject.data();
 bool phaseProfileRead=false,restoring=false,l1Restored=false,readoutPending=false,unsafe=false;
 bool drained=true,held=true,scope=true,allowDispatch=true;
 uint32_t observedTerminal[64]={};
 bool CommittedRecordsRead()const{return drained && held && !unsafe;}
 bool Scope(ReadIO&)const{return scope && !unsafe;}
 bool DispatchOutput(const void*,size_t bytes)const{return allowDispatch && bytes==256;}
 bool FailOwner(ReadIO& io){assert(!unsafe && !io.failedAt);unsafe=true;io.retains++;io.failedAt=io.touches;return false;}
 bool BootTime(ReadIO& io,uint64_t began,uint64_t& previous){
  if(!io.Touch())return false;const auto now=++io.clock;bool okay=now>=previous && now>=began && now-began<30000000;
  previous=now;return okay;
 }
 bool HoldGuard(ReadIO& io,unsigned worker,PioneerQ38TokenPhaseProfileResult& r){
  assert(worker<120 && held);(void)r;io.guards++;return io.Touch();
 }
 bool ReadFrontier(ReadIO& io,const uint32_t* boot,PioneerQ38TokenReadResult&){
  io.frontiers++;if(!io.Touch())return false;for(unsigned i=0;i<64;i++)assert(boot[i]==0x13570000+i);return true;
 }
#include "actual_phase_fail.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenPhaseProfileRead.inc"
};
bool ReadIO::Touch(){assert(owner && !owner->unsafe && !failedAt);touches++;return touches!=failAt;}
uint64_t ReadIO::Q38CohortNow(){return Touch()?++clock:UINT64_MAX;}
bool ReadIO::TokenMemoryReadChecked(unsigned worker,uint32_t address,uint32_t& out){
 assert(!worker && address>=Q::Address && address<Q::Address+Q::Words*4 && !(address&3));
 reads++;if(!Touch())return false;out=raw[(address-Q::Address)/4];if(reads==mismatch)out^=1;return true;
} // No writes,NoC,reset,DMA,unreserve,END or CPU products exist in this IO.
static std::array<uint32_t,Q::Words> Metric(){
#ifdef PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE
 return FastDecodeMetricFixture();
#else
 std::array<uint32_t,Q::Words> p={};uint64_t now=UINT64_C(1)<<40;
 Q::Initialize(p.data(),true,now);
 auto mark=[&](unsigned next){now+=19;Q::Advance(p.data(),true,now,next);};
 for(unsigned token=0;token<34;token++){
  for(unsigned layer=0;layer<64;layer++){
   mark(Q::Metadata);mark(layer%4==3?Q::Dense:Q::Recurrent);
#ifdef PIONEER_Q38_BF16_FULL_PROFILE
   for(unsigned job=0;job<(layer%4==3?7U:8U);job++)
    for(unsigned phase:{Q::MatrixPrepare,Q::MatrixDispatch,Q::MatrixGatherWait,Q::MatrixFinish,
      layer%4==3?Q::Dense:Q::Recurrent})mark(phase);
#endif
  }
  for(unsigned next:{Q::HeadNormalize,Q::HeadMatrix,Q::Feedback,Q::StateCommit,Q::Publication})mark(next);
 }
 mark(Q::FinalStop);Q::Advance(p.data(),true,now+19,Q::Phases,34);assert(Q::Closed(p.data()));return p;
#endif
}
struct Fixture {
 ReadOwner owner;ReadIO io;std::array<uint32_t,64> output={};
 Fixture(){io.owner=&owner;owner.startup->io=&io;io.raw=Metric();owner.observedTerminal[3]=34;}
 void Success(bool valid){
  const auto terminal=owner.observedTerminal[3];PioneerQ38TokenPhaseProfileResult r;
  assert(owner.ReadPhaseProfile(io,output.data(),r) && r.started && r.ready && !r.unsafe && r.valid==unsigned(valid)
   && r.phase==4 && r.worker==0 && r.word==59 && r.words==60 && r.committed==terminal
   && r.metricState==io.raw[Q::State] && r.metricError==io.raw[Q::Error] && !r.firstPC && !r.secondPC);
  for(unsigned i=0;i<60;i++)assert(output[i]==io.raw[i]);for(unsigned i=60;i<64;i++)assert(!output[i]);
  assert(io.reads==120 && io.guards==360 && io.frontiers==2 && io.bootReads==1 && !io.retains && owner.phaseProfileRead
   && owner.held && owner.drained && !owner.restoring && !owner.l1Restored && !owner.unsafe && owner.observedTerminal[3]==terminal);
  const auto before=r;const unsigned touches=io.touches;std::array<uint32_t,64> again={};PioneerQ38TokenPhaseProfileResult next;
  assert(!owner.ReadPhaseProfile(io,again.data(),next) && io.touches==touches && !memcmp(&r,&before,sizeof(r)));
 }
 void Failure(){
  PioneerQ38TokenPhaseProfileResult r;
  assert(!owner.ReadPhaseProfile(io,output.data(),r) && r.started && r.unsafe && !r.ready && !r.valid
   && !r.words && !r.committed && !r.metricState && !r.metricError && owner.unsafe && owner.phaseProfileRead
   && io.retains==1 && io.failedAt==io.touches && owner.held && owner.drained && owner.observedTerminal[3]==34);
  for(auto word:output)assert(!word);
  const unsigned touches=io.touches;PioneerQ38TokenPhaseProfileResult again;
  assert(!owner.ReadPhaseProfile(io,output.data(),again) && touches==io.touches && io.retains==1);
 }
};
int main(){
 unsigned good=0,refused=0,failures=0,events=0;
 {Fixture f;f.Success(true);events=f.io.touches;good++;}
 for(unsigned word=0;word<60;word++){Fixture f;f.io.raw[word]^=1;f.Success(false);good++;}
 {Fixture f;f.owner.observedTerminal[3]=33;f.Success(false);good++;}
 {Fixture f;f.io.raw[Q::State]=Q::Invalid;f.io.raw[Q::Error]=Q::ClockUnstable;f.Success(false);good++;}
 for(unsigned word=0;word<16;word++){
  Fixture f;PioneerQ38TokenPhaseProfileResult r;reinterpret_cast<uint32_t*>(&r)[word]^=1;const auto before=r;
  assert(!f.owner.ReadPhaseProfile(f.io,f.output.data(),r) && !f.io.touches && !f.owner.phaseProfileRead && !f.owner.unsafe
   && !memcmp(&r,&before,sizeof(r)));refused++;
 }
 for(unsigned word=0;word<64;word++){
  Fixture f;f.output[word]=1;const auto before=f.output;PioneerQ38TokenPhaseProfileResult r;
  assert(!f.owner.ReadPhaseProfile(f.io,f.output.data(),r) && !f.io.touches && !f.owner.phaseProfileRead && f.output==before);refused++;
 }
 for(unsigned condition=0;condition<11;condition++){
  Fixture f;PioneerQ38TokenPhaseProfileResult r;
  switch(condition){
   case 0:f.owner.phaseProfileRead=true;break;case 1:f.owner.restoring=true;break;case 2:f.owner.l1Restored=true;break;
   case 3:f.owner.readoutPending=true;break;case 4:f.owner.drained=false;break;case 5:f.owner.held=false;break;
   case 6:f.owner.scope=false;break;case 7:f.owner.unsafe=true;break;case 8:f.owner.allowDispatch=false;break;
   case 9:f.owner.startup->allowOutput=false;break;case 10:break;default:assert(false);
  }
  assert(!f.owner.ReadPhaseProfile(f.io,condition==10?nullptr:f.output.data(),r) && !f.io.touches && !f.io.retains);refused++;
 }
 for(unsigned alias=0;alias<10;alias++){
  Fixture f;PioneerQ38TokenPhaseProfileResult r;
  void* pointers[]={&f.owner,f.owner.memory,f.owner.cohort,f.owner.banks,f.owner.proof,f.owner.canonical,
   f.owner.startup,f.owner.ledgerObject.data(),&r,reinterpret_cast<uint8_t*>(f.output.data())+1};
  assert(!f.owner.ReadPhaseProfile(f.io,reinterpret_cast<uint32_t*>(pointers[alias]),r) && !f.io.touches && !f.io.retains);refused++;
 }
 for(unsigned event=1;event<=events;event++){Fixture f;f.io.failAt=event;f.Failure();failures++;}
 for(unsigned word=0;word<60;word++){Fixture f;f.io.mismatch=61+word;f.Failure();failures++;}
 printf("PASS actual held-phase member+Fail:%u stable metric+invalid DATA successes/%u pristine,phase,alias refusals/%u EVERY IO-event+all60 second-pass-change faults/atomic60word publication+4zero padding/one read only/invalid metric doesNOT quarantine/unsafe retainsONCE andNO following IO;MOCK lifecycle,source,HOLD,clock authority,NOT real native/model/physical timing/Work proof\n",good,refused,failures);
}
