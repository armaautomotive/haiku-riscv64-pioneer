/* SPDX-License-Identifier: MIT */
// Actual production methods;ALL authority/source/terminal/NoC endpoints MOCK.
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include "actual_hidden_records.inc"
namespace PioneerQ38GlobalChainLayout {
enum {WorkspaceBytes=0x400000};
namespace R {enum {Boot=0x10000,Scratch=0x30000,FinalOffset=0x200000};}
}
namespace PioneerQ38TokenLayout {enum {TerminalLocal=0x20000};}
struct PioneerResidentBanks {enum {Pages=64};uint32_t data[16]={};};
namespace PioneerQ38TokenRope {enum {Bytes=64};static const uint32_t Data[16]={};}
static uint32_t PioneerQ38ProjectionRoute(unsigned bank){assert(bank<7);return 0x80+bank;}
static unsigned PioneerQ38PanelCounter(unsigned i){assert(i<8);return 0x100+(i==6?2:i)*4;}
struct MockOwner;
struct MockIO {
 MockOwner* owner=nullptr;unsigned selected=0,calls=0,rawReads=0,failRead=0,packets=0,writes=0,pcReads=0;
 unsigned fault=0,faultWord=0,faultBegin=1,lastWords=0;bool dead=false,postRead=false;
 uint32_t command[10]={},scratch[4096]={},hidden[5120]={};uint64_t now=1000;
 void Touch();
 uint32_t ResidentNocRead(unsigned);
 void ResidentNocWrite(unsigned,uint32_t);
 bool TokenMemoryReadChecked(unsigned,uint32_t,uint32_t&);
 uint32_t RuntimePcRead(){Touch();pcReads++;return 0;}
 bool ActiveFault()const{return packets>=faultBegin;}
 uint64_t Q38CohortNow(){Touch();if(ActiveFault() && fault==6)return 0;if(ActiveFault() && fault==7)return now+UINT64_C(30000000);return now++;}
 void Pause(){Touch();}
};
static bool PioneerQ38TokenFifoEmpty(MockIO&,unsigned,uint32_t);
static bool PioneerQ38GraphCsrIdle(MockIO&);
struct MockOwner {
 struct Small {uint32_t data[16]={};} memoryData,canonicalData,proofData;
 Small* memory=&memoryData;Small* canonical=&canonicalData;const Small* proof=&proofData;
 PioneerResidentBanks bankData;PioneerResidentBanks* banks=&bankData;
 uint8_t ledgerData[64]={};const uint8_t* ledger=ledgerData;
 struct Cohort {struct Authority {uint32_t source[120][16]={};} authority;} ownedCohort;
 Cohort* cohort=&ownedCohort;
 struct Core {uint32_t counter[8]={};bool source=true,fifo=true,held=true;} cores[120];
 bool unsafe=false,held=true,terminalObserved=true,drained=true,restoring=false,l1Restored=false,scope=true,dispatchOutput=true;
 bool running[120]={},readoutPending=false,readoutCommandKnown=false;
 uint32_t readoutCommand[10]={},terminalCounters[120][8]={},observedTerminal[64]={},terminal[64]={},boot[64]={},expectedBoot[64]={};
 uint32_t staging[4096]={};unsigned hiddenReadCursor=0,failures=0,readoutCursor=0;
 PioneerQ38TokenHiddenReadResult ownResult;
 struct Plan {unsigned bank=0,workspace=0x100000;unsigned Bank()const{return bank;}unsigned Workspace()const{return workspace;}} plan;
 struct Startup {
  MockOwner* owner;bool output=true,read=true;
  bool PublishedOutput(const void*,size_t bytes)const{return output && bytes && bytes<=16384 && !(bytes&63);}
  bool ReadBoot(unsigned worker,uint32_t* out){assert(!worker);if(!read)return false;memcpy(out,owner->expectedBoot,256);return true;}
 } ownedStartup{this};Startup* startup=&ownedStartup;
 MockOwner(){
  observedTerminal[0]=0x1234;observedTerminal[2]=3;observedTerminal[3]=0;observedTerminal[4]=0;observedTerminal[6]=1;
  memcpy(terminal,observedTerminal,256);
  for(unsigned i=0;i<64;i++)expectedBoot[i]=boot[i]=0x1000+i;
  for(unsigned w=0;w<120;w++)for(unsigned i=0;i<8;i++)cores[w].counter[i]=terminalCounters[w][i]=100+w*8+(i==6?2:i);
 }
 bool Scope(MockIO&)const{return scope && !unsafe;}
 bool HeldAfterTerminal()const{return held && !unsafe;}
 bool CommittedRecordsRead()const{return drained && !readoutPending && HeldAfterTerminal();}
 bool DispatchOutput(const void*,size_t)const{return dispatchOutput;}
 bool FailOwner(MockIO&){unsafe=true;failures++;return false;}
 bool Sources(MockIO& io,unsigned w){
  assert(w<120);if(unsafe)return false;io.Touch();io.selected=w;
  if(!cores[w].source || !cores[w].held || (io.ActiveFault() && io.fault==4)){io.dead=true;return false;}return true;
 }
 template<class IO> bool RootStopSeal(IO&,const uint32_t*,uint32_t&){assert(false);return false;}
#include "actual_hidden_methods.inc"
};
void MockIO::Touch(){assert(owner && !owner->unsafe && !dead);calls++;}
static bool PioneerQ38TokenFifoEmpty(MockIO& io,unsigned w,uint32_t){io.Touch();assert(io.selected==w);return io.owner->cores[w].fifo;}
static bool PioneerQ38GraphCsrIdle(MockIO& io){io.Touch();return !(io.ActiveFault() && io.fault==5);}
uint32_t MockIO::ResidentNocRead(unsigned address){
 Touch();
 if(address==0x40)return 0;
 if(address<40 && !(address&3)){
  const unsigned at=address/4;
  if((!packets && fault==1 && at==faultWord) || (ActiveFault() && fault==2 && at==faultWord))return command[at]^1;
  return command[at];
 }
 assert(address>=0x100 && address<=0x11c && !(address&3));
 const unsigned at=(address-0x100)/4;
 uint32_t value=owner->cores[selected].counter[at];
 if(ActiveFault() && fault==3 && at==(faultWord==6?2:faultWord))value+=2;
 return value;
}
void MockIO::ResidentNocWrite(unsigned address,uint32_t value){
 Touch();writes++;
 if(address<40 && !(address&3)){command[address/4]=value;return;}
 assert(address==0x40 && value==1 && !selected);packets++;
 const uint64_t base=uint64_t(owner->plan.Workspace())+PioneerQ38GlobalChainLayout::R::FinalOffset;
 const unsigned bytes=owner->hiddenReadCursor?4096:16384;
 assert(command[0]==base+owner->hiddenReadCursor*16384 && !command[1]
  && command[2]==PioneerQ38ProjectionRoute(owner->plan.Bank()) && command[3]==PioneerQ38GlobalChainLayout::R::Scratch
  && !command[4] && command[5]==129 && !command[6] && !command[7] && command[8]==bytes && !command[9]);
 lastWords=bytes/4;memcpy(scratch,hidden+owner->hiddenReadCursor*4096,bytes);
 if(!(ActiveFault() && fault==8))for(unsigned i=0;i<8;i++)owner->cores[0].counter[i]+=i==1?bytes/64:i==0 || i==2 || i==3 || i==6?1U:0U;
}
bool MockIO::TokenMemoryReadChecked(unsigned worker,uint32_t address,uint32_t& value){
 Touch();assert(!worker && !selected);rawReads++;
 if(rawReads==failRead){dead=true;return false;}
 namespace R=PioneerQ38GlobalChainLayout::R;
 if(address>=R::Boot && address<R::Boot+256){value=owner->boot[(address-R::Boot)/4];return true;}
 if(address>=PioneerQ38TokenLayout::TerminalLocal && address<PioneerQ38TokenLayout::TerminalLocal+256){
  value=owner->terminal[(address-PioneerQ38TokenLayout::TerminalLocal)/4];return true;}
 assert(address>=R::Scratch && address<R::Scratch+lastWords*4);
 const unsigned at=(address-R::Scratch)/4;value=scratch[at];
 if(at+1==lastWords){
  postRead=true;
  if(ActiveFault() && fault==9)owner->terminal[faultWord]^=1;
  if(ActiveFault() && fault==10)owner->boot[faultWord]^=1;
  if(ActiveFault() && fault==11)owner->cores[faultWord].counter[0]++;
 }
 return true;
}
struct Fixture {
 std::unique_ptr<MockOwner> owner=std::make_unique<MockOwner>();MockIO io;
 std::array<uint32_t,4096> output={};
 Fixture(){io.owner=owner.get();for(unsigned i=0;i<5120;i++)io.hidden[i]=0x3f000000+i;io.hidden[17]=0x7fc12345;}
 bool Read(unsigned part,PioneerQ38TokenHiddenReadResult& r){return owner->ReadHiddenChunk(io,part,output.data(),part?1024:4096,r);}
};
static void Early(Fixture& f,unsigned part,unsigned words,uint32_t* out,PioneerQ38TokenHiddenReadResult& r){
 const auto before=f.output;assert(!f.owner->ReadHiddenChunk(f.io,part,out,words,r));
 assert(!f.io.calls && !f.io.writes && !f.owner->unsafe && !f.owner->failures && f.output==before);
}
static void Retained(Fixture& f){
 assert(f.owner->unsafe && f.owner->failures==1);
 for(auto word:f.output)assert(!word);
 const unsigned calls=f.io.calls;PioneerQ38TokenHiddenReadResult retry;
 assert(!f.Read(0,retry) && calls==f.io.calls); // refusal,NOT another packet/retry/cleanup
}
int main(){
 unsigned positives=0,early=0,faults=0;
 for(unsigned mode=0;mode<4;mode++){
  Fixture f;
  if(mode&1)for(unsigned w=0;w<120;w++)for(unsigned i=0;i<8;i++)f.owner->cores[w].counter[i]=f.owner->terminalCounters[w][i]=0xfffffff0U;
  const bool full=mode&2;
  if(full){f.owner->observedTerminal[2]=1;f.owner->observedTerminal[3]=3;f.owner->observedTerminal[4]=2;
   f.owner->observedTerminal[6]=64;f.owner->readoutCursor=3;memcpy(f.owner->terminal,f.owner->observedTerminal,256);}
  const unsigned cursor=f.owner->readoutCursor;
  for(unsigned part=0;part<2;part++){
   f.output.fill(0);PioneerQ38TokenHiddenReadResult r;assert(f.Read(part,r));
   assert(r.ready && !r.unsafe && r.phase==4 && r.part==part && r.firstWord==part*4096
    && r.wordCount==(part?1024U:4096U) && r.completedLayers==(full?64U:1U)
    && r.position==(full?2U:0U) && r.terminalKind==(full?1U:3U) && r.committed==(full?3U:0U)
    && r.packets==1 && r.bytesRead==r.wordCount*4 && f.owner->hiddenReadCursor==part+1
    && f.owner->readoutCursor==cursor && f.owner->drained && !f.owner->readoutPending && !f.io.pcReads);
   for(unsigned i=0;i<r.wordCount;i++)assert(f.output[i]==f.io.hidden[part*4096+i]);
   for(unsigned i=r.wordCount;i<4096;i++)assert(!f.output[i]);
   for(unsigned i=0;i<8;i++)assert(f.owner->terminalCounters[0][i]==f.owner->cores[0].counter[i]);
   positives++;
  }
  const unsigned calls=f.io.calls;f.output.fill(0);PioneerQ38TokenHiddenReadResult r;
  assert(!f.Read(0,r) && !f.owner->unsafe && f.io.calls==calls);early++;
 }
 for(unsigned kind=0;kind<20;kind++){
  Fixture f;PioneerQ38TokenHiddenReadResult r;unsigned part=0,words=4096;uint32_t* out=f.output.data();
  if(kind==0)part=1;if(kind==1)part=2;if(kind==2)words=0;if(kind==3)words=4095;
  if(kind==4)out=nullptr;if(kind==5)out=reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(out)+1);
  if(kind==6)out=reinterpret_cast<uint32_t*>(UINTPTR_MAX&~uintptr_t(3));
  if(kind==7)out=f.owner->staging;if(kind==8)f.output[0]=1;
  if(kind==9)f.owner->scope=false;if(kind==10)f.owner->held=false;if(kind==11)f.owner->drained=false;
  if(kind==12)f.owner->restoring=true;if(kind==13)f.owner->l1Restored=true;if(kind==14)f.owner->readoutPending=true;
  if(kind==15)f.owner->observedTerminal[6]=0;if(kind==16)f.owner->observedTerminal[6]=65;
  if(kind==17)f.owner->ownedStartup.output=false;if(kind==18)f.owner->dispatchOutput=false;
  if(kind==19)out=f.owner->memoryData.data;
  Early(f,part,words,out,r);early++;
 }
 for(unsigned word=0;word<16;word++){
  Fixture f;PioneerQ38TokenHiddenReadResult r;reinterpret_cast<uint32_t*>(&r)[word]^=1;
  Early(f,0,4096,f.output.data(),r);early++;
 }
 {Fixture f;Early(f,0,4096,f.output.data(),f.owner->ownResult);early++;}
 {Fixture f;auto* r=new(f.output.data()) PioneerQ38TokenHiddenReadResult;
  Early(f,0,4096,f.output.data(),*r);early++;}
 for(unsigned worker=0;worker<120;worker++)for(unsigned kind=0;kind<3;kind++){
  Fixture f;if(kind==0)f.owner->cores[worker].source=false;
  if(kind==1)f.owner->cores[worker].fifo=false;if(kind==2)f.owner->cores[worker].counter[0]++;
  PioneerQ38TokenHiddenReadResult r;assert(!f.Read(0,r));Retained(f);assert(!f.io.packets);faults++;
 }
 for(unsigned word=0;word<64;word++)for(unsigned kind=0;kind<2;kind++){
  Fixture f;if(kind)f.owner->boot[word]^=1;else f.owner->terminal[word]^=1;
  PioneerQ38TokenHiddenReadResult r;assert(!f.Read(0,r));Retained(f);assert(!f.io.packets);faults++;
 }
 for(unsigned kind=1;kind<=8;kind++){
  const unsigned cases=kind==1 || kind==2?10:kind==3?8:1;
  for(unsigned word=0;word<cases;word++){
   Fixture f;f.io.fault=kind;f.io.faultWord=word;
   PioneerQ38TokenHiddenReadResult r;assert(!f.Read(0,r));Retained(f);faults++;
  }
 }
 for(unsigned kind=9;kind<=11;kind++)for(unsigned word=0;word<(kind==11?120U:64U);word++){
  Fixture f;f.io.fault=kind;f.io.faultWord=word;
  PioneerQ38TokenHiddenReadResult r;assert(!f.Read(0,r));Retained(f);assert(f.io.packets==1 && f.io.postRead);faults++;
 }
 for(unsigned failure:{1U,2U,63U,128U,129U,130U,222U,4096U,4224U,4352U}){
  Fixture f;f.io.failRead=failure;PioneerQ38TokenHiddenReadResult r;
  assert(!f.Read(0,r) && f.io.dead && f.io.rawReads==failure);Retained(f);faults++;
 }
 // The second call must retain the first command/counter epoch. A failure
 // cannot publish a partial tail,change already returned DATA,or restart.
 for(unsigned kind=2;kind<=11;kind++){
  const unsigned cases=kind==2?10:kind==3?8:kind==9 || kind==10?64:kind==11?120:1;
  for(unsigned word=0;word<cases;word++){
   Fixture f;PioneerQ38TokenHiddenReadResult first;assert(f.Read(0,first));
   const auto published=f.output;f.output.fill(0);f.io.faultBegin=2;f.io.fault=kind;f.io.faultWord=word;
   PioneerQ38TokenHiddenReadResult r;assert(!f.Read(1,r));Retained(f);
   assert(f.owner->hiddenReadCursor==1 && r.packets==1);
   for(unsigned i=0;i<4096;i++)assert(published[i]==f.io.hidden[i]);faults++;
  }
 }
 for(unsigned at:{1U,64U,128U,129U,1000U,1152U,1280U}){
  Fixture f;PioneerQ38TokenHiddenReadResult first;assert(f.Read(0,first));f.output.fill(0);
  f.io.failRead=f.io.rawReads+at;PioneerQ38TokenHiddenReadResult r;
  assert(!f.Read(1,r) && f.io.dead && f.io.rawReads==f.io.failRead);Retained(f);
  assert(f.owner->hiddenReadCursor==1);faults++;
 }
 printf("PASS extracted hidden READ methods/%u MOCK parts including raw NaN+counter rollover/%u early refusals/%u retained faults/ALL120 source+FIFO+counter guards/128 post-copy metadata+120 counter faults/no partial publication/no following IO;NO native/model/physical/numerical/Work/TPS proof\n",positives,early,faults);
}
