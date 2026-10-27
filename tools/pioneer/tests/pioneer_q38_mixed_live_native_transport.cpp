/* SPDX-License-Identifier: MIT */
// These TEST instruction classes also occur in the zero/QPE1 bridge TU,
// built with a different operand simulator configuration. Distinct names
// prevent weak constructor/vtable coalescing across incompatible layouts;
// production types, program DATA and all native permissions stay unchanged.
#define Machine MixedLiveNativeMachineBase
#define RuntimeMachine MixedLiveNativeRuntimeMachine
#define BlockMachine MixedLiveNativeBlockMachine
#define StreamMachine MixedLiveNativeStreamMachine
#define ScalarMachine MixedLiveNativeScalarMachine
#define PIONEER_Q38_MIXED_LIVE_OWNER_ACTUAL_FIXTURE_ONLY
#include "pioneer_q38_mixed_live_owner_actual_test.cpp"
#include "pioneer_q38_mixed_live_native_transport.h"
struct MixedLiveNativeTransport::Impl:LiveActualIO {
 unsigned resets=0;
 Impl(const char* root,const char* worker,const char* qdp,const char* qda):LiveActualIO(root,worker,qdp,qda){
  (void)&LiveActualUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;
  (void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
  (void)&BlockInitialRefusals;(void)&BlockLaterRefusals;(void)&JoinedRefusals;(void)&ProjectionRefusals;
  (void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;
  assert(CodeIO::SelectRuntimeCore(0));track=true;
 }
 uint32_t ReadLocal(uint32_t a){
  if(a==(current?unsigned(M::Status):unsigned(T::Status)))Advance(current);
  if(PioneerQ38MixedLiveSnapshotContains(current,a))return PhysicalRead(current,a);
  if(!current && a>=0x64000 && a<0x6e000)return prefix.at(PrefixIndex(current,a));
  if(a>=TT_Q38_PANEL_VIEW_BASE && a<TT_Q38_PANEL_VIEW_BASE+TT_Q38_PANEL_VIEW_BYTES)return C().Q38PanelRead(a);
  assert(a>=0x17e000 && a<0x180000);return C().ResidentL1Read((a-0x17e000)/4);
 }
 void WriteLocal(uint32_t a,uint32_t v){
  if(PioneerQ38MixedLiveSnapshotContains(current,a)){PhysicalWrite(current,a,v);return;}
  if(!current && a>=0x64000 && a<0x6e000){prefix.at(PrefixIndex(current,a))=v;return;}
  if(a>=TT_Q38_PANEL_VIEW_BASE && a<TT_Q38_PANEL_VIEW_BASE+TT_Q38_PANEL_VIEW_BYTES){C().Q38PanelWrite(a,v);return;}
  assert(a>=0x17e000 && a<0x180000);C().ResidentL1Write((a-0x17e000)/4,v);
 }
};
MixedLiveNativeTransport::MixedLiveNativeTransport(const char* root,const char* worker,const char* qdp,const char* qda)
 :implementation(new Impl(root,worker,qdp,qda)){}
MixedLiveNativeTransport::~MixedLiveNativeTransport(){delete implementation;}
void MixedLiveNativeTransport::SeedRoot(const uint32_t* a,const uint32_t* v,unsigned n,
 const uint32_t* source,const uint32_t* command,const uint32_t* counts){
 auto& p=*implementation;assert(!p.current && !p.resets);
 for(unsigned i=0;i<n;i++)p.WriteLocal(a[i],v[i]);
 for(unsigned w=0;w<120;w++){
  auto& c=*p.core[w];c.config=source[0];if(w)c.config&=~0x10000U;
  for(unsigned i=0;i<16;i++)c.source[i]=source[i];c.source[0]&=~0x10000U;
 }
 auto& c=p.C();for(unsigned i=0;i<10;i++)c.command[i]=command[i];
 c.counters[0]=counts[0];c.counters[1]=counts[1];c.accepted=counts[2];c.counters[3]=counts[3];
 c.writeCounters[0]=counts[4];c.writeCounters[1]=counts[5];c.writeCounters[3]=counts[7];
}
bool MixedLiveNativeTransport::Select(unsigned w){return implementation->LiveActualIO::SelectRuntimeCore(w);}
bool MixedLiveNativeTransport::MapPanel(){auto& p=*implementation;p.Touch();return p.C().mapping;}
bool MixedLiveNativeTransport::PanelStable(){return implementation->C().mapping;}
uint32_t MixedLiveNativeTransport::Read32(Read kind,unsigned a,unsigned b,unsigned c){
 auto& p=*implementation;p.Touch();
 switch(kind){
  case Source:return p.core[0]->SourceRead(a); // global fixed DRAM source, NOT selected worker NIU
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
  case Noc:return p.C().ResidentNocRead(a);
  case Panel:return p.ReadLocal(a);
  case Pci:return p.C().HostPciRead(a,uint16_t(b),uint8_t(c));
  case Pc:return p.RuntimePcRead();
 }assert(false);return 0;
}
void MixedLiveNativeTransport::Write32(Write kind,unsigned a,uint32_t v,unsigned b){
 auto& p=*implementation;p.Touch();
 switch(kind){
  case SetTlb:p.C().DramTlbWrite(a,b,v);return;
  case SetLocal:p.WriteLocal(0x17e000+a*4,v);return;
  case SetCode:assert(!a);p.WriteLocal(0x6000,v);return;
  case SetStatus:p.WriteLocal(0x17ffc0+a*4,v);return;
  case SetNoc:if(a==0x40){p.ResidentNocWrite(a,v);return;}p.C().ResidentNocWrite(a,v);return;
  case SetPanel:p.WriteLocal(a,v);return;
  case SetReset:p.resets++;p.RuntimeResetWrite(v);return;
 }assert(false);
}
uint32_t MixedLiveNativeTransport::MappedRead(uint32_t a){auto& p=*implementation;p.Touch();return p.ReadLocal(a);}
void MixedLiveNativeTransport::MappedWrite(uint32_t a,uint32_t v){auto& p=*implementation;p.Touch();p.WriteLocal(a,v);}
uint32_t* MixedLiveNativeTransport::PanelSaved(){return implementation->C().Q38PanelSavedWords();}
uint32_t* MixedLiveNativeTransport::Backup(unsigned n){return implementation->C().HostReadBackup(n);}
void MixedLiveNativeTransport::Quarantine(){implementation->quarantined=true;}
void MixedLiveNativeTransport::Pause(){implementation->now++;}
uint64_t MixedLiveNativeTransport::Now()const{return implementation->now;}
unsigned MixedLiveNativeTransport::Touches()const{return implementation->touches;}
unsigned MixedLiveNativeTransport::Resets()const{return implementation->resets;}
uint64_t MixedLiveNativeTransport::Fingerprint(){
 auto& p=*implementation;uint64_t hash=14695981039346656037ULL;
 // Logical L1 byte state only. Source/NoC controls and peer leases are
 // separately audited by actual DispatchClose; TLB windows are selections.
 for(unsigned w=0;w<120;w++){
  p.current=w;
  for(unsigned i=0;i<PioneerQ38MixedLiveSnapshotWords(w);i++){
   hash^=p.PhysicalRead(w,PioneerQ38MixedLiveSnapshotAddress(w,i));hash*=1099511628211ULL;
  }
 }p.current=0;return hash;
}
void MixedLiveNativeTransport::Configure(const PioneerQ38MixedLiveBlockPlan& plan,const uint32_t* input,unsigned words){
 auto& p=*implementation;auto& m=p.machine;assert(words==5120 && input && !p.starts && !p.execution && plan.Outer());
 // Observer command geometry follows actual native immutable DATA. Only
 // the real completed QPE1 GDDR output is copied as raw input; expected
 // norm/attention/FFN products are NEVER uploaded. The actual host owner
 // still installs/reads back every boot, recipe, frame and runtime word.
 for(unsigned i=0;i<128;i++)m.blockOuter[i]=plan.Outer()[i];
 for(unsigned i=0;i<64;i++)m.outer[i]=plan.Outer()[i];
 assert(m.sequence==plan.Outer()[48] && !m.projections && !m.inputObserved);
 for(unsigned i=0;i<5120;i++){
  unsigned magnitude=input[i]&0x7fffffff;assert(magnitude<=0x477fe000 && (!magnitude || magnitude>=0x26800000));
  m.wantedInput[i]=input[i];m.dram[CanonicalLiveMachine::Key(PioneerQ38ProjectionRoute(plan.Outer()[73]),plan.Outer()[74]+i*4)]=input[i];
 }
}
#ifdef PIONEER_Q38_MIXED_LIVE_ORIGINAL_NATIVE_TEST
bool MixedLiveNativeTransport::AttachOriginalLearned(ProjectionExecutionLearnedBytes& bytes){
 auto& p=*implementation;auto& m=p.machine;
 if(!bytes.Ready() || p.starts || p.execution || m.originalLearned)return false;
 struct Word{uint64_t key;uint32_t value;};std::vector<Word> staged;unsigned norms=0;
 for(unsigned i=0;i<m.canonical->Count();i++){
  const auto* part=m.canonical->Part(i);
  if(part->kind!=m.canonical->WeightKind || part->layer!=3 || part->type)continue;
  if(part->rows!=1 || (part->bytes!=20480 && part->bytes!=1024))return false;
  unsigned route=PioneerQ38ProjectionRoute(part->bank),base=m.canonical->Base+part->offset;
  std::vector<uint8_t> raw(part->bytes);
  if(!bytes.Ready() || !bytes.Read(route,base,raw.data(),unsigned(raw.size())) || !bytes.Ready())return false;
  for(unsigned at=0;at<raw.size();at+=4){
   uint32_t value=uint32_t(raw[at])|(uint32_t(raw[at+1])<<8)|(uint32_t(raw[at+2])<<16)|(uint32_t(raw[at+3])<<24);
   staged.push_back({CanonicalLiveMachine::Key(route,base+at),value});
  }
  norms++;
 }
 if(norms!=4 || staged.size()!=10752 || !bytes.Ready())return false;
 for(const auto& word:staged)m.dram[word.key]=word.value;
 m.originalLearned=&bytes;return true;
} // ORIGINAL norm/matrix raw endpoints;no expected products/private/native phase is published
#endif
void MixedLiveNativeTransport::Observe(const PioneerQ38MixedLiveResult& result){auto& p=*implementation;assert(!p.execution);p.execution=&result;}
void MixedLiveNativeTransport::VerifyRun(){implementation->machine.VerifyLive();}
void MixedLiveNativeTransport::VerifyClosed(){auto& p=*implementation;assert(p.starts==120 && p.holds==120 && p.machine.stops==119);}
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK
void MixedLiveNativeTransport::VerifyReadback(const uint32_t* values,unsigned words){
 auto& p=*implementation;assert(values && words==5120 && p.readbackPackets==5 && p.execution && p.execution->closed && p.holds==120);
 const auto& o=p.machine.blockOuter;
 for(unsigned i=0;i<words;i++)assert(values[i]==p.machine.dram.at(CanonicalLiveMachine::Key(o[8],o[9]+T::FinalOffset+i*4)));
} // no expected products supplied to the owner before/during execution
#endif
void MixedLiveNativeTransport::Diagnostic(){auto& p=*implementation;
 fprintf(stderr,"NATIVE_MACHINE rootPC%08x starts%u holds%u projections%u phase%u packets%u stops%u\n",p.machine.pc,p.starts,p.holds,p.machine.projections,p.machine.Phase(),p.machine.packets,p.machine.stops);
}
