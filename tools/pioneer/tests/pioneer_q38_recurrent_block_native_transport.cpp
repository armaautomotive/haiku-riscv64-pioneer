/* SPDX-License-Identifier: MIT */
// TEST endpoint TU uses ordinary instruction/operand-model flags,NOT the
// native role. Distinct simulator class names prevent weak ODR collisions
// with the separately compiled zero/QPE1 bridge's different model layout.
#define Machine RecurrentNativeMachineBase
#define RuntimeMachine RecurrentNativeRuntimeMachine
#define BlockMachine RecurrentNativeBlockMachine
#define StreamMachine RecurrentNativeStreamMachine
#define ScalarMachine RecurrentNativeScalarMachine
#define PIONEER_Q38_RECURRENT_BLOCK_OWNER_ACTUAL_FIXTURE_ONLY
#include "pioneer_q38_recurrent_block_owner_actual_test.cpp"
#include "pioneer_q38_recurrent_block_native_transport.h"
struct RecurrentBlockNativeTransport::Impl:RecurrentOwnerActualIO {
 unsigned resets=0;bool configured=false;
 Impl(const char* root,const char* worker):RecurrentOwnerActualIO(root,worker){
  (void)&RecurrentOwnerUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;
  (void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&Refusals;(void)&LateRefusals;(void)&RunPersistentGdnPair;
  assert(CodeIO::SelectRuntimeCore(0));track=true;
 }
 uint32_t ReadLocal(uint32_t a){
  if(a==(current?unsigned(M::Status):unsigned(RB::Status)))Advance(current);
  if(PioneerQ38RecurrentBlockSnapshotContains(current,a))return PhysicalRead(current,a);
  if(!current && a>=0x64000 && a<0x6e000)return prefix.at(PrefixIndex(current,a));
  if(a>=TT_Q38_PANEL_VIEW_BASE && a<TT_Q38_PANEL_VIEW_BASE+TT_Q38_PANEL_VIEW_BYTES)return C().Q38PanelRead(a);
  assert(a>=0x17e000 && a<0x180000);return C().ResidentL1Read((a-0x17e000)/4);
 }
 void WriteLocal(uint32_t a,uint32_t value){
  if(PioneerQ38RecurrentBlockSnapshotContains(current,a)){PhysicalWrite(current,a,value);return;}
  if(!current && a>=0x64000 && a<0x6e000){prefix.at(PrefixIndex(current,a))=value;return;}
  if(a>=TT_Q38_PANEL_VIEW_BASE && a<TT_Q38_PANEL_VIEW_BASE+TT_Q38_PANEL_VIEW_BYTES){C().Q38PanelWrite(a,value);return;}
  assert(a>=0x17e000 && a<0x180000);C().ResidentL1Write((a-0x17e000)/4,value);
 }
};
RecurrentBlockNativeTransport::RecurrentBlockNativeTransport(const char* root,const char* worker):implementation(new Impl(root,worker)){}
RecurrentBlockNativeTransport::~RecurrentBlockNativeTransport(){delete implementation;}
void RecurrentBlockNativeTransport::SeedRoot(const uint32_t* a,const uint32_t* v,unsigned n,const uint32_t* source,const uint32_t* command,const uint32_t* counts){
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
bool RecurrentBlockNativeTransport::Select(unsigned w){return implementation->RecurrentOwnerActualIO::SelectRuntimeCore(w);}
bool RecurrentBlockNativeTransport::MapPanel(){auto& p=*implementation;p.Touch();return p.C().mapping;}
bool RecurrentBlockNativeTransport::PanelStable(){return implementation->C().mapping;}
uint32_t RecurrentBlockNativeTransport::Read32(Read kind,unsigned a,unsigned b,unsigned c){
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
  case Noc:return p.C().ResidentNocRead(a);
  case Panel:return p.ReadLocal(a);
  case Pci:return p.C().HostPciRead(a,uint16_t(b),uint8_t(c));
  case Pc:return p.RuntimePcRead();
 }assert(false);return 0;
}
void RecurrentBlockNativeTransport::Write32(Write kind,unsigned a,uint32_t value,unsigned b){
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
uint32_t RecurrentBlockNativeTransport::MappedRead(uint32_t a){auto& p=*implementation;p.Touch();return p.ReadLocal(a);}
void RecurrentBlockNativeTransport::MappedWrite(uint32_t a,uint32_t value){auto& p=*implementation;p.Touch();p.WriteLocal(a,value);}
uint32_t* RecurrentBlockNativeTransport::PanelSaved(){return implementation->C().Q38PanelSavedWords();}
uint32_t* RecurrentBlockNativeTransport::Backup(unsigned n){return implementation->C().HostReadBackup(n);}
void RecurrentBlockNativeTransport::Quarantine(){implementation->quarantined=true;}
void RecurrentBlockNativeTransport::Pause(){implementation->now++;}
uint64_t RecurrentBlockNativeTransport::Now()const{return implementation->now;}
unsigned RecurrentBlockNativeTransport::Touches()const{return implementation->touches;}
unsigned RecurrentBlockNativeTransport::Resets()const{return implementation->resets;}
uint64_t RecurrentBlockNativeTransport::Fingerprint(){
 auto& p=*implementation;uint64_t hash=14695981039346656037ULL;
 for(unsigned w=0;w<120;w++){
  p.current=w;
  for(unsigned i=0;i<PioneerQ38RecurrentBlockSnapshotWords(w);i++){
   hash^=p.PhysicalRead(w,PioneerQ38RecurrentBlockSnapshotAddress(w,i));hash*=1099511628211ULL;
  }
 }p.current=0;return hash;
}
bool RecurrentBlockNativeTransport::Configure(const PioneerQ38MixedRecurrentBlockPlan& operation,const uint32_t* input,unsigned words,const RecurrentInitializedByteSource& bytes){
 auto& p=*implementation;auto& m=p.machine;const auto* o=operation.Outer();
 if(!o || words!=5120 || !input || p.configured || p.starts || p.execution || o[3] || o[4] || o[7] || o[12] || o[13])return false;
 std::array<uint32_t,1920> sources;
 for(unsigned w=0;w<120;w++)for(unsigned i=0;i<16;i++)sources[w*16+i]=operation.Boot(w)[16+i];
 auto wide=[](uint32_t lo,uint32_t hi){return uint64_t(lo)|(uint64_t(hi)<<32);};
 PioneerQ38GdnIdentity id={wide(o[32],o[33]),wide(o[34],o[35]),wide(o[36],o[37]),wide(o[38],o[39])};
 auto clone=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();
 if(!clone->Build(*m.canonical,o[3],o[4],o[13],o[7],o[12],o[6],o[48],id,sources.data(),o[68],o[69]))return false;
 for(unsigned i=0;i<128;i++)if(clone->Outer()[i]!=o[i])return false;
 for(unsigned w=0;w<120;w++)for(unsigned i=0;i<64;i++)if(clone->Boot(w)[i]!=operation.Boot(w)[i])return false;
 for(unsigned s=0;s<8;s++)for(unsigned i=0;i<1280;i++)if(clone->Recipe(s)[i]!=operation.Recipe(s)[i])return false;
 for(unsigned i=0;i<o[60];i++)if(clone->Conv()[i]!=operation.Conv()[i])return false;
 for(unsigned i=0;i<o[61];i++)if(clone->Gdn()[i]!=operation.Gdn()[i])return false;
 // Actual QPE1 output and actual zero-machine output bytes,not products
 // computed by a host oracle or inferred from an initialized ready bit.
 for(unsigned i=0;i<5120;i++)m.Put(PioneerQ38ProjectionRoute(o[68]),o[69]+i*4,input[i]);
 std::vector<uint32_t> state(8192);
 for(unsigned w=0;w<96;w++){
  const auto* d=operation.Gdn()+w*192+64;
  for(unsigned slot=0;slot<2;slot++){
   unsigned route=slot?d[13]:d[12],address=slot?d[7]:d[5],bank=7;
   for(unsigned b=0;b<7;b++)if(PioneerQ38ProjectionRoute(b)==route)bank=b;
   if(bank>=7 || !bytes.Recurrent(bank,address,state.data(),8192))return false;
   for(unsigned i=0;i<8192;i++)m.Put(route,address+i*4,state[i]);
  }
 }
 uint32_t history[48];
 for(unsigned w=0;w<120;w++)for(unsigned group=0;group<PioneerQ38ConvBatchCount(w);group++){
  const auto* d=operation.Conv()+w*384+group*64;unsigned bank=7;
  for(unsigned b=0;b<7;b++)if(PioneerQ38ProjectionRoute(b)==d[41])bank=b;
  if(bank>=7 || !bytes.History(bank,d[15],history,48))return false;
  for(unsigned i=0;i<48;i++)m.Put(d[41],d[15]+i*4,history[i]);
 }
 m.block=std::move(clone);for(unsigned i=0;i<128;i++)m.outer[i]=o[i];
 for(unsigned i=0;i<64;i++)m.boot[i]=operation.Boot(0)[i];
 for(unsigned i=0;i<16;i++)m.source[i]=sources[i];
 p.configured=true;return true;
}
void RecurrentBlockNativeTransport::Observe(const PioneerQ38RecurrentBlockResult& result){auto& p=*implementation;assert(p.configured && !p.execution);p.execution=&result;}
void RecurrentBlockNativeTransport::VerifyRun(){implementation->machine.VerifyBlock();}
void RecurrentBlockNativeTransport::VerifyClosed(){auto& p=*implementation;assert(p.starts==120 && p.holds==120 && p.machine.stopStarts==119);}
void RecurrentBlockNativeTransport::Diagnostic(){auto& p=*implementation;
 fprintf(stderr,"QRB_NATIVE_MACHINE rootPC%08x starts%u holds%u matrices%u phase%u packets%u stops%u\n",p.machine.pc,p.starts,p.holds,p.machine.matrixStarts,p.machine.Phase(),p.machine.packets,p.machine.stopStarts);
}
