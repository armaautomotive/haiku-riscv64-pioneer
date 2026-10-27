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
#include "../PioneerQ38RecurrentOriginalWeightBytes.h"
#include "../PioneerQ38RecurrentBlockComparison.h"
struct NativeRecurrentLearnedMachine:RecurrentBlockMachine {
 RecurrentBlockRawLearnedBytes* original=nullptr;
 std::array<std::vector<uint32_t>,PioneerRecurrentStageCount> captured;
 NativeRecurrentLearnedMachine(const char* root,const char* worker,bool virgin):RecurrentBlockMachine(root,worker,virgin){}
 bool Attach(RecurrentBlockRawLearnedBytes& bytes){
  if(original || !bytes.Ready())return false;
  unsigned spans=0;uint64_t length=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);if(p->kind!=canonical->WeightKind || p->layer)continue;
   if(!bytes.Contains(PioneerQ38ProjectionRoute(p->bank),canonical->Base+p->offset,p->bytes))return false;
   spans++;length+=p->bytes;
  }
  if(spans!=56 || length!=280925056 || !bytes.Ready())return false;
  // Remove ALL synthetic F32/Conv/GDN learned words. Matrix fallback is
  // also disabled in Get below,including a missing original span.
  for(auto i=dram.begin();i!=dram.end();){
   if(bytes.Contains(unsigned(i->first>>32),unsigned(i->first),4))i=dram.erase(i);else ++i;
  }
  original=&bytes;return true;
 }
 uint32_t Get(unsigned route,unsigned address)const override{
  if(!original)return RecurrentBlockMachine::Get(route,address);
  if(original->Contains(route,address,4)){uint32_t value;assert(original->Word(route,address,value));return value;}
  auto raw=dram.find(Key(route,address));assert(raw!=dram.end());return raw->second;
 }
 void Submit()override{
  if(original)assert(original->BeginRawPacket());
  RecurrentBlockMachine::Submit();
  if(original)assert(original->EndRawPacket());
 }
 void LocalCapture(PioneerRecurrentOriginalStage stage,unsigned address,unsigned words){
  auto& out=captured[stage];assert(out.empty());out.resize(words);for(unsigned i=0;i<words;i++)out[i]=memory.at(address+i*4);
 }
 void RemoteCapture(PioneerRecurrentOriginalStage stage,unsigned role,unsigned words){
  auto& out=captured[stage];assert(out.empty());out.resize(words);for(unsigned i=0;i<words;i++)out[i]=Get(outer[8],outer[9]+PioneerQ38MixedRecurrentOffset(role)+i*4);
 }
 void ObserveMatrix()override{
  bool fresh=Phase()!=observed;RecurrentBlockMachine::ObserveMatrix();if(!original || !fresh)return;
  switch(Phase()){
   case 2:LocalCapture(PioneerRecurrentEmbedding,RB::OriginalCopy,5120);LocalCapture(PioneerRecurrentNormalized,RB::Input,5120);break;
   case 8:{
    RemoteCapture(PioneerRecurrentAlpha,10,48);RemoteCapture(PioneerRecurrentBeta,11,48);RemoteCapture(PioneerRecurrentZ,5,6144);RemoteCapture(PioneerRecurrentQkv,6,10240);
    auto& conv=captured[PioneerRecurrentConv];conv.resize(10240,0x7fc00000);
    for(unsigned w=0;w<120;w++)for(unsigned batch=0;batch<PioneerQ38ConvBatchCount(w);batch++){
     const auto* d=block->Conv()+w*384+batch*64;for(unsigned i=0;i<16;i++)conv.at(d[7]+i)=Get(d[8],d[4]+d[43]+i*4);
    }
    auto& state=captured[PioneerRecurrentState];state.resize(786432,0x7fc00000);
    auto& raw=captured[PioneerRecurrentRaw];raw.resize(6144,0x7fc00000);
    for(unsigned w=0;w<96;w++){
     const auto* d=block->Gdn()+w*192+64;unsigned head=w/2,half=w%2;
     for(unsigned key=0;key<128;key++)for(unsigned value=0;value<64;value++)state[head*16384+key*128+half*64+value]=Get(d[13],d[7]+(key*64+value)*4);
     for(unsigned i=0;i<64;i++)raw[head*128+half*64+i]=child[w]->memory.at(0x8e800+i*4);
    }
    auto& gated=captured[PioneerRecurrentGated];gated.resize(6144);for(unsigned i=0;i<6144;i++)gated[i]=Get(outer[8],outer[9]+TT_Q38_GDN_GATED_OUTPUT+i*4);
    break;
   }
   case 10:RemoteCapture(PioneerRecurrentProjection,13,5120);LocalCapture(PioneerRecurrentResidual,RB::PostResidual,5120);LocalCapture(PioneerRecurrentPostNormalized,RB::PostNormalized,5120);break;
   case 13:RemoteCapture(PioneerRecurrentFfnGate,2,17408);RemoteCapture(PioneerRecurrentFfnUp,3,17408);LocalCapture(PioneerRecurrentActivated,RB::Input,17408);break;
  }
 } // read ONLY genuinely produced instruction bytes;NEVER supplies products to the owner
 void CaptureFinal(){
  if(!original)return;RemoteCapture(PioneerRecurrentDown,4,5120);auto& out=captured[PioneerRecurrentFinal];assert(out.empty());out.resize(5120);
  for(unsigned i=0;i<5120;i++)out[i]=Get(outer[8],outer[9]+RB::FinalOffset+i*4);
 }
};
struct RecurrentBlockNativeTransport::Impl:RecurrentOwnerActualIOBase<NativeRecurrentLearnedMachine> {
 using OwnerIO=RecurrentOwnerActualIOBase<NativeRecurrentLearnedMachine>;
 unsigned resets=0;bool configured=false,validationReadback=false;
 unsigned readbackPackets=0,readbackFault=0,readbackFaultFence=0;
 PioneerQ38RecurrentBlockResult closedObservation;
 bool AtValidationReadback()const{return validationReadback && execution && execution->phase==7 && holds==120;}
 bool ReadbackScratch(uint32_t a)const{return a>=0x80000 && a<0x81000;}
 void ValidationReadbackPacket(uint32_t value){
  assert(AtValidationReadback() && !current && value==1 && readbackPackets<5 && execution->stopped==119 && execution->held==120);
  for(unsigned w=0;w<120;w++)assert(core[w]->coreReset==0x47800);
  const auto* o=machine.block->Outer();const unsigned expected[]={o[9]+RB::FinalOffset+readbackPackets*4096,0,o[8],0x80000,0,129,0,0,4096,0};
  for(unsigned i=0;i<10;i++)assert(C().command[i]==expected[i]);
  // The endpoint copies ONLY actual emitted-instruction output bytes. No
  // CPU arithmetic/reference products or native ready/STOP/hold setters.
  if(readbackFault!=1)for(unsigned i=0;i<1024;i++)PhysicalWrite(0,0x80000+i*4,machine.Get(o[8],expected[0]+i*4));
  auto& c=C();c.counters[0]+=readbackFault==2?2:1;c.counters[1]+=64;c.counters[3]++;c.accepted++;readbackPackets++;
 }
 Impl(const char* root,const char* worker):OwnerIO(root,worker){
  (void)&RecurrentOwnerUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;
  (void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&Refusals;(void)&LateRefusals;(void)&RunPersistentGdnPair;
  assert(CodeIO::SelectRuntimeCore(0));track=true;
 }
 uint32_t ReadLocal(uint32_t a){
  if(a==(current?unsigned(M::Status):unsigned(RB::Status)))Advance(current);
  if(PioneerQ38RecurrentBlockSnapshotContains(current,a)){
   auto value=PhysicalRead(current,a);
   if(AtValidationReadback() && readbackPackets && readbackFault==1 && ReadbackScratch(a))readbackFaultFence=touches;
   return value;
  }
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
bool RecurrentBlockNativeTransport::Select(unsigned w){return implementation->OwnerIO::SelectRuntimeCore(w);}
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
  case Noc:{
   auto value=p.C().ResidentNocRead(a);
   if(p.AtValidationReadback() && p.readbackPackets){
    if(p.readbackFault==2 && a==PioneerQ38PanelCounter(0))p.readbackFaultFence=p.touches;
    if(p.readbackFault==3 && a==0x840){p.readbackFaultFence=p.touches;return 1;}
   }return value;
  }
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
  case SetNoc:if(a==0x40){
   if(p.AtValidationReadback() && !p.C().command[7])p.ValidationReadbackPacket(value);
   else p.ResidentNocWrite(a,value);return;
  }p.C().ResidentNocWrite(a,value);return;
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
void RecurrentBlockNativeTransport::VerifyRun(){implementation->machine.VerifyBlock();implementation->machine.CaptureFinal();}
void RecurrentBlockNativeTransport::VerifyClosed(){auto& p=*implementation;assert(p.starts==120 && p.holds==120 && p.machine.stopStarts==119);}
bool RecurrentBlockNativeTransport::AttachOriginalLearned(RecurrentBlockRawLearnedBytes& bytes){
 auto& p=*implementation;return !p.configured && !p.starts && !p.execution && p.machine.Attach(bytes);
}
bool RecurrentBlockNativeTransport::CaptureOriginalOutput(uint32_t* output,unsigned words){
 auto& p=*implementation;auto& m=p.machine;
 if(!output || words!=5120 || (uintptr_t(output)&3) || uintptr_t(output)>UINTPTR_MAX-20480 || !m.original || !m.original->Ready()
  || !p.execution || !p.execution->closed || p.execution->unsafe || p.execution->held!=120 || p.execution->stopped!=119
  || p.starts!=120 || p.holds!=120 || m.stopStarts!=119 || !m.block)return false;
 std::array<uint32_t,5120> captured;const auto* o=m.block->Outer();
 for(unsigned i=0;i<5120;i++){
  auto found=m.dram.find(m.Key(o[8],o[9]+RB::FinalOffset+i*4));if(found==m.dram.end())return false;captured[i]=found->second;
 }
 if(!m.original->Ready())return false;
 std::memcpy(output,captured.data(),sizeof(captured));return true;
} // simulated raw GDDR capture AFTER real native STOP/holds/restore,NOT physical DMA
bool RecurrentBlockNativeTransport::PublishOriginalStages(RecurrentOriginalStageSink& sink){
 auto& p=*implementation;auto& m=p.machine;
 if(!m.original || !m.original->Ready() || !p.execution || !p.execution->closed || p.execution->unsafe || p.holds!=120 || p.starts!=120 || m.stopStarts!=119)return false;
 const unsigned counts[]={5120,5120,48,48,6144,10240,10240,786432,6144,6144,5120,5120,5120,17408,17408,17408,5120,5120};
 for(unsigned i=0;i<PioneerRecurrentStageCount;i++)if(m.captured[i].size()!=counts[i])return false;
 for(unsigned i=0;i<PioneerRecurrentStageCount;i++)if(!m.original->Ready() || !sink.Publish(PioneerRecurrentOriginalStage(i),m.captured[i].data(),unsigned(m.captured[i].size())))return false;
 return m.original->Ready();
} // native scenario invokes ONLY after successful full final-END/context destruction
void RecurrentBlockNativeTransport::EnableValidationReadback(){auto& p=*implementation;
 assert(!p.configured && !p.execution && !p.starts && !p.validationReadback);p.validationReadback=true;
}
void RecurrentBlockNativeTransport::FailValidationReadback(unsigned fault){auto& p=*implementation;
 assert(p.validationReadback && p.execution && p.execution->phase==5 && !p.holds && !p.readbackPackets && !p.readbackFault && fault>=1 && fault<=3);p.readbackFault=fault;
}
unsigned RecurrentBlockNativeTransport::ValidationReadbackPackets()const{return implementation->readbackPackets;}
bool RecurrentBlockNativeTransport::ValidationReadbackFaultObserved()const{const auto& p=*implementation;
 return p.readbackFault && p.readbackFaultFence && p.readbackFaultFence==p.touches;
}
void RecurrentBlockNativeTransport::VerifyValidationReadback(const uint32_t* values,unsigned words){auto& p=*implementation;
 assert(p.validationReadback && !p.readbackFault && p.readbackPackets==5 && values && words==5120 && p.execution && p.execution->closed
  && !p.execution->unsafe && p.execution->held==120 && p.execution->stopped==119 && p.holds==120 && p.machine.stopStarts==119);
 const auto* o=p.machine.block->Outer();for(unsigned i=0;i<words;i++)assert(values[i]==p.machine.Get(o[8],o[9]+RB::FinalOffset+i*4));
} // TEST assertion only,never publishes a CPU result into execution
void RecurrentBlockNativeTransport::PreserveClosedValidationObservation(const uint32_t* values,unsigned words){auto& p=*implementation;
 assert(p.execution && p.execution!=&p.closedObservation);
 VerifyValidationReadback(values,words);
 // The actual handler owns and deletes its temporary result after the
 // final user copy. Preserve its ALREADY CLOSED result verbatim while
 // alive; never leave a borrowed pointer for post-return/final-END tests.
 // This copy is private to the raw endpoint observer,not a native owner,
 // producer grant,phase setter or result supplied to production code.
 p.closedObservation=*p.execution;p.execution=&p.closedObservation;
}
void RecurrentBlockNativeTransport::Diagnostic(){auto& p=*implementation;
 fprintf(stderr,"QRB_NATIVE_MACHINE rootPC%08x starts%u holds%u matrices%u phase%u packets%u stops%u\n",p.machine.pc,p.starts,p.holds,p.machine.matrixStarts,p.machine.Phase(),p.machine.packets,p.machine.stopStarts);
}
