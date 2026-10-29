/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
// DISTINCT native preparation,not the legacy cohort's best-effort helpers.
// No RISC release,NoC request,counter clear/rebase,retry or cleanup. Original
// control receipts are saved ONCE while all participants are still held.
__attribute__((noinline)) bool PioneerSessionIO::PrepareTokenNativeParticipant(unsigned worker){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 if(worker>=120 || !s || s->phase!=PioneerQ38TokenNativeState::DispatchPreparing || !TokenModelSourceScope(false)
  || !c->q38DispatchRoutes || q38TokenMemoryMapAttempted || !s->memory || !s->startup || s->runtime)return false;
 auto* d=c->q38Dispatch;if(!d || d!=s->participants || d->prepared!=worker)return false;
 auto& node=d->worker[worker];auto& store=node.Store();auto& image=node.image;
 if(node.master!=(worker?NULL:&c->lease) || node.noc.state!=TT_COMPUTE_NOC_PASSED || !node.noc.tlb_restored
  || node.noc.recovery_required || node.noc.version!=(worker?PioneerWorkerVersion(worker):1)
  || !PioneerWorkerRoute(dram,node.noc.source[0],worker) || !BlackholeHostSourceRoute(node.noc.source[0])
  || image.saved || image.dirty || image.ready || image.running || image.unsafe || store.pending || store.recovery_required
  || (worker?(store.saved || store.controls_saved || store.configured || store.ready):(!store.saved || !store.ready)))return false;
 const uint32 originalConfig=worker?node.noc.source[0][0]:store.config;
 uint32 source[16],commands[10]={},counters[8]={};
 for(unsigned i=0;i<16;i++)source[i]=node.noc.source[0][i];
 uint32* scratch=worker?NULL:store.saved;uint32* saved=NULL;
 bool configured=!worker,receipts=false,selected=false;
 const uint64 began=Q38CohortNow();uint64 previous=began;
 if(began>UINT64_MAX-UINT64_C(300000000))return false;
 d->closeDetail[0]=0x410;d->closeWorker=worker; // CPU diagnostic ONLY,never authority
 auto same=[&](uint32 actual,uint32 expected,uint32 address){
  if(actual==expected)return true;
  d->closeDetail[1]=address;d->closeDetail[2]=actual;d->closeDetail[3]=expected;return false;
 };
 auto scope=[&](){
  if(!(q38MutableContext==c && c->q38TokenNative==s && c->q38Dispatch==d && s->participants==d
   && s->phase==PioneerQ38TokenNativeState::DispatchPreparing && !s->runtime && d->prepared==worker
   && !q38TokenMemoryMapAttempted && c->q38DispatchRoutes && runtimeMapped && fleetEnabled
   && node.master==(worker?NULL:&c->lease) && &node.Store()==&store && !store.pending && !store.recovery_required
   && !image.unsafe && !image.running && store.saved==scratch && TokenModelSourceScope(false)))return false;
  const uint64 now=Q38CohortNow();
  if(now<previous || now<began || now-began>=UINT64_C(300000000))return false;
  previous=now;return true;
 }; // CPU/native FD/thread/team/generation/clock/host scope before EVERY actual access
 auto access=[&](){return scope() && selected && runtimeCore==worker && runtimeEntry && runtimePc && scratchData
  && payloadBytes==8192 && Q38PanelMappingStable() && (!saved || Q38PanelSavedWords()==saved);};
 auto nocRead=[&](uint32 address,uint32& value){
  if((address!=0x64 && address!=0x68 && !BlackholeHostWriteReadRegister(address)) || !access())return false;
  value=address==0x64 || address==0x68?TokenFifoStatusWord(address):ResidentNocRead(address);return true;
 };
 auto read=[&](unsigned kind,unsigned word,uint32& value){
  if(kind>3 || (kind==0?word>=2048:kind==1?word>=TT_Q38_PANEL_VIEW_WORDS:kind==2?word!=0:word>=3) || !access())return false;
  value=kind==0?ResidentL1Read(word):kind==1?Q38PanelRead(TT_Q38_PANEL_VIEW_BASE+word*4):kind==2?RuntimeCodeRead(0):RuntimeControlRead(word);
  return true; // UINT32_MAX is legitimate original DATA
 };
 auto guard=[&](){
  if(!access() || ResetRead()!=0x47800 || !access() || ResetRead()!=0x47800)return false;
  for(unsigned i=0;i<16;i++){uint32 value,expected=i?source[i]:configured?(originalConfig|0x10000U):originalConfig;
   if(!nocRead(BlackholeDramSourceOffset(i),value) || !same(value,expected,BlackholeDramSourceOffset(i)))return false;}
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(!access() || SourceRead(i)!=dram.source[i])return false;
  bool pciOkay=true;
  auto pci=[&](unsigned peer,uint16_t offset,uint8_t bytes)->uint32{
   if(peer>=2 || !access()){pciOkay=false;return UINT32_MAX;}return HostPciRead(peer,offset,bytes);
  };
  struct Pci {decltype(pci)& call;uint32 HostPciRead(unsigned p,uint16_t o,uint8_t b){return call(p,o,b);}} pciReader{pci};
  for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(pciReader,p) || !pciOkay)return false;
  struct Fifo {decltype(nocRead)& call;unsigned worker;
   bool TokenFifoReadChecked(unsigned w,bool overflow,uint32& value){return w==worker && call(overflow?0x68U:0x64U,value);}
  } fifo{nocRead,worker};
  if(!PioneerQ38TokenFifoEmpty(fifo,worker,configured?(originalConfig|0x10000U):originalConfig))return false;
  for(unsigned i=0;i<4;i++){uint32 value;if(!nocRead(0x40+i*0x800,value) || value)return false;}
  for(unsigned i=0;i<32;i++){uint32 value;if(!nocRead(0x240+i*4,value) || (value&255))return false;}
  for(unsigned i=0;i<10;i++){uint32 value;if(!nocRead(i*4,value) || (receipts?value!=commands[i]:worker && value))return false;
   if(!receipts)commands[i]=value;}
  for(unsigned i=0;i<8;i++){uint32 value;if(!nocRead(PioneerQ38PanelCounter(i),value) || (receipts && !same(value,counters[i],PioneerQ38PanelCounter(i))))return false;
   if(!receipts)counters[i]=value;}
  receipts=true;return scope(); // first original save only;NEVER rebase after a known operation
 };
 // Exact physical selection uses only existing200/201,checks EACH known
 // mapping write before another write or ANY alias access. No index202.
 runtimeCore=worker;
 for(unsigned slot=0;slot<2;slot++)for(unsigned word=0;word<3;word++){
  const uint32 expected=BlackholeDramFixedTlbWord(slot?0xffa00000U:0U,PioneerWorkerX(worker),PioneerWorkerY(worker),false,word);
  if(!scope())return false;
  DramTlbWrite(slot,word,expected);
  if(!scope() || DramTlbRead(slot,word)!=expected)return false;
 }
 if(!scope() || !MapQ38Panel())return false;
 selected=true;
 d->closeDetail[0]=0x411;
 if(!guard())return false;
 if(worker){
  d->closeDetail[0]=0x412;
  if(!access())return false;
  scratch=HostReadBackup(2048);store.saved=scratch;
  if(!scratch || (reinterpret_cast<uintptr_t>(scratch)&3) || reinterpret_cast<uintptr_t>(scratch)>UINTPTR_MAX-8192 || !scope())return false;
  for(unsigned slot=0;slot<2;slot++)for(unsigned word=0;word<3;word++){
   if(!access())return false;
   store.tlb[slot][word]=DramTlbRead(slot,word);
  }
  if((store.tlb[0][2]&0xffffc000U) || (store.tlb[1][2]&0xffffc000U))return false;
  store.controls_saved=true;store.tlb_restored=false;store.config=originalConfig;
  uint32 irq;if(!nocRead(0x178,irq) || (irq&0xffff))return false;
  for(unsigned word=0;word<2048;word++){uint32 value;if(!read(0,word,value))return false;scratch[word]=value;}
  if(!guard())return false;
  d->closeDetail[0]=0x413;
  store.configured=true;store.niu_restored=false;configured=true;
  if(!access())return false;
  ResidentNocWrite(0x100,originalConfig|0x10000U); // ONLY NIU write,NEVER START
  uint32 actual;if(!nocRead(0x100,actual) || actual!=(originalConfig|0x10000U) || !guard())return false;
  store.ready=true;
 }
 d->closeDetail[0]=0x414;
 if(!guard() || !access())return false;
 saved=Q38PanelSavedWords();
 if(!saved || (reinterpret_cast<uintptr_t>(saved)&3)
  || reinterpret_cast<uintptr_t>(saved)>UINTPTR_MAX-(size_t(TT_Q38_PANEL_VIEW_WORDS)+1)*4 || !access())return false;
 d->closeDetail[0]=0x415;
 for(unsigned word=0;word<3;word++){uint32 value;if(!read(3,word,value) || !same(word?value:value&1,0,0xffb121b0U+word*4))return false;}
 d->closeDetail[0]=0x416;
 if(!read(2,0,saved[0]))return false;
 for(unsigned word=0;word<TT_Q38_PANEL_VIEW_WORDS;word++)if(!read(1,word,saved[word+1]))return false;
 d->closeDetail[0]=0x417;
 if(!guard())return false;
 image.saved=true;image.dirty=true;
 d->closeDetail[0]=0x418;
 for(uint32 address=0x70000;address<0x74000;address+=4){
  const uint32 expected=address<0x72000?
#ifdef PIONEER_Q38_INT_SFPU
   PioneerQ38DispatchPrefixWord(worker,address)
#else
   0U
#endif
   :PioneerQ38DispatchRoleWord(worker,(address-0x72000)/4);
  if(!access())return false;
  Q38PanelWrite(address,expected);
  uint32 actual;if(!read(1,(address-TT_Q38_PANEL_VIEW_BASE)/4,actual) || !same(actual,expected,address))return false;
 }
 d->closeDetail[0]=0x419;
 if(!access())return false;
 RuntimeCodeWrite(0,TT_Q38_DISPATCH_BOOT);
 uint32 entry;if(!read(2,0,entry) || entry!=TT_Q38_DISPATCH_BOOT || !guard())return false;
 d->closeDetail[0]=0x41a;
 for(uint32 address=0x70000;address<0x74000;address+=4){
  const uint32 expected=address<0x72000?
#ifdef PIONEER_Q38_INT_SFPU
   PioneerQ38DispatchPrefixWord(worker,address)
#else
   0U
#endif
   :PioneerQ38DispatchRoleWord(worker,(address-0x72000)/4);
  uint32 actual;if(!read(1,(address-TT_Q38_PANEL_VIEW_BASE)/4,actual) || actual!=expected)return false;
 }
 d->closeDetail[0]=0x41b;
 if(!read(2,0,entry) || entry!=TT_Q38_DISPATCH_BOOT || !guard() || !scope())return false;
 for(unsigned i=0;i<16;i++)d->authority.source[worker][i]=i?source[i]:(originalConfig|0x10000U);
 image.ready=true;d->prepared=worker+1;
 for(auto& value:d->closeDetail)value=0;
 d->closeWorker=0;return true;
} // caller retains every partial lease/alias on failure;NO retry or legacy cleanup
#endif
