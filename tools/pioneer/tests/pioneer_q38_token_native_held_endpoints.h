/* SPDX-License-Identifier: MIT */
// TEST hardware/VM endpoints ONLY. Root zero execution and all120 L1/CSR
// histories belong to ONE fixture from construction through held restoration.
// No ready flag,private model state,journal or processor history is copied.
static unsigned tokenChildFail=0,tokenChildAttempts=0;
void* operator new(std::size_t bytes,const std::nothrow_t&) noexcept{
 if(tokenChildFail && ++tokenChildAttempts==tokenChildFail)return nullptr;
 try{return ::operator new(bytes);}catch(...){return nullptr;}
} // only unpublished-child OOM injection;ordinary allocation/delete remain real
using area_id=int;
static constexpr int B_ANY_KERNEL_ADDRESS=1,B_KERNEL_READ_AREA=2,B_KERNEL_WRITE_AREA=4,B_UNCACHED_MEMORY=8;
struct TokenHeldEndpoints:ZeroControlIO {
 static constexpr uint64 Bar=UINT64_C(0x900000000);
 static constexpr unsigned Words=0x180000/4;
 struct{uint64 bar0_host_start=Bar;}profile;
 std::array<std::unique_ptr<PanelIO>,119> peers;
 std::array<std::vector<uint32>,120> extra;
 std::vector<uint32> alias=std::vector<uint32>(Words),original;
 unsigned runtimeCore=0,maps=0,aliasReads=0,aliasWrites=0;
 const PioneerQ38TokenMemoryResult* journalResult=nullptr;
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE
 std::vector<uint32> beforeNativeImage;
 const PioneerQ38TokenRestoreResult* nativeRestoreResult=nullptr;
 bool restoreFault=false;unsigned restoreFaultSeen=0,restoreFaultTouches=0;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE
 std::vector<uint32> beforePeerScratch;std::array<uint32,119> beforePeerConfig={};
 const PioneerQ38TokenPeerRestoreResult* nativePeerResult=nullptr;
 unsigned peerRestoreFault=0,peerFaultSeen=0,peerFaultTouches=0,peerMapWriteSeen=0;
 void SnapshotPrePeerStores(){
  assert(beforePeerScratch.empty() && rootStarts==1010 && rootStops==1010 && rootHolds==1010);
  beforePeerScratch.resize(119*2048);
  for(unsigned w=1;w<120;w++){
   assert(Core(w).coreReset==0x47800);beforePeerConfig[w-1]=Core(w).config;
   for(unsigned i=0;i<2048;i++)beforePeerScratch[(w-1)*2048+i]=Raw(w,0x17e000+i*4);
  }
 } // actual pre-lease physical byte/config observation,NEVER fed back to a controller or processor
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE
 std::vector<uint32> beforeDispatchImage;
 const PioneerQ38TokenDispatchRestoreResult* nativeDispatchResult=nullptr;
 bool dispatchRestoreFault=false;unsigned dispatchFaultSeen=0,dispatchFaultTouches=0;
 void SnapshotPreDispatch(){
  assert(beforeDispatchImage.empty() && rootStarts==1010 && rootStops==1010 && rootHolds==1010);
  beforeDispatchImage.resize(120U*(TT_Q38_PANEL_VIEW_WORDS+1U));
  for(unsigned w=0;w<120;w++){
   assert(Core(w).coreReset==0x47800);
   const unsigned base=w*(TT_Q38_PANEL_VIEW_WORDS+1U);beforeDispatchImage[base]=Raw(w,0x6000);
   for(unsigned i=0;i<TT_Q38_PANEL_VIEW_WORDS;i++)beforeDispatchImage[base+1+i]=Raw(w,TT_Q38_PANEL_VIEW_BASE+i*4);
  }
 } // actual physical bytes before ANY dispatch preparation,not saved[] or a supplied processor input
#endif
 unsigned fault=0,faultSeen=0,touchesAtFault=0;
#ifdef PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE
 bool runtimeMode=false;unsigned runtimeWorkspace=0,runtimeFault=0,runtimePackets=0,runtimeWrites=0,runtimeReads=0,runtimeObserved=0;
 std::vector<uint32> runtimeDram=std::vector<uint32>(67108864/4,0x13579bdf);
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
 bool (*executionDramRead)(TokenHeldEndpoints&,unsigned,uint32,uint8*,unsigned)=nullptr;
 bool (*executionDramPut)(TokenHeldEndpoints&,unsigned,uint32,uint32)=nullptr;
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
 void (*observeVocabulary)(TokenHeldEndpoints&,unsigned)=nullptr;
#endif
#endif
#endif
 const char* lastOp="none";unsigned lastAddress=0,lastValue=0;
 uint32 Trace(const char* op,unsigned address,uint32 value){lastOp=op;lastAddress=address;lastValue=value;return value;}
 unsigned payloadBytes=8192;bool fleetEnabled=false;
 uint32* runtimeEntry=&entry;uint32* scratchData=destination;
 const volatile uint32* runtimePc=&entry; // simulated VM handle only;actual PC reads use the instruction CPU
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
 bool executionDropCancel=false;unsigned executionCancelWrites=0,executionCancelObserved=0;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
 bool trafficObserve=false;unsigned trafficFault=0,trafficObserved=0;
#endif
 explicit TokenHeldEndpoints(const char* image):ZeroControlIO(image){
  for(auto& peer:peers){peer=std::make_unique<PanelIO>();peer->transferBytes=8192;}
  for(unsigned w=0;w<120;w++){
   extra[w].resize(Words);
   for(unsigned i=0;i<Words;i++)extra[w][i]=i%17?uint32(0x9e3779b9U*(i+1)^0x1234567U*(w+1)):UINT32_MAX;
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE
   if(w)for(unsigned i=0;i<2048;i++)Core(w).destination[i]=i%17?uint32(0xa5973101U*(i+1)^w):UINT32_MAX;
#endif
  }
  // Qualify the simulated endpoint's multiplexing BEFORE any owner/ROM
  // history exists. Inherited root-only panel methods must never be used.
  const unsigned address=0x74000,at=(address-TT_Q38_PANEL_VIEW_BASE)/4;
  for(unsigned w=0;w<120;w++){runtimeCore=w;Q38PanelWrite(address,0x5a000000+w);}
  for(unsigned w=0;w<120;w++){runtimeCore=w;assert(Q38PanelRead(address)==0x5a000000+w);}
  for(unsigned w=0;w<120;w++){runtimeCore=w;Q38PanelWrite(address,0xab860000+at);}
  runtimeCore=0;
 }
 PanelIO& C(){assert(runtimeCore<120);return runtimeCore?*peers[runtimeCore-1]:static_cast<PanelIO&>(*this);}
 PanelIO& Core(unsigned worker){assert(worker<120);return worker?*peers[worker-1]:static_cast<PanelIO&>(*this);}
 uint32& Raw(unsigned worker,uint32 address){
  assert(worker<120 && !(address&3) && address<0x180000);auto& c=Core(worker);
  if(address==0x6000)return c.entry;
  if(address>=TT_Q38_PANEL_VIEW_BASE && address-TT_Q38_PANEL_VIEW_BASE<TT_Q38_PANEL_VIEW_BYTES)
   return c.view.at((address-TT_Q38_PANEL_VIEW_BASE)/4);
  if(address>=0x140000 && address<0x148000)return c.l1[(address-0x140000)/4];
  if(address>=0x17e000)return c.destination[(address-0x17e000)/4];
  return extra[worker].at(address/4);
 }
 uint32 Address(const volatile uint32* p)const{
  const uintptr_t a=reinterpret_cast<uintptr_t>(p),base=reinterpret_cast<uintptr_t>(alias.data());
  assert(a>=base && a-base<Words*4 && !((a-base)&3));return uint32(a-base);
 }
 void ObserveAlias(unsigned address,bool write){
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE
  if(beforeNativeImage.empty() && !write && journalResult && journalResult->phase==1 && !runtimeCore){
   assert(address==0x40000);beforeNativeImage.resize(PioneerQ38TokenL1::SavedWords);
   for(unsigned w=0;w<120;w++){
    assert(Core(w).coreReset==0x47800);
    for(unsigned i=0;i<PioneerQ38TokenL1::SnapshotWords(w);i++)
     beforeNativeImage[PioneerQ38TokenL1::SnapshotBase(w)+i]=Raw(w,PioneerQ38TokenL1::SnapshotAddress(w,i));
   }
  } // independent byte OBSERVATION before any native image write;never fed to an owner/journal or processor
  if(restoreFault && !restoreFaultSeen && nativeRestoreResult && nativeRestoreResult->phase==2 && write && !runtimeCore && address==0x40000){
   restoreFaultSeen++;restoreFaultTouches=touches;sReloadGeneration++;
  } // after an ACTUAL original-L1 write;following native access must refuse
#endif
  if(!fault || faultSeen || !journalResult)return;
  const bool hit=fault==1?!write && journalResult->phase==1 && !runtimeCore && address==0x40000
   :fault==2?write && journalResult->phase==3 && runtimeCore==119 && address==0x64000
   :write && journalResult->phase==6 && !runtimeCore && address==0x60004;
  if(hit){faultSeen++;touchesAtFault=touches;sReloadGeneration++;}
 } // TEST metadata loss AFTER an actual alias access;next native check must refuse
 void SnapshotOriginal(){
  assert(original.empty() && runtimeCore==0 && rootStarts==1010 && rootStops==1010 && rootHolds==1010);
  original.resize(PioneerQ38TokenL1::SavedWords);
  for(unsigned w=0;w<120;w++){
   assert(Core(w).coreReset==0x47800);
   for(unsigned i=0;i<PioneerQ38TokenL1::SnapshotWords(w);i++)
    original[PioneerQ38TokenL1::SnapshotBase(w)+i]=Raw(w,PioneerQ38TokenL1::SnapshotAddress(w,i));
  }
 }
 bool SelectRuntimeCore(unsigned w){
  Touch();assert(w<120);runtimeCore=w;
  // Simulated fixed-window endpoint,as in the existing cohort fixture;
  // actual topology/source-ID probes still execute in the production helper.
  return BlackholeDramConfigure(C(),0,0,1,2,false) && BlackholeDramConfigure(C(),1,0xffa00000,1,2,false);
 }
 bool MapRuntime(){Touch();return true;}
 bool MapQ38Panel(){Touch();return C().mapping;}
 bool Q38PanelMappingStable(){Touch();return C().mapping;}
 uint32 Q38PanelRead(uint32 address){Touch();
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
  if(!runtimeCore && address==PioneerQ38TokenLayout::CancelLocal && executionDropCancel && executionCancelWrites)executionCancelObserved=touches;
#endif
  const uint32 value=Trace("panel",address,C().Q38PanelRead(address));
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
  if(trafficObserve && !runtimeCore && address==PioneerQ38GlobalChainLayout::R::Status+12 && !trafficObserved){
   if(trafficFault==1){sReloadGeneration++;trafficObserved=touches;}
   if(trafficFault==2){Core(0).source[11]^=1;trafficObserved=touches;}
  }
#endif
  return value;
 }
 void Q38PanelWrite(uint32 address,uint32 value){Touch();
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
  if(!runtimeCore && C().coreReset==0x46800){
   assert(address==PioneerQ38TokenLayout::CancelLocal && value==PioneerQ38TokenLayout::CancelMagic && !executionCancelWrites);
   executionCancelWrites++;if(!executionDropCancel)Raw(0,address)=value;return;
  }
#endif
  C().Q38PanelWrite(address,value);
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE
  if(dispatchRestoreFault && !dispatchFaultSeen && nativeDispatchResult && nativeDispatchResult->phase==2
   && runtimeCore==119 && address==TT_Q38_PANEL_VIEW_BASE){
   dispatchFaultSeen++;dispatchFaultTouches=touches;sReloadGeneration++;
  } // generation loss AFTER an actual original dispatch write;following checked read must refuse
#endif
 }
 uint32* Q38PanelSavedWords(){Touch();return C().Q38PanelSavedWords();}
 uint32* HostReadBackup(unsigned words){Touch();return C().HostReadBackup(words);}
 void DramTlbWrite(unsigned s,unsigned w,uint32 value){Touch();C().DramTlbWrite(s,w,value);
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE
  if(nativePeerResult && nativePeerResult->started){
   C().nativePhysicalWindow=true;C().nativeWindowX=PioneerWorkerX(runtimeCore);C().nativeWindowY=PioneerWorkerY(runtimeCore);
  } // expected physical target for this endpoint;actual TLB words still independently checked,never rewritten by the TEST
  if(peerRestoreFault==2 && nativePeerResult && nativePeerResult->phase==1 && !runtimeCore && !s && !w)peerMapWriteSeen++;
#endif
 }
 uint32 DramTlbRead(unsigned s,unsigned w){Touch();const uint32 actual=C().DramTlbRead(s,w);
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE
  if(peerRestoreFault==2 && !peerFaultSeen && nativePeerResult && nativePeerResult->phase==1 && !runtimeCore && !s && !w){
   assert(peerMapWriteSeen==1);peerFaultSeen++;peerFaultTouches=touches;return actual^1U;
  } // FIRST bad readback follows an ACTUAL known mapping write;no supplied ready/PC receipt
#endif
  return actual;
 }
 uint32 SourceRead(unsigned w){Touch();return Trace("source",w,C().SourceRead(w));}
 uint32 ResidentNocRead(uint32 address){Touch();const uint32 actual=Trace("noc",address,C().ResidentNocRead(address));
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE
  if(peerRestoreFault==3 && !peerFaultSeen && nativePeerResult && nativePeerResult->phase==3 && runtimeCore==119 && address==0x100){
   peerFaultSeen++;peerFaultTouches=touches;return actual^1U;
  }
#endif
  return actual;
 }
 uint32 TokenFifoStatusWord(uint32 address){
  Touch();assert(address==0x64 || address==0x68);
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE
  if(!peerFaultSeen && nativePeerResult && nativePeerResult->phase==1 && !runtimeCore
   && ((peerRestoreFault==5 && address==0x64) || (peerRestoreFault==6 && address==0x68))){
   peerFaultSeen++;peerFaultTouches=touches;return address==0x64?0x1010100fU:1U;
  } // first bad queued/overflow status on SAME genuine predecessor;NO ready/STOP supplied
#endif
  // This simulator initiates requests synchronously,no request FIFO exists
  // in front of that endpoint. Outstanding work is independently modeled.
  return C().NativeFifoStatusWord(address);
 }
 uint32 HostPciRead(unsigned p,uint16_t address,uint8_t bytes){Touch();return Trace("pci",p*0x10000U+address,StoreMock::HostPciRead(p,address,bytes));}
 void ResidentNocWrite(uint32 address,uint32 value){Touch();
#ifdef PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE
  if(runtimeMode && address==0x40){
   assert(!runtimeCore && value==1 && !HostReadMock::quarantined && !wideQuarantined && runtimeWorkspace);
   auto& c=C();auto* command=c.command;const bool write=command[7]==0x212;
   assert(write || !command[7]);const unsigned remote=command[write?3:0],bytes=command[8],route=PioneerQ38ProjectionRoute(0);
   assert(!command[1] && !command[4] && !command[6] && !command[9] && !(remote&63) && !(bytes&63) && bytes && bytes<=16384
    && command[write?0:3]==PioneerQ38GlobalChainLayout::R::Scratch && command[write?2:5]==129 && command[write?5:2]==route
    && remote>=runtimeWorkspace && remote<=runtimeWorkspace+67108864-bytes && c.coreReset==0x47800);
   runtimePackets++;c.doorbells++;c.doorbellRead=false;
   if(write)runtimeWrites++;else runtimeReads++;
   for(unsigned i=0;i<bytes/4;i++){
    auto& local=Raw(0,PioneerQ38GlobalChainLayout::R::Scratch+i*4);auto& device=runtimeDram.at((remote-runtimeWorkspace)/4+i);
    if(write)device=local;else local=device;
   }
   if(write){c.writeCounters[0]++;c.writeCounters[1]+=bytes/64;c.writeCounters[3]++;}
   else{c.counters[0]++;c.counters[1]+=bytes/64;c.counters[3]++;}
   c.accepted++;
   if(runtimeFault==1 && runtimePackets==1){sReloadGeneration++;runtimeObserved=touches;}
   if(runtimeFault==2 && runtimePackets==4616){Raw(0,PioneerQ38GlobalChainLayout::R::Scratch)^=1;runtimeObserved=touches;}
   if(runtimeFault==3 && runtimePackets==1){c.counters[0]++;runtimeObserved=touches;}
   return;
  }
#endif
  C().ResidentNocWrite(address,value);
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE
  if(peerRestoreFault==1 && !peerFaultSeen && nativePeerResult && nativePeerResult->phase==2 && runtimeCore==119 && !address){
   peerFaultSeen++;peerFaultTouches=touches;sReloadGeneration++;
  } // generation loss AFTER an ACTUAL original command write;next native access must refuse
#endif
 }
 uint32 RuntimeCodeRead(unsigned word){Touch();return Trace("entry",word,C().RuntimeCodeRead(word));}
 void RuntimeCodeWrite(unsigned word,uint32 value){Touch();C().RuntimeCodeWrite(word,value);}
 uint32 RuntimeControlRead(unsigned word){Touch();return C().RuntimeControlRead(word);}
 uint32 RuntimeStatusRead(unsigned word){Touch();return C().RuntimeStatusRead(word);}
 void RuntimeStatusWrite(unsigned word,uint32 value){Touch();C().RuntimeStatusWrite(word,value);}
 uint32 ResidentL1Read(unsigned word){Touch();const uint32 actual=C().ResidentL1Read(word);
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE
  if(peerRestoreFault==4 && !peerFaultSeen && nativePeerResult && nativePeerResult->phase==4 && runtimeCore==119 && !word){
   peerFaultSeen++;peerFaultTouches=touches;return actual^1U;
  }
#endif
  return actual;
 }
 void ResidentL1Write(unsigned word,uint32 value){Touch();C().ResidentL1Write(word,value);}
 uint32 ResetRead(){
  Touch();if(runtimeCore)return Trace("reset",runtimeCore,C().coreReset);
  for(unsigned target=0;target<120;target++)for(bool readOnly:{false,true}){
   bool same=true;for(unsigned w=0;w<3;w++)same&=tlb[1][w]==BlackholeDramFixedTlbWord(0xffa00000,PioneerWorkerX(target),PioneerWorkerY(target),readOnly,w);
   if(same)return Trace("reset",target,Core(target).coreReset);
  }
  assert(false && "reset outside an actual selected fixed window");return 0;
 }
 uint32 ComputeNocRead(uint32 offset){
  Touch();unsigned target=120;
  for(unsigned w=0;w<120;w++){
   bool same=true;for(unsigned i=0;i<3;i++)same&=C().tlb[1][i]==BlackholeDramFixedTlbWord(0xffa00000,PioneerWorkerX(w),PioneerWorkerY(w),true,i);
   if(same)target=w;
  }
  assert(target<120);
  if(offset==0x44)return 0x1c611000|BlackholeDramSourceX(C().source,PioneerWorkerX(target))|(PioneerWorkerY(target)<<6);
  if(offset==0x48)return 0x10000|target;
  for(unsigned w=0;w<16;w++)if(offset==BlackholeDramSourceOffset(w))return C().source[w];
  if(offset==0x40 || offset==0x840 || offset==0x1040 || offset==0x1840 || (offset>=0x240 && offset<0x280))return 0;
  assert(false);return 0;
 }
};
static TokenHeldEndpoints* tokenHeldEndpoint=nullptr;
static area_id map_physical_memory(const char*,uint64 physical,unsigned bytes,int placement,unsigned flags,void** view){
 auto& io=*tokenHeldEndpoint;io.Touch();assert(io.maps<4 && placement==B_ANY_KERNEL_ADDRESS
  && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));
 const unsigned offsets[]={0x20000,0x40000,0x60000,0xac000},lengths[]={8192,131072,57344,868352};
 const unsigned i=io.maps++;io.Trace("map",i,bytes);assert(physical==TokenHeldEndpoints::Bar+0x19000000+offsets[i] && bytes==lengths[i]);
 if(!i)io.SnapshotOriginal();*view=io.alias.data()+offsets[i]/4;return int(i)+37;
}
static uint32 PioneerRead32(const volatile uint32* p){auto& io=*tokenHeldEndpoint;io.Touch();io.aliasReads++;
 const auto address=io.Address(p),value=io.Raw(io.runtimeCore,address);io.ObserveAlias(address,false);return value;}
static void PioneerWrite32(volatile uint32* p,uint32 value){auto& io=*tokenHeldEndpoint;io.Touch();io.aliasWrites++;
 const auto address=io.Address(p);io.Raw(io.runtimeCore,address)=value;io.ObserveAlias(address,true);}
