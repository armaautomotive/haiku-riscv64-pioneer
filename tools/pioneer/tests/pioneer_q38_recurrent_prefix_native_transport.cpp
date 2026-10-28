/* SPDX-License-Identifier: MIT */
// TEST raw endpoints ONLY, compiled apart from the genuine native role.
// No native permission, ready record, PC/nonce or CPU product is supplied.
#define Machine PrefixNativeMachineBase
#define RuntimeMachine PrefixNativeRuntimeMachine
#define BlockMachine PrefixNativeBlockMachine
#define StreamMachine PrefixNativeStreamMachine
#define ScalarMachine PrefixNativeScalarMachine
#define PIONEER_Q38_RECURRENT_PREFIX_OWNER_ACTUAL_FIXTURE_ONLY
#include "pioneer_q38_recurrent_prefix_owner_actual_test.cpp"
#include "pioneer_q38_recurrent_prefix_native_transport.h"
#include "../PioneerQ38RecurrentOriginalWeightBytes.h"
#include "../PioneerQ38RecurrentPrefixComparison.h"
struct NativeOriginalPrefixMachine:AutonomousPrefixMachine {
 RecurrentBlockRawLearnedBytes* original=nullptr;
 std::array<std::array<std::vector<uint32_t>,PioneerRecurrentStageCount>,3> captured;
 NativeOriginalPrefixMachine(const char* root,const char* worker):AutonomousPrefixMachine(root,worker){}
 bool Attach(RecurrentBlockRawLearnedBytes& bytes){
  if(original || !bytes.Ready())return false;
  unsigned count=0;uint64_t length=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);if(p->kind!=canonical->WeightKind || p->layer>=3)continue;
   if(!bytes.Contains(PioneerQ38ProjectionRoute(p->bank),canonical->Base+p->offset,p->bytes))return false;
   count++;length+=p->bytes;
  }
  if(count!=168 || length!=842775168 || !bytes.Ready())return false;
  // Remove ALL synthetic learned words for ALL3 layers. Once attached,
  // Get has no generator fallback, including missing original extents.
  for(auto i=dram.begin();i!=dram.end();){
   if(bytes.Contains(unsigned(i->first>>32),unsigned(i->first),4))i=dram.erase(i);else ++i;
  }
  original=&bytes;return true;
 }
 uint32_t Get(unsigned route,unsigned address)const override{
  if(!original)return AutonomousPrefixMachine::Get(route,address);
  if(original->Contains(route,address,4)){uint32_t value;assert(original->Word(route,address,value));return value;}
  auto raw=dram.find(Key(route,address));assert(raw!=dram.end());return raw->second;
 }
 void Submit()override{
  if(original)assert(original->BeginRawPacket());AutonomousPrefixMachine::Submit();if(original)assert(original->EndRawPacket());
 }
 void LocalCapture(PioneerRecurrentOriginalStage stage,unsigned address,unsigned words){
  auto& out=captured.at(outer[3])[stage];assert(out.empty());out.resize(words);
  for(unsigned i=0;i<words;i++)out[i]=memory.at(address+i*4);
 }
 void RemoteCapture(PioneerRecurrentOriginalStage stage,unsigned role,unsigned words){
  auto& out=captured.at(outer[3])[stage];assert(out.empty());out.resize(words);
  for(unsigned i=0;i<words;i++)out[i]=Get(outer[8],outer[9]+PioneerQ38MixedRecurrentOffset(role)+i*4);
 }
 void ObserveMatrix()override{
  bool fresh=Phase()!=observed;RecurrentBlockMachine::ObserveMatrix();if(!original || !fresh)return;
  switch(Phase()){
   case 2:LocalCapture(PioneerRecurrentEmbedding,RB::OriginalCopy,5120);LocalCapture(PioneerRecurrentNormalized,RB::Input,5120);break;
   case 8:{
    RemoteCapture(PioneerRecurrentAlpha,10,48);RemoteCapture(PioneerRecurrentBeta,11,48);RemoteCapture(PioneerRecurrentZ,5,6144);RemoteCapture(PioneerRecurrentQkv,6,10240);
    auto& stages=captured.at(outer[3]);auto& conv=stages[PioneerRecurrentConv];conv.resize(10240,0x7fc00000);
    for(unsigned w=0;w<120;w++)for(unsigned batch=0;batch<PioneerQ38ConvBatchCount(w);batch++){
     const auto* d=block->Conv()+w*384+batch*64;for(unsigned i=0;i<16;i++)conv.at(d[7]+i)=Get(d[8],d[4]+d[43]+i*4);
    }
    auto& state=stages[PioneerRecurrentState];state.resize(786432,0x7fc00000);auto& raw=stages[PioneerRecurrentRaw];raw.resize(6144,0x7fc00000);
    for(unsigned w=0;w<96;w++){
     const auto* d=block->Gdn()+w*192+64;unsigned head=w/2,half=w%2;
     for(unsigned key=0;key<128;key++)for(unsigned value=0;value<64;value++)state[head*16384+key*128+half*64+value]=Get(d[13],d[7]+(key*64+value)*4);
     for(unsigned i=0;i<64;i++)raw[head*128+half*64+i]=child[w]->memory.at(0x8e800+i*4);
    }
    auto& gated=stages[PioneerRecurrentGated];gated.resize(6144);for(unsigned i=0;i<6144;i++)gated[i]=Get(outer[8],outer[9]+TT_Q38_GDN_GATED_OUTPUT+i*4);
    break;
   }
   case 10:RemoteCapture(PioneerRecurrentProjection,13,5120);LocalCapture(PioneerRecurrentResidual,RB::PostResidual,5120);LocalCapture(PioneerRecurrentPostNormalized,RB::PostNormalized,5120);break;
   case 13:RemoteCapture(PioneerRecurrentFfnGate,2,17408);RemoteCapture(PioneerRecurrentFfnUp,3,17408);LocalCapture(PioneerRecurrentActivated,RB::Input,17408);break;
  }
 } // snapshots observe only; never change instructions, metadata or products
 void Store(uint32_t address,uint32_t value)override{
  unsigned before=completedBlocks;AutonomousPrefixMachine::Store(address,value);
  if(!original || completedBlocks==before)return;
  assert(completedBlocks==before+1 && outer[3]==before);RemoteCapture(PioneerRecurrentDown,4,5120);
  auto& out=captured.at(before)[PioneerRecurrentFinal];assert(out.empty());out.resize(5120);
  for(unsigned i=0;i<5120;i++)out[i]=Get(outer[8],outer[9]+RB::FinalOffset+i*4);
 }
};
struct RecurrentPrefixNativeTransport::Impl:PrefixOwnerActualIOBase<NativeOriginalPrefixMachine> {
 using OwnerIO=PrefixOwnerActualIOBase<NativeOriginalPrefixMachine>;
 unsigned resets=0;bool configured=false;
 bool readback=false,reading=false;unsigned readbackFault=0,readbackFence=0,readPackets=0;
 std::array<uint32_t,10240> inheritedPrefix={}; // untouched QPE1/legacy64000..6e000 padding outside new snapshot
 Impl(const char* root,const char* worker):OwnerIO(root,worker){
  (void)&PrefixOwnerUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;
  (void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&Refusals;(void)&LateRefusals;(void)&RunPersistentGdnPair;
  assert(CodeIO::SelectRuntimeCore(0));track=true;
 }
 bool AtReadback()const{return readback && execution && execution->phase==7 && holds==120;}
 void ReadbackFaultHit(){if(!readbackFence)readbackFence=touches;}
 void ResidentNocWrite(uint32_t a,uint32_t value){
  if(!AtReadback() || a!=0x40 || C().command[7]){OwnerIO::ResidentNocWrite(a,value);return;}
  Touch();assert(!current && value==1 && readPackets<5 && execution->completed==119 && execution->stopped==119 && execution->held==120);
  for(unsigned w=0;w<120;w++)assert(core[w]->coreReset==0x47800);
  const auto* o=operation->Layer(2)->Outer();assert(o[3]==2 && o[48]==3);
  const unsigned next[]={o[9]+RB::FinalOffset+readPackets*4096,0,o[8],0x80000,0,129,0,0,4096,0};
  for(unsigned i=0;i<10;i++)assert(C().command[i]==next[i]);
  if(readbackFault!=1)for(unsigned i=0;i<1024;i++)PhysicalWrite(0,0x80000+i*4,machine.Get(o[8],next[0]+i*4));
  auto& c=C();c.counters[0]++;c.counters[1]+=64;c.counters[3]++;c.accepted++;readPackets++;reading=true;
 } // raw fixed READ bytes/counters only;never completion,STOP,hold or native permission
 uint32_t ReadLocal(uint32_t a){
  if(a==(current?unsigned(M::Status):unsigned(RB::Status)))Advance(current);
  if(PioneerQ38RecurrentPrefixSnapshotContains(current,a)){
   auto value=PhysicalRead(current,a);
   if(AtReadback() && reading && readbackFault==1 && !current && a>=0x80000 && a<0x81000)ReadbackFaultHit();
   return value;
  }
  if(!current && a>=0x64000 && a<0x6e000)return inheritedPrefix.at((a-0x64000)/4);
  if(a>=TT_Q38_PANEL_VIEW_BASE && a<TT_Q38_PANEL_VIEW_BASE+TT_Q38_PANEL_VIEW_BYTES)return C().Q38PanelRead(a);
  assert(a>=0x17e000 && a<0x180000);return C().ResidentL1Read((a-0x17e000)/4);
 }
 void WriteLocal(uint32_t a,uint32_t value){
  if(PioneerQ38RecurrentPrefixSnapshotContains(current,a)){
   if(AtReadback() && !current && a==0x80000)reading=false;
   PhysicalWrite(current,a,value);return;
  }
  if(!current && a>=0x64000 && a<0x6e000){inheritedPrefix.at((a-0x64000)/4)=value;return;}
  if(a>=TT_Q38_PANEL_VIEW_BASE && a<TT_Q38_PANEL_VIEW_BASE+TT_Q38_PANEL_VIEW_BYTES){C().Q38PanelWrite(a,value);return;}
  assert(a>=0x17e000 && a<0x180000);C().ResidentL1Write((a-0x17e000)/4,value);
 }
};
RecurrentPrefixNativeTransport::RecurrentPrefixNativeTransport(const char* root,const char* worker):implementation(new Impl(root,worker)){}
RecurrentPrefixNativeTransport::~RecurrentPrefixNativeTransport(){delete implementation;}
void RecurrentPrefixNativeTransport::SeedRoot(const uint32_t* a,const uint32_t* v,unsigned n,const uint32_t* source,const uint32_t* command,const uint32_t* counts){
 auto& p=*implementation;assert(!p.current && !p.resets && !p.configured);
 for(unsigned i=0;i<n;i++)p.WriteLocal(a[i],v[i]);
 for(unsigned w=0;w<120;w++){
  auto& c=*p.core[w];c.config=source[0];if(w)c.config&=~0x10000U;
  for(unsigned i=0;i<16;i++)c.source[i]=source[i];c.source[0]&=~0x10000U;
 }
 auto& c=p.C();for(unsigned i=0;i<10;i++)c.command[i]=command[i];
 c.counters[0]=counts[0];c.counters[1]=counts[1];c.accepted=counts[2];c.counters[3]=counts[3];
 c.writeCounters[0]=counts[4];c.writeCounters[1]=counts[5];c.writeCounters[3]=counts[7];
}
bool RecurrentPrefixNativeTransport::Select(unsigned w){return implementation->OwnerIO::SelectRuntimeCore(w);}
bool RecurrentPrefixNativeTransport::MapPanel(){auto& p=*implementation;p.Touch();return p.C().mapping;}
bool RecurrentPrefixNativeTransport::PanelStable(){return implementation->C().mapping;}
uint32_t RecurrentPrefixNativeTransport::Read32(Read kind,unsigned a,unsigned b,unsigned c){
 auto& p=*implementation;p.Touch();
 switch(kind){
  case Source:return p.core[0]->SourceRead(a);
  case Reset:{
   for(unsigned w=0;w<120;w++){
    bool same=true;for(unsigned i=0;i<3;i++)same&=p.C().tlb[1][i]==BlackholeDramFixedTlbWord(0xffa00000,PioneerWorkerX(w),PioneerWorkerY(w),true,i);
    if(same)return p.core[w]->coreReset;
   }return p.C().coreReset;
  }
  case ComputeNoc:return p.CohortIO::ComputeNocRead(a);
  case Tlb:return p.C().DramTlbRead(a,b);
  case Local:return p.ReadLocal(0x17e000+a*4);
  case Code:assert(!a);return p.ReadLocal(0x6000);
  case Control:return p.C().RuntimeControlRead(a);
  case Status:return p.ReadLocal(0x17ffc0+a*4);
  case Noc:{auto value=p.C().ResidentNocRead(a);
   if(p.AtReadback() && p.reading && ((p.readbackFault==2 && a==PioneerQ38PanelCounter(0)) || (p.readbackFault==3 && a==0x840))){
    p.ReadbackFaultHit();return p.readbackFault==2?value+2:1;
   }return value;
  }
  case Panel:return p.ReadLocal(a);
  case Pci:return p.C().HostPciRead(a,uint16_t(b),uint8_t(c));
  case Pc:return p.RuntimePcRead();
 }assert(false);return 0;
}
void RecurrentPrefixNativeTransport::Write32(Write kind,unsigned a,uint32_t value,unsigned b){
 auto& p=*implementation;p.Touch();
 switch(kind){
  case SetTlb:p.C().DramTlbWrite(a,b,value);return;
  case SetLocal:p.WriteLocal(0x17e000+a*4,value);return;
  case SetCode:assert(!a);p.WriteLocal(0x6000,value);return;
  case SetStatus:p.WriteLocal(0x17ffc0+a*4,value);return;
  case SetNoc:if(a==0x40){p.ResidentNocWrite(a,value);return;}p.C().ResidentNocWrite(a,value);return;
  case SetPanel:p.WriteLocal(a,value);return;
  case SetReset:p.resets++;p.RuntimeResetWrite(value);return;
 }assert(false);
}
uint32_t RecurrentPrefixNativeTransport::MappedRead(uint32_t a){auto& p=*implementation;p.Touch();return p.ReadLocal(a);}
void RecurrentPrefixNativeTransport::MappedWrite(uint32_t a,uint32_t value){auto& p=*implementation;p.Touch();p.WriteLocal(a,value);}
uint32_t* RecurrentPrefixNativeTransport::PanelSaved(){return implementation->C().Q38PanelSavedWords();}
uint32_t* RecurrentPrefixNativeTransport::Backup(unsigned n){return implementation->C().HostReadBackup(n);}
void RecurrentPrefixNativeTransport::Quarantine(){implementation->quarantined=true;}
void RecurrentPrefixNativeTransport::Pause(){implementation->now++;}
uint64_t RecurrentPrefixNativeTransport::Now()const{return implementation->now;}
unsigned RecurrentPrefixNativeTransport::Touches()const{return implementation->touches;}
unsigned RecurrentPrefixNativeTransport::Resets()const{return implementation->resets;}
uint64_t RecurrentPrefixNativeTransport::Fingerprint(){
 auto& p=*implementation;uint64_t hash=14695981039346656037ULL;
 for(unsigned w=0;w<120;w++){
  p.current=w;
  for(unsigned i=0;i<PioneerQ38RecurrentPrefixSnapshotWords(w);i++){
   hash^=p.PhysicalRead(w,PioneerQ38RecurrentPrefixSnapshotAddress(w,i));hash*=1099511628211ULL;
  }
 }p.current=0;return hash;
}
bool RecurrentPrefixNativeTransport::Configure(const PioneerQ38RecurrentPrefixPlan& operation,const uint32_t* input,unsigned words,const PrefixInitializedByteSource& bytes){
 auto& p=*implementation;auto& m=p.machine;const auto* first=operation.Layer(0);
 if(!first || words!=5120 || !input || p.configured || p.starts || p.execution || m.completedBlocks || !m.saved.empty())return false;
 const auto* o=first->Outer();if(!o || o[3] || o[4] || o[12] || o[13] || operation.Bank()>=7)return false;
 std::array<uint32_t,1920> sources;
 for(unsigned w=0;w<120;w++){
  const auto* boot=operation.Boot(w);if(!boot)return false;
  for(unsigned i=0;i<16;i++)sources[w*16+i]=boot[16+i];
 }
 auto wide=[](uint32_t lo,uint32_t hi){return uint64_t(lo)|(uint64_t(hi)<<32);};
 const auto* boot=operation.Boot(0);
 PioneerQ38GdnIdentity id={wide(boot[8],boot[9]),wide(boot[10],boot[11]),wide(boot[12],boot[13]),wide(boot[14],boot[15])};
 auto clone=std::make_unique<PioneerQ38RecurrentPrefixPlan>();
 if(!clone->Build(*m.canonical,operation.Bank(),id,sources.data()))return false;
 for(unsigned w=0;w<120;w++)for(unsigned i=0;i<64;i++)if(clone->Boot(w)[i]!=operation.Boot(w)[i])return false;
 for(unsigned layer=0;layer<3;layer++){
  const auto* actual=operation.Layer(layer);const auto* expected=clone->Layer(layer);if(!actual || !expected)return false;
  for(unsigned i=0;i<128;i++)if(actual->Outer()[i]!=expected->Outer()[i])return false;
  for(unsigned recipe=0;recipe<8;recipe++)for(unsigned i=0;i<1280;i++)if(actual->Recipe(recipe)[i]!=expected->Recipe(recipe)[i])return false;
  for(unsigned i=0;i<expected->Outer()[60];i++)if(actual->Conv()[i]!=expected->Conv()[i])return false;
  for(unsigned i=0;i<expected->Outer()[61];i++)if(actual->Gdn()[i]!=expected->Gdn()[i])return false;
 }
 // Read ONLY raw bytes actually produced by the native zero/QPE1 machines.
 // These reads transfer no source/clock/init/embedding/ownership capability.
 std::vector<uint32_t> state(8192);
 auto bank=[](unsigned route){for(unsigned b=0;b<7;b++)if(PioneerQ38ProjectionRoute(b)==route)return b;return 7U;};
 for(unsigned layer=0;layer<3;layer++){
  const auto& actual=*operation.Layer(layer);const auto* f=actual.Outer();
  for(unsigned w=0;w<96;w++){
   const auto* d=actual.Gdn()+w*192+64;
   for(unsigned slot=0;slot<2;slot++){
    unsigned route=slot?d[13]:d[12],address=slot?d[7]:d[5],selected=bank(route);
    if(selected>=7 || !bytes.Recurrent(selected,address,state.data(),8192))return false;
    for(unsigned i=0;i<8192;i++)m.Put(route,address+i*4,state[i]);
   }
  }
  uint32_t history[48];
  for(unsigned w=0;w<120;w++)for(unsigned group=0;group<PioneerQ38ConvBatchCount(w);group++){
   const auto* d=actual.Conv()+w*384+group*64;unsigned selected=bank(d[41]);
   if(selected>=7 || !bytes.History(selected,d[15],history,48))return false;
   for(unsigned i=0;i<48;i++)m.Put(d[41],d[15]+i*4,history[i]);
  }
  auto block=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();
  if(!block->Build(*m.canonical,layer,0,0,operation.Bank(),0,1001+layer*16,layer+1,id,sources.data(),f[68],f[69]))return false;
  if(layer)m.prepared[layer]=std::move(block);else m.block=std::move(block);
  uint32_t package;if(!operation.PackageAddress(layer,package))return false;
  for(unsigned i=0;i<XP::Bytes/4;i++)m.Put(f[8],package+i*4,0xa5a5a5a5);
 }
 for(unsigned i=0;i<5120;i++)m.Put(PioneerQ38ProjectionRoute(o[68]),o[69]+i*4,input[i]);
 for(unsigned i=0;i<128;i++)m.outer[i]=o[i];
 for(unsigned i=0;i<64;i++)m.boot[i]=operation.Boot(0)[i];
 for(unsigned i=0;i<16;i++)m.source[i]=sources[i];
 // No usable default metadata is supplied by this raw adapter. The real
 // native owner must stage all57 packets; ROOT must perform every copy.
 for(unsigned i=0;i<XP::RecipeBytes/4;i++)m.Put(o[8],o[57]+i*4,0xa5a5a5a5);
 for(unsigned i=0;i<XP::ConvBytes/4;i++)m.Put(o[8],o[58]+i*4,0xa5a5a5a5);
 for(unsigned i=0;i<XP::GdnBytes/4;i++)m.Put(o[8],o[59]+i*4,0xa5a5a5a5);
 p.operation=std::move(clone);p.configured=true;return true;
}
void RecurrentPrefixNativeTransport::Observe(const PioneerQ38RecurrentPrefixResult& result){
 auto& p=*implementation;assert(p.configured && !p.execution);p.execution=&result;
}
bool RecurrentPrefixNativeTransport::AttachOriginalLearned(RecurrentBlockRawLearnedBytes& bytes){
 auto& p=*implementation;return !p.configured && !p.starts && !p.execution && p.machine.Attach(bytes);
}
void RecurrentPrefixNativeTransport::EnableValidationReadback(unsigned fault){
 auto& p=*implementation;assert(fault<=3 && p.execution && p.execution->phase==5 && p.execution->completed==119 && p.execution->stopped==119
  && !p.execution->unsafe && !p.execution->held && !p.readback && !p.holds && p.machine.completedBlocks==3);
 p.readback=true;p.readbackFault=fault;
} // TEST endpoint selection only;actual production owner still proves all STOPs+holds
unsigned RecurrentPrefixNativeTransport::ReadbackFaultFence()const{return implementation->readbackFence;}
unsigned RecurrentPrefixNativeTransport::ReadbackPackets()const{return implementation->readPackets;}
void RecurrentPrefixNativeTransport::VerifyReadback(const uint32_t* words,unsigned count){
 auto& p=*implementation;assert(words && count==5120 && p.readback && !p.readbackFault && !p.readbackFence && p.readPackets==5
  && p.execution && p.execution->closed && !p.execution->unsafe && p.holds==120);
 const auto* o=p.operation->Layer(2)->Outer();
 for(unsigned i=0;i<count;i++)assert(words[i]==p.machine.Get(o[8],o[9]+RB::FinalOffset+i*4));
} // compare actual instruction bytes,NOT a CPU reference or physical-model proof
bool RecurrentPrefixNativeTransport::CaptureOriginalOutput(uint32_t* output,unsigned words){
 auto& p=*implementation;auto& m=p.machine;
 if(!output || words!=5120 || (uintptr_t(output)&3) || uintptr_t(output)>UINTPTR_MAX-20480
  || !m.original || !m.original->Ready() || !p.execution || !p.execution->closed || p.execution->unsafe
  || p.execution->held!=120 || p.execution->stopped!=119 || p.starts!=120 || p.holds!=120 || m.stopStarts!=119)return false;
 const auto& captured=m.captured[2][PioneerRecurrentFinal];if(captured.size()!=words)return false;
 std::array<uint32_t,5120> observed;
 for(unsigned i=0;i<words;i++)observed[i]=captured[i];
 if(!m.original->Ready())return false;
 std::memcpy(output,observed.data(),sizeof(observed));return true;
} // TEST CPU observation of already-produced bytes, NOT physical readback
bool RecurrentPrefixNativeTransport::PublishOriginalStages(PrefixOriginalStageSink& sink){
 auto& p=*implementation;auto& m=p.machine;
 if(!m.original || !m.original->Ready() || !p.execution || !p.execution->closed || p.execution->unsafe
  || p.holds!=120 || p.starts!=120 || m.stopStarts!=119 || m.completedBlocks!=3)return false;
 const unsigned words[]={5120,5120,48,48,6144,10240,10240,786432,6144,6144,5120,5120,5120,17408,17408,17408,5120,5120};
 for(unsigned layer=0;layer<3;layer++)for(unsigned stage=0;stage<PioneerRecurrentStageCount;stage++)
  if(m.captured[layer][stage].size()!=words[stage])return false;
 for(unsigned layer=0;layer<3;layer++)for(unsigned stage=0;stage<PioneerRecurrentStageCount;stage++)
  if(!m.original->Ready() || !sink.Publish(layer,PioneerRecurrentOriginalStage(stage),m.captured[layer][stage].data(),words[stage]))return false;
 return m.original->Ready();
} // caller invokes ONLY after genuine final-END/context destruction
void RecurrentPrefixNativeTransport::VerifyRun(){
 auto& p=*implementation;auto& m=p.machine;
 assert(p.starts==120 && !p.holds && m.completedBlocks==3 && m.selfReleases==3 && m.selfOuterWrites==384
  && m.pc==0x40060 && !m.unknown && !m.pending && m.stopJobs==119 && m.stopStarts==119 && m.memory.at(RB::Doorbell)==4);
 for(const auto& peer:m.child)assert(peer->pc==peer->stopPC && !peer->pending && peer->drained);
 for(const auto& saved:m.saved)saved.Verify(m);
} // pure observer: NEVER call VerifyStop, which deliberately changes Doorbell
void RecurrentPrefixNativeTransport::VerifyClosed(){
 auto& p=*implementation;assert(p.starts==120 && p.holds==120 && p.machine.stopStarts==119 && p.execution
  && p.execution->closed && !p.execution->unsafe && p.execution->stopped==119 && p.execution->held==120);
}
void RecurrentPrefixNativeTransport::Diagnostic(){
 auto& p=*implementation;fprintf(stderr,"QPX_NATIVE_MACHINE rootPC%08x starts%u holds%u blocks%u phase%u packets%u stops%u\n",
  p.machine.pc,p.starts,p.holds,p.machine.completedBlocks,p.machine.Phase(),p.machine.packets,p.machine.stopStarts);
} // already-owned CPU values ONLY, no device IO
