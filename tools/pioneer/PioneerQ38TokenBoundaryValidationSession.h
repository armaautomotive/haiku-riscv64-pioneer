/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT) || PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT != 1
#error Exact separate observed-boundary diagnostic client required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC) || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_BENCHMARK_HOST)
#error Observed-boundary client cannot alias kernel,native,old USER,CPU oracle,Work or changed source roles,including defined0
#endif
#ifndef PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_SESSION_H
#define PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_SESSION_H
#include "PioneerQ38PublicModelSource.h"
#include "PioneerQ38TokenBoundarySnapshot.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenHiddenValidationProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <array>
#include <memory>

// Actual fixed-public uploader and diagnostic lifecycle. ALL learned parts,
// including embeddings/FP32 metadata, are uploaded, never mutable state or
// CPU model products. The native backend owns initialization and execution.
// IO/Source substitution exists for explicit OFFLINE transport fixtures only.
// Neither a successful fixture nor these returned DATA records prove model
// numerics, native lifetime/stack safety, physical speed or Work readiness.
template<class IO,class Source=PioneerQ38PublicModelSource>
class PioneerQ38TokenBoundaryValidationSession {
public:
 struct PendingObservation {uint32_t word=0,firstFrontend=0,secondFrontend=0;};
private:
 using Packet=PioneerQ38TokenControl::Packet;
 enum State {Virgin,Running,Terminal,Held,Drained,Complete,Failed};
 IO& io;Source& source;PioneerQwen38PackedPlan layout;
 std::unique_ptr<Packet> packet=std::make_unique<Packet>();
 std::array<uint32_t,3*64> records={};
 std::unique_ptr<PioneerQ38TokenHiddenValidation::Packet> hiddenPacket=std::make_unique<PioneerQ38TokenHiddenValidation::Packet>();
 std::unique_ptr<std::array<uint32_t,5120>> hidden=std::make_unique<std::array<uint32_t,5120>>();
 bool captured=false;
 State state=Virgin;int fd=-1;uint64_t epoch=0,generation=0;
 bool attempted=false,prepared=false,preflightAttempted=false,cancelAttempted=false,cancelled=false;
 unsigned committed=0,completedLayers=0,position=0,terminalStage=0;
 PendingObservation pendingObservation={};bool pendingObservationValid=false;
 bool Current()const{
  return epoch && source.Ready() && source.Model().Epoch()==epoch && layout.MatchesSource(source.Model());
 }
 void Close(){if(fd>=0){int owned=fd;fd=-1;io.Close(owned);}}
 bool Stop(const char* why){
  if(!failure)failure=why;
  state=Failed;records.fill(0);hidden->fill(0);captured=false;
  pendingObservation={};pendingObservationValid=false;
  std::memset(hiddenPacket.get(),0,sizeof(*hiddenPacket));Close();return false;
 } // CLOSE ONLY,never automatic retry/cancel/END/reset after uncertainty
 template<class T>bool Call(unsigned op,T& value){
  operation=op;
  return state!=Failed && fd>=0 && Current() && io.Call(fd,op,&value,sizeof(value)) && Current();
 }
 bool DriverIdentity(unsigned owner){
  tt_pioneer_driver_reload r={};r.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,r) || !PioneerDriverReloadRequest(r) || r.expected_tag || r.expected_generation
   || r.driver!=55 || r.guard_version!=1 || r.build_tag!=PioneerQ38TokenHiddenValidation::BuildTag
   || !r.generation || (generation && r.generation!=generation) || r.quiesced || r.cold
   || r.active_owner!=owner || r.open_count!=1)return false;
  generation=r.generation;return true;
 }
 bool TokenCall(unsigned op,unsigned count=0){
  namespace P=PioneerQ38TokenControl;namespace V=PioneerQ38TokenHiddenValidation;
  pendingObservationValid=false;
  std::memset(packet.get(),0,sizeof(*packet));auto& q=packet->request;q.magic=P::Magic;q.version=P::Version;q.operation=op;q.records=count;
  if(op==P::Start){q.prompts=V::Prompts;q.limit=V::Positions;packet->prompt[0]=V::FirstToken;packet->prompt[1]=V::SecondToken;}
  if(!V::ControlRequestShape(*packet))return false;
  const auto request=q;
  if(!Call(TT_PCI_PROBE_Q38_TOKEN_HIDDEN_CONTROL,*packet) || std::memcmp(&q,&request,sizeof(q)))return false;
  for(unsigned i=0;i<P::Context;i++)if(packet->prompt[i]!=(op==P::Start && i<2?unsigned(i?V::SecondToken:V::FirstToken):0U))return false;
  const auto& r=packet->reply;
  if(r.version!=P::Version || r.operation!=op || r.status || r.started!=1 || r.unsafe || r.ready>1
   || r.complete!=unsigned(op==P::Finish) || r.records!=(op==P::Read?count:0U))return false;
  for(auto word:r.reserved)if(word)return false;
  if(r.detail[0]!=1 || r.detail[1]!=1 || r.detail[2]!=r.stage)return false;
  for(unsigned i=(op==P::Read?count*64:0);i<P::Batch*P::RecordWords;i++)if(packet->records[i])return false;
  return true;
 }
 bool CaptureBoundary()const{
  return cancelAttempted && cancelled && !position && !committed && completedLayers>=1 && completedLayers<=64 && terminalStage==1;
 }
 bool HiddenCall(unsigned part){
  namespace V=PioneerQ38TokenHiddenValidation;
  if(state!=Drained || !CaptureBoundary() || part>=2 || captured)return false;
  std::memset(hiddenPacket.get(),0,sizeof(*hiddenPacket));
  auto& q=hiddenPacket->request;q.magic=V::Magic;q.version=V::Version;q.part=part;q.words=V::PartWords(part);
  if(!V::RequestShape(*hiddenPacket))return false;
  const auto request=q;
  if(!Call(TT_PCI_PROBE_Q38_TOKEN_HIDDEN_READ,*hiddenPacket) || std::memcmp(&q,&request,sizeof(q)))return false;
  const auto& r=hiddenPacket->reply;const auto* d=r.detail;
  if(r.version!=V::Version || r.part!=part || r.started!=1 || r.ready!=1 || r.unsafe || r.status || r.words!=q.words)return false;
  for(auto word:r.reserved)if(word)return false;
  // Native InspectTerminal maps terminal[2]==3 to cancelled,whereas its
  // public stage is terminal[10]. They are NOT the same enum/field.
  const uint32_t expected[]={1,1,4,0,0,0,1,part,part*4096,q.words,position,completedLayers,3,committed,1,q.words*4};
  for(unsigned i=0;i<16;i++)if(d[i]!=expected[i])return false;
  for(unsigned i=q.words;i<V::ChunkWords;i++)if(hiddenPacket->words[i])return false;
  std::copy(hiddenPacket->words,hiddenPacket->words+q.words,hidden->begin()+part*4096);
  return true;
 } // exact raw DATA,including NaN;NO CPU arithmetic/numerical acceptance
 bool RecordData()const{
  // Consistency of the ALREADY kernel-validated public DATA only. Session
  // identifiers/counters are not recreated into native ownership proofs.
  for(unsigned pos=0;pos<committed;pos++){
   const auto* r=packet->records+pos*64;
   const unsigned input=pos==0?248319:pos==1?17:packet->records[64+4];
   if(r[0]!=0x31435451 || r[1]!=1 || r[2]!=pos || r[3]!=input || r[4]>=248320
    || (r[5]&0x7f800000U)==0x7f800000U || r[6]!=pos+1 || r[7]!=((pos+1)&1)
    || r[8]!=unsigned(pos==0) || r[9]!=unsigned(pos<2) || r[10]!=64 || r[11]!=973
    || r[38]!=r[42] || r[44]!=pos+1 || r[45]!=48 || r[46]!=119)return false;
   for(unsigned i=12;i<36;i++)if(r[i]!=packet->records[i])return false;
   for(unsigned i=47;i<64;i++)if(r[i])return false;
  }
  return true;
 }
public:
 const char* failure=nullptr;unsigned operation=0;
 uint64_t pagesUploaded=0,bytesPlanned=0;unsigned partsUploaded=0,uploadSpans=0;
 PioneerQ38TokenBoundaryValidationSession(IO& transport,Source& original):io(transport),source(original){}
 ~PioneerQ38TokenBoundaryValidationSession(){Close();} // no implicit ioctl/END
 PioneerQ38TokenBoundaryValidationSession(const PioneerQ38TokenBoundaryValidationSession&)=delete;
 PioneerQ38TokenBoundaryValidationSession& operator=(const PioneerQ38TokenBoundaryValidationSession&)=delete;
 bool TerminalReady()const{return state==Terminal;}
 bool Completed()const{return state==Complete;}
 bool Captured()const{return state==Complete && captured && CaptureBoundary() && Current();}
 bool CancellationConsumed()const{return state==Complete && cancelAttempted && cancelled;}
 unsigned CommittedRecords()const{return state==Complete?committed:0;}
 unsigned CancelledLayers()const{return CancellationConsumed()?completedLayers:0;}
 bool CopyPendingObservation(PendingObservation& output)const{
  if(state!=Running || !pendingObservationValid || !Current())return false;
  output=pendingObservation;return true;
 } // validated reply DATA ONLY;word is a readout index,NOT a completed layer;
 // raw frontend words are NOT retired PCs,completion or cleanup authority.
 bool Preflight(){
  if(preflightAttempted || attempted || state!=Virgin)return false;
  preflightAttempted=true;
  // Same-open whole-file SHA happens inside the real fixed Source.Open,
  // BEFORE the first accelerator FD or device operation exists.
  if(!source.Open() || !source.Ready() || source.Model().Tensors().size()!=851
   || !layout.Build(source.Model(),8192,true,true))return Stop("source_preparation");
  epoch=source.Model().Epoch();
  if(!Current() || !layout.Tiled() || layout.DeltaSlots()!=2)return Stop("source_identity");
  const auto& parts=layout.Parts();unsigned weightParts=0;
  for(const auto& p:parts)if(p.kind==PIONEER_Q38_WEIGHT){weightParts++;bytesPlanned+=p.allocated;}
  if(parts.size()!=4678 || weightParts!=3551 || !bytesPlanned || bytesPlanned>UINT64_C(21)*1024*1024*1024
   || !Current())return Stop("model_geometry");
  prepared=true;return true;
 } // source/geometry ONLY: NEVER opens an accelerator FD
 bool InspectLearnedPageSamples(){
  if(!prepared || attempted || state!=Virgin || !Current())return false;
  auto page=std::make_unique<std::array<uint8_t,4096>>();const auto& parts=layout.Parts();
  for(unsigned f=0;f<parts.size();f++){
   const auto& p=parts[f];if(p.kind!=PIONEER_Q38_WEIGHT)continue;
   const unsigned count=p.allocated/4096;
   for(unsigned at:{0U,count/2,count-1})if(!source.ReadWeightPage(layout,f,at,page->data(),4096) || !Current())return Stop("sample_read");
  }
  return true;
 } // RAW first/middle/last page reads only,NOT a full-payload or numerical test
 bool Begin(){
  namespace P=PioneerQ38TokenControl;
  if(attempted || state!=Virgin)return false;
  if(!prepared && !Preflight())return false;
  attempted=true;
  if(!Current())return Stop("source_changed");
  const auto& parts=layout.Parts();
  fd=io.Open();if(fd<0)return Stop("open");
  if(!DriverIdentity(0))return Stop("driver_identity");
  tt_q38_bank_support banks={};tt_q38_span_support spans={};
  if(!Call(TT_PCI_PROBE_Q38_BANK_SUPPORT,banks) || banks.driver!=45 || banks.version!=1 || banks.banks!=7
   || banks.page_bytes!=4096 || banks.max_pages!=4 || banks.storage_only!=1 || banks.bytes_per_bank!=layout.Capacity
   || !Call(TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,spans) || spans.driver!=55 || spans.version!=1 || spans.banks!=7
   || spans.page_bytes!=4096 || spans.max_bytes!=1048576 || spans.before_cohort!=1 || spans.explicit_end!=1 || spans.reserved)return Stop("storage_identity");
  tt_q38_bank_session begin={};begin.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_BEGIN,begin) || begin.result.version!=1 || begin.result.state!=TT_RESIDENT_SESSION_READY
   || begin.result.active!=1 || begin.result.host_ready!=1 || begin.result.cold || begin.result.capabilities
   || begin.result.bytes!=UINT64_C(21)*1024*1024*1024)return Stop("begin");
  auto upload=std::make_unique<tt_q38_span_upload>();
  for(unsigned f=0;f<parts.size();f++){
   const auto& p=parts[f];if(p.kind!=PIONEER_Q38_WEIGHT)continue;
   for(unsigned at=0;at<p.allocated/4096;){
    const unsigned count=std::min(256U,p.allocated/4096-at);std::memset(upload.get(),0,sizeof(*upload));
    upload->version=1;upload->bank=p.bank;upload->bytes=count*4096;
    upload->offset=uint64_t(p.proposedAddress)-layout.Base+uint64_t(at)*4096;
    for(unsigned page=0;page<count;page++)if(!source.ReadWeightPage(layout,f,at+page,
     reinterpret_cast<uint8_t*>(upload->data+page*1024),4096))return Stop("source_read");
    if(!Call(TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,*upload))return Stop("upload");
    const auto& r=upload->result;
    if(r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.bytes!=count*4096 || r.pages!=count || r.nonce!=uploadSpans+1
     || r.started!=1 || r.completed!=1 || r.held!=1 || r.host_exact || r.matched_words || r.recovery_required || r.reserved)return Stop("upload_reply");
    pagesUploaded+=count;uploadSpans++;at+=count;io.Progress(pagesUploaded*4096,bytesPlanned);
    if(!Current())return Stop("source_changed");
   }
   partsUploaded++;
  }
  if(partsUploaded!=3551 || pagesUploaded*4096!=bytesPlanned)return Stop("upload_count");
  tt_q38_span_end end={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,end) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required)return Stop("span_end");
  if(!DriverIdentity(1))return Stop("driver_changed");
  if(!TokenCall(P::Start))return Stop("start");
  const auto& r=packet->reply;const auto* d=r.detail;
  if(r.ready!=1 || r.stage!=8 || d[3]!=1 || d[4] || d[5] || d[6] || d[7])return Stop("start_reply");
  state=Running;return true;
 }
 bool RequestCancellation(){
  if(state!=Running || cancelAttempted)return false;
  cancelAttempted=true; // mark BEFORE the first uncertain operation;no retry
  if(!TokenCall(PioneerQ38TokenControl::Cancel))return Stop("cancel");
  const auto& r=packet->reply;const auto* d=r.detail;
  if(r.ready!=1 || r.stage!=7 || d[5] || d[6] || d[7] || d[8] || d[9]!=1
   || d[10] || d[11] || d[12] || d[13] || d[14] || d[15])return Stop("cancel_reply");
  return true;
 } // one RAW request only;NEVER a STOP/HOLD/drain/END permission
 // Pending is successful observation,NOT a reason to retry Start or HOLD.
 // Poll timing/deadline belongs to the explicit caller. Loss/timeout closes
 // only; a separate recovery must resolve any retained native resources.
 bool Poll(){
  if(state!=Running)return false;
  if(!TokenCall(PioneerQ38TokenControl::Observe))return Stop("observe");
  const auto& r=packet->reply;const auto* d=r.detail;
  if(d[5] || d[6]!=r.ready || d[7]>1 || (d[7] && !cancelAttempted) || d[14] || d[15])return Stop("observe_reply");
  if(!r.ready){
   if(r.stage!=9 || d[3] || d[4]>63 || d[7] || d[8] || d[9] || d[10] || d[11])return Stop("pending_reply");
   pendingObservation={d[4],d[12],d[13]};pendingObservationValid=true;
   return true;
  }
  if(r.stage!=11 || d[3] || d[8]>3 || d[9]>=3 || !d[10] || d[10]>64)return Stop("terminal_reply");
  // d[12..13] are the actual RAW frontend snapshots,not retired PCs or
  // USER-side completion authority. The kernel must prove its current
  // boot/receipt/code/119 STOP/FIFO seal before publishing ready. All packet,
  // source/FD/generation,stage,unsafe and terminal-shape checks still apply.
  if(d[7]){
   if(!d[11] || d[11]>3 || d[8]!=d[9]+unsigned(d[11]==3)
    || (d[11]>1 && d[10]!=64))return Stop("cancelled_terminal_reply");
  }else if(d[8]!=3 || d[9]!=2 || d[10]!=64 || d[11])return Stop("normal_terminal_reply");
  cancelled=d[7]!=0;committed=d[8];position=d[9];completedLayers=d[10];terminalStage=d[11];
  state=Terminal;return true;
 }
 bool Finish(){
  namespace P=PioneerQ38TokenControl;
  if(state!=Terminal)return false;
  if(!TokenCall(P::Hold))return Stop("hold");
  {const auto& r=packet->reply;const auto* d=r.detail;
   if(r.ready!=1 || r.stage!=4 || d[5] || d[6]!=1 || d[7]!=120)return Stop("hold_reply");
   for(unsigned i=10;i<16;i++)if(d[i])return Stop("hold_reserved");}
  state=Held;
  if(!TokenCall(P::Read,committed))return Stop("read");
  {const auto& r=packet->reply;const auto* d=r.detail;
   if(r.ready!=1 || r.stage!=4 || d[5] || d[6]!=1 || d[7] || d[8]!=committed
    || d[9]!=unsigned(committed!=0) || d[10]!=committed*256
    || d[11]!=1 || d[14] || d[15] || !RecordData())return Stop("read_reply");}
  std::copy(packet->records,packet->records+records.size(),records.begin());state=Drained;
  if(CaptureBoundary()){
   if(!HiddenCall(0) || !HiddenCall(1))return Stop("hidden_read");
   captured=true;
  } // A missed capture window still follows the validated normal cleanup;
  // it never exports wrong-position/layer DATA or quarantines a clean trial.
  if(!TokenCall(P::Finish))return Stop("finish");
  {const auto& r=packet->reply;const auto* d=r.detail;
   if(r.ready!=1 || r.stage!=11 || d[3]!=1 || d[4] || d[5]!=1 || d[6] || d[7])return Stop("finish_reply");}
  int owned=fd;fd=-1;
  if(io.Close(owned)!=0)return Stop("close");
  state=Complete;return true;
 }
 bool CopySnapshot(PioneerQ38TokenBoundarySnapshot* output,size_t bytes)const{
  if(!Captured() || !output || bytes!=sizeof(*output))return false;
  namespace S=PioneerQ38TokenBoundarySnapshotData;
  const uint64_t tag=PioneerQ38TokenHiddenValidation::BuildTag;
  const uint32_t h[]={S::Magic,S::Version,position,248319,completedLayers,5120,3,committed,
   uint32_t(tag),uint32_t(tag>>32),uint32_t(generation),uint32_t(generation>>32),2,0,0,0};
  std::copy(h,h+16,output->header);std::copy(hidden->begin(),hidden->end(),output->words);output->footer=S::Footer;
  return true;
 } // only after BOTH parts,actual Finish and successful Close;never a partial snapshot
 void Abort(){if(state!=Complete)Stop("aborted");}
};
#endif
