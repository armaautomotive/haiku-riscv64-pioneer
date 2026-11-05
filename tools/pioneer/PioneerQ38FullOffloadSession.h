/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || PIONEER_Q38_FULL_OFFLOAD_CLIENT != 1 \
 || defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || (defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) && PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1) \
 || (defined(PIONEER_Q38_FULL_OFFLOAD_METRICS) && PIONEER_Q38_FULL_OFFLOAD_METRICS != 1)
#error Full-offload client is separate from hybrid Work,CPU validation and diagnostics
#endif
#ifndef PIONEER_Q38_FULL_OFFLOAD_SESSION_H
#define PIONEER_Q38_FULL_OFFLOAD_SESSION_H
#include "PioneerQ38PublicModelSource.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadMetricsRole.h"
#endif
#include <array>
#include <cstring>
#include <memory>
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
#include "PioneerQ38PreparedQ4SessionSource.h"
#endif

// Host work is authentication,weight-format loading,token IDs and lifecycle
// metadata ONLY. No llama context,ggml graph,learned products,host KV or hidden
// state. All inference arithmetic,including greedy selection,is on card.
// IO/Source substitution is for explicit offline transport tests,not fallback.
template<class IO,class Source=PioneerQ38PublicModelSource
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
 ,class Prepared=PioneerQ38PreparedQ4SessionSource<Source>
#endif
 >
class PioneerQ38FullOffloadSession {
 using Packet=PioneerQ38TokenControl::Packet;
 enum State {Virgin,Running,Terminal,Held,Drained,Complete,Failed};
 IO& io;Source& source;PioneerQwen38PackedPlan layout;
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
 std::unique_ptr<Prepared> preparedData=std::make_unique<Prepared>();
#endif
 std::unique_ptr<Packet> packet=std::make_unique<Packet>();
 std::array<uint32_t,PioneerQ38FullOffload::MaxPositions*64> records={};
 std::array<uint32_t,PioneerQ38FullOffload::MaxPrompt> prompt={};
 State state=Virgin;int fd=-1;uint64_t epoch=0,generation=0;
 bool prepared=false,preflightAttempted=false,attempted=false,probed=false,cancelAttempted=false,cancelled=false;
 unsigned prompts=0,limit=0,eos0=0,eos1=0,committed=0,position=0,layers=0,terminalStage=0;
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
 unsigned observedCount=0;
#endif
#ifdef PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE
 PioneerQ38TokenPhaseProfileControl::Packet phaseProfile;
 bool phaseProfileRead=false;
#endif
 bool Current()const{return epoch && source.Ready() && source.Model().Epoch()==epoch && layout.MatchesSource(source.Model())
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
  && (!prepared || preparedData->Ready())
#endif
  ;}
 void Close(){if(fd>=0){const int owned=fd;fd=-1;io.Close(owned);}}
 bool Stop(const char* why){if(!failure)failure=why;state=Failed;records.fill(0);Close();return false;}
 template<class T>bool Call(unsigned op,T& value){
  operation=op;return state!=Failed && fd>=0 && Current() && io.Call(fd,op,&value,sizeof(value)) && Current();
 }
 bool Identity(unsigned owner){
  tt_pioneer_driver_reload r={};r.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,r) || !PioneerDriverReloadRequest(r) || r.expected_tag || r.expected_generation
   || r.driver!=55 || r.guard_version!=1 || r.build_tag!=PioneerQ38FullOffload::BuildTag
   || !r.generation || (generation && r.generation!=generation) || r.quiesced || r.cold
   || r.active_owner!=owner || r.open_count!=1)return false;
  generation=r.generation;return true;
 }
 bool Support(){
  PioneerQ38FullOffload::Support support={};
  return Call(TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT,support) && PioneerQ38FullOffload::Supported(support);
 }
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
 bool PublishPreparedSource(){
  namespace P=PioneerQ38PreparedSource;
  auto p=std::make_unique<P::Packet>();
  for(unsigned step=0;step<P::Phases+2;step++){
   const unsigned op=step==0?P::Begin:step==P::Phases+1?P::Seal:P::Phase;
   const unsigned index=op==P::Begin?0:op==P::Phase?step-1:unsigned(P::Phases);
   if(!preparedData->Publication(op,index,*p) || !P::RequestShape(*p))return false;
   const auto request=p->request;
   if(!Call(TT_PCI_PROBE_Q38_PREPARED_SOURCE,*p) || std::memcmp(&p->request,&request,sizeof(request)))return false;
   const auto& r=p->reply;
   if(r.version!=P::Version || r.operation!=op || r.status || r.accepted!=1 || r.ready!=unsigned(op==P::Seal)
    || r.parts!=P::Parts || r.phases!=(op==P::Begin?0:op==P::Phase?index+1:unsigned(P::Phases)) || r.reserved)return false;
  }
  return Current();
 }
#endif
 bool Control(unsigned action,unsigned count=0){
  namespace P=PioneerQ38TokenControl;
  std::memset(packet.get(),0,sizeof(*packet));auto& q=packet->request;
  q.magic=P::Magic;q.version=P::Version;q.operation=action;q.records=count;
  if(action==P::Start){q.prompts=prompts;q.limit=limit;q.stopEos=1;q.eos0=eos0;q.eos1=eos1;
   std::copy(prompt.begin(),prompt.begin()+prompts,packet->prompt);}
  if(!PioneerQ38FullOffload::RequestShape(*packet))return false;
  const auto request=q;
  if(!Call(TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL,*packet) || std::memcmp(&q,&request,sizeof(q)))return false;
  for(unsigned i=0;i<P::Context;i++)if(packet->prompt[i]!=(action==P::Start && i<prompts?prompt[i]:0U))return false;
  const auto& r=packet->reply;
  if(r.version!=P::Version || r.operation!=action || r.status || r.started!=1 || r.unsafe || r.ready>1
   || r.complete!=unsigned(action==P::Finish) || r.records!=(action==P::Read?count:0U))return false;
  for(auto word:r.reserved)if(word)return false;
  if(r.detail[0]!=1 || r.detail[1]!=1 || r.detail[2]!=r.stage)return false;
  for(unsigned i=action==P::Read?count*64:0;i<P::Batch*P::RecordWords;i++)if(packet->records[i])return false;
  return true;
 }
 bool RecordData(unsigned first,unsigned count)const{
  for(unsigned n=0;n<count;n++){
   const unsigned pos=first+n;const auto* r=packet->records+n*64;
   const unsigned input=pos<prompts?prompt[pos]:n?packet->records[(n-1)*64+4]:records[(pos-1)*64+4];
   const bool end=pos+1==limit || (pos+1>=prompts && PioneerQ38FullOffloadEog::Contains(r[4]));
   if(r[0]!=0x31435451 || r[1]!=1 || r[2]!=pos || r[3]!=input || r[4]>=PioneerQ38FullOffload::Vocabulary
    || (r[5]&0x7f800000U)==0x7f800000U || r[6]!=pos+1 || r[7]!=((pos+1)&1)
    || r[8]!=unsigned(pos+1<prompts) || r[9]!=unsigned(!end) || (end && pos+1!=committed)
    || r[10]!=64 || r[11]!=973 || r[38]!=r[42] || r[44]!=pos+1 || r[45]!=48 || r[46]!=119)return false;
   const auto* anchor=first?records.data():packet->records;
   for(unsigned i=12;i<36;i++)if(r[i]!=anchor[i])return false;
   for(unsigned i=47;i<64;i++)if(r[i])return false;
   if(pos+1==committed && !cancelled && !end)return false;
  }
  return true;
 } // consistency of kernel-validated DATA,not a CPU numerical comparison
public:
 const char* failure=nullptr;unsigned operation=0;
 uint64_t pagesUploaded=0,bytesPlanned=0;unsigned partsUploaded=0,uploadSpans=0;
 PioneerQ38FullOffloadSession(IO& transport,Source& original):io(transport),source(original){}
 ~PioneerQ38FullOffloadSession(){Close();} // never implicit cancellation/END/reset
 PioneerQ38FullOffloadSession(const PioneerQ38FullOffloadSession&)=delete;
 PioneerQ38FullOffloadSession& operator=(const PioneerQ38FullOffloadSession&)=delete;
 bool TerminalReady()const{return state==Terminal;}
 bool Completed()const{return state==Complete;}
 bool KnownClean()const{return state==Complete || (state==Virgin && !attempted && fd<0);}
 bool WasCancelled()const{return state==Complete && cancelled;}
 bool CancelRequested()const{return cancelAttempted;}
 bool FailedState()const{return state==Failed;}
 bool Preflight(){
  if(preflightAttempted || attempted || state!=Virgin)return false;
  preflightAttempted=true;
  if((!source.Ready() && !source.Open()) || !source.Ready() || source.Model().Tensors().size()!=851
   || !layout.Build(source.Model(),8192,true,true))return Stop("source_preparation");
  epoch=source.Model().Epoch();
  if(!Current() || !layout.Tiled() || layout.DeltaSlots()!=2)return Stop("source_identity");
  unsigned weights=0;
  for(const auto& p:layout.Parts())if(p.kind==PIONEER_Q38_WEIGHT){weights++;bytesPlanned+=p.allocated;}
  if(layout.Parts().size()!=4678 || weights!=3551 || bytesPlanned!=UINT64_C(19399024640) || !Current())return Stop("model_geometry");
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
  if(!preparedData->Build(source,layout) || !preparedData->Ready())return Stop("prepared_source");
  bytesPlanned=preparedData->Bytes();
  if(preparedData->Spans().size()!=4895 || bytesPlanned!=UINT64_C(20470759424) || !Current())return Stop("prepared_geometry");
#endif
  prepared=true;return true;
 } // whole same-open original authentication BEFORE any accelerator Open
 bool Probe(){
  if(probed || attempted || state!=Virgin || (!prepared && !Preflight()))return false;
  probed=true;fd=io.Open();if(fd<0)return Stop("open");
  if(!Identity(0) || !Support())return Stop("full_offload_unavailable");
  const int owned=fd;fd=-1;if(io.Close(owned))return Stop("close");
  return Current()?true:Stop("source_changed");
 } // metadata ONLY;Load cannot execute BEGIN or inference
 bool Begin(const uint32_t* ids,unsigned count,unsigned outputs,unsigned firstEos,unsigned secondEos){
  namespace F=PioneerQ38FullOffload;
  if(attempted || state!=Virgin || !ids || !count || count>F::MaxPrompt || !outputs || outputs>F::MaxOutput
   || count+outputs-1>F::MaxPositions || firstEos!=PioneerQ38FullOffloadEog::Eos0 || secondEos!=PioneerQ38FullOffloadEog::Eos1)return false;
  for(unsigned i=0;i<count;i++)if(ids[i]>=F::Vocabulary)return false;
  if(!prepared && !Preflight())return false;
  attempted=true;prompts=count;limit=count+outputs-1;eos0=firstEos;eos1=secondEos;
  std::copy(ids,ids+count,prompt.begin());
  if(!Current())return Stop("source_changed");
  fd=io.Open();if(fd<0)return Stop("open");
  if(!Identity(0) || !Support())return Stop("full_offload_unavailable");
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
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
  const auto& parts=preparedData->Spans();
#else
  const auto& parts=layout.Parts();
#endif
  for(unsigned f=0;f<parts.size();f++){
   const auto& p=parts[f];
#ifndef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
   if(p.kind!=PIONEER_Q38_WEIGHT)continue;
#endif
   for(unsigned at=0;at<p.allocated/4096;){
    const unsigned pages=std::min(256U,p.allocated/4096-at);std::memset(upload.get(),0,sizeof(*upload));
    upload->version=1;upload->bank=p.bank;upload->bytes=pages*4096;
    upload->offset=uint64_t(
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
     p.address
#else
     p.proposedAddress
#endif
     )-layout.Base+uint64_t(at)*4096;
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
    for(unsigned page=0;page<pages;page++)if(!preparedData->ReadPage(f,at+page,
     reinterpret_cast<uint8_t*>(upload->data+page*1024),4096))return Stop("prepared_read");
#else
    for(unsigned page=0;page<pages;page++)if(!source.ReadWeightPage(layout,f,at+page,
     reinterpret_cast<uint8_t*>(upload->data+page*1024),4096))return Stop("source_read");
#endif
    if(!Call(TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,*upload))return Stop("upload");
    const auto& r=upload->result;
    if(r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.bytes!=pages*4096 || r.pages!=pages || r.nonce!=uploadSpans+1
     || r.started!=1 || r.completed!=1 || r.held!=1 || r.host_exact || r.matched_words || r.recovery_required || r.reserved)return Stop("upload_reply");
    pagesUploaded+=pages;uploadSpans++;at+=pages;io.Progress(pagesUploaded*4096,bytesPlanned);
    if(!Current())return Stop("source_changed");
   }
   partsUploaded++;
  }
  if(partsUploaded!=
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
   4895
#else
   3551
#endif
   || pagesUploaded*4096!=bytesPlanned)return Stop("upload_count");
  tt_q38_span_end end={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,end) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required)return Stop("span_end");
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
  if(!Identity(1))return Stop("start_identity");
  if(!PublishPreparedSource())return Stop("prepared_publication");
  if(!Control(PioneerQ38TokenControl::Start))return Stop("start");
#else
  if(!Identity(1) || !Control(PioneerQ38TokenControl::Start))return Stop("start");
#endif
  const auto& r=packet->reply;const auto* d=r.detail;
  if(r.ready!=1 || r.stage!=8 || d[3]!=1 || d[4] || d[5] || d[6] || d[7])return Stop("start_reply");
  state=Running;return true;
 }
 bool Cancel(){
  if(state!=Running || cancelAttempted)return false;
  cancelAttempted=true;
  if(!Control(PioneerQ38TokenControl::Cancel))return Stop("cancel");
  const auto& r=packet->reply;const auto* d=r.detail;
  if(r.ready!=1 || r.stage!=7 || d[5] || d[6] || d[7] || d[8] || d[9]!=1
   || d[10] || d[11] || d[12] || d[13] || d[14] || d[15])return Stop("cancel_reply");
  return true;
 } // monotonic request;not STOP,HOLD or END authority
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
 bool ObserveCount(PioneerQ38FullOffload::CounterPacket& output){
  namespace F=PioneerQ38FullOffload;
  if((state!=Running && state!=Terminal) || !F::CounterRequest(output))return false;
  F::CounterPacket packet;const auto request=packet.request;
  if(!Call(TT_PCI_PROBE_Q38_FULL_OFFLOAD_COUNTER,packet)
   || std::memcmp(&packet.request,&request,sizeof(request)) || !F::CounterReply(packet)
   || packet.reply.detail[8]!=limit || packet.reply.detail[9]!=prompts
   || packet.reply.detail[7]<observedCount || (state==Terminal && packet.reply.detail[7]!=committed))return Stop("counter");
  observedCount=packet.reply.detail[7];output=packet;return true;
 } // read-only progress only;does NOT set Terminal,Held,Drained or Complete
#endif
 bool Poll(){
  if(state!=Running)return false;
  if(!Control(PioneerQ38TokenControl::Observe))return Stop("observe");
  const auto& r=packet->reply;const auto* d=r.detail;
  if(d[5] || d[6]!=r.ready || d[7]>1 || (d[7] && !cancelAttempted) || d[14] || d[15])return Stop("observe_reply");
  if(!r.ready){
   if(r.stage!=9 || d[3] || d[4]>63 || d[7] || d[8] || d[9] || d[10] || d[11])return Stop("pending_reply");
   return true;
  }
  if(r.stage!=11 || d[3] || d[8]>limit || d[9]>=limit || !d[10] || d[10]>64)return Stop("terminal_reply");
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
  if(d[8]<observedCount)return Stop("terminal_counter_regression");
#endif
  if(d[7]){
   if(!d[11] || d[11]>3 || d[8]!=d[9]+unsigned(d[11]==3) || (d[11]>1 && d[10]!=64))return Stop("cancelled_terminal_reply");
  }else if(d[8]<prompts || d[8]!=d[9]+1 || d[10]!=64 || d[11])return Stop("normal_terminal_reply");
  cancelled=d[7]!=0;committed=d[8];position=d[9];layers=d[10];terminalStage=d[11];state=Terminal;return true;
 }
 bool Finish(){
  namespace P=PioneerQ38TokenControl;
  if(state!=Terminal)return false;
  if(!Control(P::Hold))return Stop("hold");
  {const auto& r=packet->reply;const auto* d=r.detail;
   if(r.ready!=1 || r.stage!=4 || d[5] || d[6]!=1 || d[7]!=120)return Stop("hold_reply");
   for(unsigned i=10;i<16;i++)if(d[i])return Stop("hold_reserved");}
  state=Held;unsigned cursor=0;
  do{
   const unsigned count=std::min(64U,committed-cursor);
   if(!Control(P::Read,count))return Stop("read");
   const auto& r=packet->reply;const auto* d=r.detail;
   if(r.ready!=1 || r.stage!=4 || d[5] || d[6]!=1 || d[7]!=cursor || d[8]!=count
    || d[9]!=unsigned(count!=0) || d[10]!=count*256 || d[11]!=unsigned(cursor+count==committed)
    || d[14] || d[15] || !RecordData(cursor,count))return Stop("read_reply");
   std::copy(packet->records,packet->records+count*64,records.begin()+cursor*64);cursor+=count;
  }while(cursor<committed);
  state=Drained;
#ifdef PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE
  {namespace C=PioneerQ38TokenPhaseProfileControl;
   C::Packet observed;const auto request=observed.request;
   if(phaseProfileRead || !Call(TT_PCI_PROBE_Q38_FULL_OFFLOAD_PHASE_PROFILE,observed)
    || std::memcmp(&observed.request,&request,sizeof(request)) || !C::ReplyShape(observed)
    || observed.reply.status || observed.reply.ready!=1 || observed.reply.detail[8]!=committed)return Stop("phase_profile");
   phaseProfile=observed;phaseProfileRead=true;}
#endif
  if(!Control(P::Finish))return Stop("finish");
  {const auto& r=packet->reply;const auto* d=r.detail;
   if(r.ready!=1 || r.stage!=11 || d[3]!=1 || d[4] || d[5]!=1 || d[6] || d[7])return Stop("finish_reply");}
  const int owned=fd;fd=-1;if(io.Close(owned))return Stop("close");state=Complete;return true;
 } // full STOP/HOLD/drain/restore/END before any answer is published
#ifdef PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE
 bool CopyPhaseProfile(PioneerQ38TokenPhaseProfileControl::Packet& output)const{
  const uintptr_t out=reinterpret_cast<uintptr_t>(&output),self=reinterpret_cast<uintptr_t>(this);
  if(out>UINTPTR_MAX-sizeof(output) || self>UINTPTR_MAX-sizeof(*this)
   || (out<self+sizeof(*this) && self<out+sizeof(output)))return false;
  namespace C=PioneerQ38TokenPhaseProfileControl;
  if(state!=Complete || failure || !Current() || !phaseProfileRead || !C::RequestShape(output)
   || !C::ReplyShape(phaseProfile))return false;
  output=phaseProfile;return true;
 } // observation only,AFTER actual END+Close;bad timing still requires normal cleanup
#endif
 bool CopyGeneratedTokens(uint32_t* output,unsigned capacity,unsigned& count)const{
  if(state!=Complete || failure || !Current() || !output || capacity<PioneerQ38FullOffload::MaxOutput)return false;
  if(!cancelled && (layers!=64 || terminalStage || committed!=position+1))return false;
  std::array<uint32_t,PioneerQ38FullOffload::MaxOutput> ids={};unsigned used=0;
  for(unsigned pos=prompts-1;pos<committed;pos++){
   const auto id=records[pos*64+4];if(PioneerQ38FullOffloadEog::Contains(id))break;
   if(used>=ids.size())return false;
   ids[used++]=id;
  }
  std::copy(ids.begin(),ids.begin()+used,output);count=used;return true;
 }
 void Abort(){if(state!=Complete)Stop("aborted");}
};
#endif
