/* SPDX-License-Identifier: MIT */
// Actual production methods; ALL other grammar,authority,hardware and
// VerifyImage endpoints deliberately MOCK. Not a native STOP/cleanup proof.
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include "actual_root_records.inc"
namespace PioneerQ38GlobalChainLayout {
enum {Progress=0x50000};
namespace R {enum {Status=0x100,Boot=0x1000,PeerBoots=0x10000,PeerJobs=0x20000,Nonces=0x30000,Counters=0x40000};}
}
static bool mockSession=true,mockTerminalShape=true,mockJobShape=true;
namespace PioneerQ38TokenLayout {
enum {Magic=0x1111,TerminalMagic=0x2222,CancelMagic=0x3333,TerminalLocal=0x51000,CancelLocal=0x52000};
static bool Session(const uint32_t*,const uint32_t*,unsigned,unsigned){return mockSession;}
}
namespace PioneerQ38SelectedGlobalWorker {
enum {Boot=0x1000,Job=0x2000,Status=0x3000,Doorbell=0x4000,Magic=0x4444,Stop=9};
static unsigned Peer(unsigned w){return w+129;}
static bool JobShape(const uint32_t*,const uint32_t*,unsigned,unsigned){return mockJobShape;}
}
namespace PioneerQ38TokenPrograms {
static bool ProgramWord(unsigned w,uint32_t pc,uint32_t& value){
 assert(!w);value=0x6f;return pc==0x60000 || pc==0x60004 || pc==0x40010;
} // frozen OLD method only;does NOT invent a word at0x60008
}
static bool PioneerQ38TokenTerminalShape(const uint32_t*,const uint32_t*,const uint32_t*,unsigned,unsigned,bool,bool){return mockTerminalShape;}
static bool PioneerQ38TokenTerminalRecheckData(const uint32_t* a,const uint32_t* b,uint32_t& word){
 for(unsigned i=0;i<64;i++){word=i;if(a[i]!=b[i])return false;}return true;
}
static unsigned PioneerQ38PanelCounter(unsigned i){assert(i<8);return i;}
struct MethodMock;
struct MockIO {
 MethodMock* owner=nullptr;unsigned selected=0,calls=0,reads=0,failRead=0,pcReads=0;
 unsigned peerSources=0,imageChecks=0,mutateAfterImage=0;
 bool failed=false,source=true,idle=true,fifo=true,image=true;
 std::array<uint32_t,4> rootPC={0x60004,0x60004,0x60008,0x60008};
 uint32_t peerPC=0x20060;
 void Touch();
 bool TokenMemoryReadChecked(unsigned,uint32_t,uint32_t&);
 uint32_t RuntimePcRead();
 uint32_t ResidentNocRead(unsigned);
};
static bool PioneerQ38TokenFifoEmpty(MockIO& io,unsigned w,uint32_t){io.Touch();assert(io.selected==w);return io.fifo;}
static bool PioneerQ38GraphCsrIdle(MockIO& io){io.Touch();return io.idle;}
struct MethodMock {
 struct Authority {uint32_t source[120][16]={};};struct Cohort {Authority authority;} owned;
 Cohort* cohort=&owned;
 struct Core {uint32_t boot[64]={},job[64]={},status[16]={},counter[8]={},doorbell=0;};
 Core cores[120];uint32_t expectedBoot[120][64]={},cachedBoot[119][64]={},cachedJob[119][64]={},cachedCounters[119][8]={},nonces[119]={};
 uint32_t terminal[64]={},cancel[16]={},progress[3]={},session[128]={},staging[4096]={};
 uint32_t observedTerminal[64]={},terminalCounters[120][8]={},terminalNonces[120]={};
 bool terminalObserved=false,holding=false,held=false,execution=true,scope=true,unsafe=false,cancelRequested=true,running[120]={};
 unsigned failureCalls=0;
 struct Plan {unsigned Bank()const{return 7;}unsigned Workspace()const{return 0x100000;}} plan;
 struct Startup {
  MethodMock* owner;
  bool ReadBoot(unsigned w,uint32_t* out){assert(w<120);memcpy(out,owner->expectedBoot[w],256);return true;}
  bool ReadSession(unsigned,uint8_t* out,unsigned bytes){assert(bytes==512);memcpy(out,owner->session,512);return true;}
 } ownedStartup{this};Startup* startup=&ownedStartup;
 struct Memory {
  MethodMock* owner;
  bool VerifyImage(MockIO& io,unsigned w,PioneerQ38TokenMemoryResult& result){
   assert(!w && io.selected==0);io.Touch();io.imageChecks++;
   if(!io.image){result.word=71;return false;}
   switch(io.mutateAfterImage){
    case 0:break;case 1:owner->terminal[63]^=1;break;case 2:owner->cores[0].boot[17]^=1;break;
    case 3:owner->cores[0].counter[0]++;break;case 4:owner->cores[0].status[2]=1;break;
    case 5:io.fifo=false;break;case 6:io.source=false;break;case 7:io.idle=false;break;
    default:assert(false);
   }
   return true;
  }
 } ownedMemory{this};Memory* memory=&ownedMemory;
 MethodMock(){
  for(unsigned w=0;w<120;w++){
   running[w]=true;for(unsigned i=0;i<64;i++)expectedBoot[w][i]=cores[w].boot[i]=0x100000+w*256+i;
   for(unsigned i=0;i<8;i++)cores[w].counter[i]=100+w*8+(i==6?2:i);
   if(!w)continue;
   nonces[w-1]=w+7;auto& c=cores[w];c.doorbell=nonces[w-1];
   c.job[1]=c.doorbell;c.job[2]=PioneerQ38SelectedGlobalWorker::Stop;c.job[3]=w;
   c.job[4]=PioneerQ38SelectedGlobalWorker::Peer(w);c.job[7]=9998;
   c.status[0]=c.status[1]=c.doorbell;c.status[3]=PioneerQ38SelectedGlobalWorker::Magic;
   c.status[4]=PioneerQ38SelectedGlobalWorker::Stop;
   memcpy(cachedBoot[w-1],c.boot,256);memcpy(cachedJob[w-1],c.job,256);memcpy(cachedCounters[w-1],c.counter,32);
  }
  cores[0].status[3]=PioneerQ38TokenLayout::Magic;
  terminal[0]=PioneerQ38TokenLayout::TerminalMagic;terminal[1]=1;terminal[2]=3;terminal[6]=1;terminal[10]=1;
  for(unsigned i=0;i<8;i++)terminal[36+i]=cores[0].counter[i];
  cancel[0]=PioneerQ38TokenLayout::CancelMagic;
 }
 bool ExecutionStarted()const{return execution && !unsafe;}
 bool Scope(MockIO&)const{return scope && !unsafe;}
 bool Sources(MockIO& io,unsigned w){
  assert(w<120);if(!Scope(io))return false;io.Touch();if(!io.source)return false;
  io.selected=w;if(w)io.peerSources++;return true;
 }
 template<class Result> bool Fail(MockIO&,Result& r){unsafe=true;failureCalls++;r.unsafe=1;r.ready=0;return false;}
 bool Fail(MockIO&,PioneerQ38TokenExecutionResult& r){unsafe=true;failureCalls++;r.unsafe=1;r.acknowledged=0;return false;}
 uint32_t& Word(unsigned w,uint32_t address){
  namespace R=PioneerQ38GlobalChainLayout::R;namespace T=PioneerQ38TokenLayout;namespace W=PioneerQ38SelectedGlobalWorker;
  auto range=[&](uint32_t base,unsigned words){return address>=base && address<base+4*words && !(address&3);};
  if(w){
   if(range(W::Boot,64))return cores[w].boot[(address-W::Boot)/4];
   if(range(W::Job,64))return cores[w].job[(address-W::Job)/4];
   if(range(W::Status,16))return cores[w].status[(address-W::Status)/4];
   assert(address==W::Doorbell);return cores[w].doorbell;
  }
  if(range(R::Status,16))return cores[0].status[(address-R::Status)/4];
  if(range(R::Boot,64))return cores[0].boot[(address-R::Boot)/4];
  if(range(R::PeerBoots,119*64)){const auto i=(address-R::PeerBoots)/4;return cachedBoot[i/64][i%64];}
  if(range(R::PeerJobs,119*64)){const auto i=(address-R::PeerJobs)/4;return cachedJob[i/64][i%64];}
  if(range(R::Nonces,119))return nonces[(address-R::Nonces)/4];
  if(range(R::Counters,119*8)){const auto i=(address-R::Counters)/4;return cachedCounters[i/8][i%8];}
  if(range(T::TerminalLocal,64))return terminal[(address-T::TerminalLocal)/4];
  if(range(T::CancelLocal,16))return cancel[(address-T::CancelLocal)/4];
  assert(range(PioneerQ38GlobalChainLayout::Progress,3));return progress[(address-PioneerQ38GlobalChainLayout::Progress)/4];
 }
#include "actual_root_methods.inc"
};
void MockIO::Touch(){assert(owner && !owner->unsafe && !failed);calls++;}
bool MockIO::TokenMemoryReadChecked(unsigned w,uint32_t address,uint32_t& out){
 Touch();reads++;assert(w==selected);
 if(reads==failRead){failed=true;return false;}
 out=owner->Word(w,address);return true;
}
uint32_t MockIO::RuntimePcRead(){Touch();if(selected)return peerPC;return rootPC[pcReads++%4];}
uint32_t MockIO::ResidentNocRead(unsigned index){Touch();assert(index<8);return owner->cores[selected].counter[index];}
struct Fixture {std::unique_ptr<MethodMock> owner=std::make_unique<MethodMock>();MockIO io;Fixture(){io.owner=owner.get();mockSession=mockTerminalShape=mockJobShape=true;}};
static void Accepted(Fixture& f){
 PioneerQ38TokenTerminalResult r;assert(f.owner->InspectTerminal(f.io,r,false));
 assert(r.ready && !r.unsafe && f.owner->terminalObserved && !f.owner->unsafe && f.io.imageChecks==1 && f.io.peerSources==238);
 assert(r.firstPC==f.io.rootPC[2] && r.secondPC==f.io.rootPC[3]);
}
int main(){
 unsigned positives=0,refusals=0,pending=0;
 {Fixture f;PioneerQ38TokenTerminalResult r;assert(!f.owner->OldInspectTerminal(f.io,r,false));
  assert(f.owner->unsafe && r.phase==11 && r.worker==0 && r.word==63 && r.firstPC==0x60008 && r.secondPC==0x60008);refusals++;}
 constexpr uint32_t samples[]={0,1,0xffffffffU,0x60000,0x60004,0x60008,0x40010,0x1585ec};
 for(auto first:samples)for(auto final:samples){Fixture f;f.io.rootPC={first,first,final,final};Accepted(f);positives++;}
 for(auto raw:samples){
  Fixture f;f.io.rootPC.fill(raw);f.owner->terminal[0]=0;PioneerQ38TokenTerminalResult r;
  assert(f.owner->InspectTerminal(f.io,r,false) && !r.ready && !r.unsafe && !f.owner->terminalObserved
   && !f.io.peerSources && !f.io.imageChecks);pending++;
  PioneerQ38TokenExecutionResult observed;assert(f.owner->ObserveRoot(f.io,observed) && observed.acknowledged
   && !observed.unsafe && observed.firstPC==raw && observed.secondPC==raw && !f.owner->terminalObserved);positives++;
 }
 // Even before a terminal marker,actual boot corruption cannot be hidden
 // by either a terminal-looking or a fault-looking RAW frontend sample.
 for(unsigned word=0;word<64;word++)for(unsigned method=0;method<2;method++){
  Fixture f;f.owner->terminal[0]=0;f.owner->cores[0].boot[word]^=1;
  PioneerQ38TokenTerminalResult terminal;PioneerQ38TokenExecutionResult observed;
  const bool okay=method?f.owner->ObserveRoot(f.io,observed):f.owner->InspectTerminal(f.io,terminal,false);
  assert(!okay && f.owner->unsafe && f.owner->failureCalls==1 && !f.owner->terminalObserved);
  assert((method?observed.word:terminal.word)==word);refusals++;
 }
 for(unsigned kind=0;kind<11;kind++){
  Fixture f;
  if(kind==0)f.owner->cores[0].status[2]=1;if(kind==1)f.owner->terminal[0]^=1;
  if(kind==2)mockSession=false;if(kind==3)mockTerminalShape=false;if(kind==4)mockJobShape=false;
  if(kind==5)f.owner->cancel[0]=0;if(kind==6)f.owner->cores[0].counter[0]++;
  if(kind==7)f.io.fifo=false;if(kind==8)f.io.image=false;if(kind==9)f.io.source=false;if(kind==10)f.io.idle=false;
  PioneerQ38TokenTerminalResult r;assert(!f.owner->InspectTerminal(f.io,r,false) && f.owner->unsafe && !r.ready
   && !f.owner->terminalObserved && f.owner->failureCalls==1);refusals++;
 }
 for(unsigned worker=1;worker<120;worker++)for(unsigned kind=0;kind<7;kind++){
  Fixture f;auto& c=f.owner->cores[worker];
  if(kind==0)c.boot[17]^=1;if(kind==1)c.job[7]^=1;if(kind==2)c.status[2]=1;if(kind==3)c.doorbell++;
  if(kind==4)c.counter[0]++;if(kind==5)f.owner->nonces[worker-1]=0;if(kind==6)f.owner->cachedBoot[worker-1][47]^=1;
  PioneerQ38TokenTerminalResult r;assert(!f.owner->InspectTerminal(f.io,r,false) && f.owner->unsafe && !r.ready
   && r.worker==worker && !f.owner->terminalObserved && f.owner->failureCalls==1);refusals++;
 }
 // Second seal pass must catch a change AFTER the image endpoint succeeded.
 for(unsigned kind=1;kind<8;kind++){
  Fixture f;f.io.mutateAfterImage=kind;uint32_t expected[64],word=0;memcpy(expected,f.owner->terminal,256);
  assert(!f.owner->RootStopSeal(f.io,expected,word) && f.io.imageChecks==1 && !f.owner->terminalObserved);refusals++;
 }
 // Genuine read failure stops immediately;Touch forbids any following IO.
 for(unsigned fail:{1U,2U,3U,66U,67U,129U,500U,10000U,20000U,30000U}){
  Fixture f;f.io.failRead=fail;PioneerQ38TokenTerminalResult r;
  assert(!f.owner->InspectTerminal(f.io,r,false) && f.io.failed && f.io.reads==fail && f.owner->unsafe && !r.ready);refusals++;
 }
 for(unsigned kind=0;kind<5;kind++){
  Fixture f;Accepted(f);PioneerQ38TokenTerminalResult r;
  if(kind==0)f.owner->terminal[0]=0;if(kind==1)f.owner->terminal[63]^=1;
  if(kind==2)f.owner->nonces[118]++;if(kind==3)f.owner->cores[119].counter[0]++;if(kind==4)f.io.image=false;
  assert(!f.owner->InspectTerminal(f.io,r,true) && f.owner->unsafe && !r.ready);refusals++;
 }
 {Fixture f;Accepted(f);PioneerQ38TokenHoldResult r;f.io.rootPC.fill(0x60008);
  assert(f.owner->HoldGuard(f.io,0,r) && r.firstPC==0x60008 && r.secondPC==0x60008 && f.io.imageChecks==2);positives++;}
 for(unsigned kind=0;kind<5;kind++){
  Fixture f;Accepted(f);PioneerQ38TokenHoldResult r;
  if(kind==0)f.owner->terminalObserved=false;if(kind==1)f.owner->terminal[63]^=1;if(kind==2)f.io.image=false;
  if(kind==3)f.owner->cores[0].boot[9]^=1;if(kind==4)f.io.fifo=false;
  assert(!f.owner->HoldGuard(f.io,0,r));refusals++;
 }
 for(auto pc:{0x20060U,0x20064U,0x20068U}){
  Fixture f;Accepted(f);PioneerQ38TokenHoldResult r;f.io.peerPC=pc;
  assert(f.owner->HoldGuard(f.io,119,r)==(pc!=0x20068));if(pc==0x20068)refusals++;else positives++;
 }
 for(unsigned word=0;word<16;word++){
  Fixture f;PioneerQ38TokenTerminalResult r;reinterpret_cast<uint32_t*>(&r)[word]^=1;
  assert(!f.owner->InspectTerminal(f.io,r,false) && !f.owner->unsafe && !f.io.calls);refusals++;
 }
 for(unsigned kind=0;kind<5;kind++){
  Fixture f;if(kind==0)f.owner->holding=true;if(kind==1)f.owner->held=true;if(kind==2)f.owner->terminalObserved=true;
  if(kind==3)f.owner->execution=false;if(kind==4)f.owner->scope=false;PioneerQ38TokenTerminalResult r;
  assert(!f.owner->InspectTerminal(f.io,r,false) && !f.owner->unsafe && !f.io.calls);refusals++;
 }
 printf("PASS extracted ROOT methods/%u MOCK positives/%u refusals/%u unpublished pending;old physical PC sequence reproduced;RAW PC never grants readiness;current BOOT/status/terminal/119 peer/counter/code/FIFO guards and post-image second pass;read failure NO following IO. NO native/model/physical/Work/TPS proof\n",positives,refusals,pending);
}
