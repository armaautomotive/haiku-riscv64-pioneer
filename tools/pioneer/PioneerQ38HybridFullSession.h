/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Qgw8TraceClientRole.h"
#if defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION)
#if PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_USER_SESSION_INSPECTION) || PIONEER_Q38_QGW8_USER_SESSION_INSPECTION != 1 \
 || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) || defined(PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT) \
 || defined(PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT) || defined(PIONEER_Q38_HYBRID_FIXED_126) \
 || defined(PIONEER_Q38_HYBRID_MATRIX_CLIENT_127)
#error QGW8 full session requires its separate non-loadable inspection role
#endif
#elif !defined(PIONEER_Q38_HYBRID_FULL_CLIENT) || PIONEER_Q38_HYBRID_FULL_CLIENT != 1 \
 || !defined(PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT) || PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT != 1 \
 || !defined(PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT) || PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT != 1
#error Hybrid full candidate is a separate, non-validation client
#endif
#ifndef PIONEER_Q38_QGW8_HIDDEN_SESSION_ROLE_BOUND
#define PIONEER_Q38_QGW8_HIDDEN_SESSION_ROLE_BOUND
#ifdef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
enum {PioneerQ38Qgw8HiddenSessionRole=1};
#else
enum {PioneerQ38Qgw8HiddenSessionRole=0};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
static_assert(PioneerQ38Qgw8HiddenSessionRole==1,"late QGW8 hidden session selection");
#else
static_assert(PioneerQ38Qgw8HiddenSessionRole==0,"late QGW8 hidden session removal");
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT)
#error Hybrid full candidate is a separate, non-validation client
#endif
#ifndef PIONEER_Q38_HYBRID_FULL_SESSION_ROLE_BOUND
#define PIONEER_Q38_HYBRID_FULL_SESSION_ROLE_BOUND
#ifdef PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION
enum {PioneerQ38HybridFullSessionRole=8};
#else
enum {PioneerQ38HybridFullSessionRole=1};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION
static_assert(PioneerQ38HybridFullSessionRole==8,"late QGW8 full-session role change");
#else
static_assert(PioneerQ38HybridFullSessionRole==1,"late legacy full-session role change");
#endif
#ifndef PIONEER_Q38_HYBRID_FULL_SESSION_H
#define PIONEER_Q38_HYBRID_FULL_SESSION_H
#ifdef PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION
#include "PioneerQ38Qgw8UploadSessionInspection.h"
#else
#include "PioneerQ38HybridUploadSession.h"
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h"
#ifdef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8HiddenProtocol.h"
#include "PioneerQ38TokenHiddenSnapshot.h"
#endif
#ifdef PIONEER_Q38_QGW8_TRACE_CLIENT_131
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8TraceNativeProtocol.h"
#include "PioneerQ38Qgw8LayerTraceHandoff.h"
#endif
#include <algorithm>
#include <array>
#include <memory>
#include <new>

// One-shot candidate lifecycle. The uploader and its descriptor remain
// private: START, OBSERVE, HOLD, READ and FINISH use the exact same sealed
// receipt/FD. Any uncertain operation closes only; no retry, BANK_END or
// implicit device cleanup follows. This is not an Assistant integration.
template<class Pages,class IO> class PioneerQ38HybridFullSession {
#ifdef PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION
 using Upload=PioneerQ38Qgw8UploadSessionInspection<Pages,IO>;
 static_assert(PioneerQ38FullOffload::BuildTag==Upload::InspectionTag,"QGW8 full/upload inspection identity mismatch");
#else
 using Upload=PioneerQ38HybridUploadSession<Pages,IO,true>;
#endif
 using Packet=PioneerQ38TokenControl::Packet;
public:
 struct ControlObservation {
  unsigned action=0,callPassed=0,requestUnchanged=0;
  PioneerQ38TokenControl::Reply reply={};
 }; // untrusted diagnostic data only;never grants retry,cleanup or device authority
private:
 enum State {Virgin,Running,Terminal,Held,Drained,Complete,Failed};
 Upload upload;
 std::unique_ptr<Packet> packet=std::make_unique<Packet>();
 std::array<uint32_t,PioneerQ38FullOffload::MaxPositions*64> records={};
 std::array<uint32_t,PioneerQ38FullOffload::MaxPrompt> prompt={};
#ifdef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
 std::unique_ptr<PioneerQ38Qgw8Hidden::Packet> hiddenPacket=std::make_unique<PioneerQ38Qgw8Hidden::Packet>();
 std::array<uint32_t,PioneerQ38Qgw8Hidden::Words> hidden={};
 unsigned hiddenParts=0;bool hiddenCaptured=false;
#endif
#ifdef PIONEER_Q38_QGW8_TRACE_CLIENT_131
 std::unique_ptr<PioneerQ38Qgw8LayerTracePacket::Packet> tracePacket=std::make_unique<PioneerQ38Qgw8LayerTracePacket::Packet>();
 std::unique_ptr<PioneerQ38Qgw8LayerTraceHandoff> traceHandoff;bool traceCaptured=false;
#endif
 State state=Virgin;
 ControlObservation lastControl={};bool observedControl=false;
 unsigned prompts=0,limit=0,committed=0,position=0,layers=0,terminalStage=0;
 bool attempted=false,cancelAttempted=false,cancelled=false;
 bool Stop(const char* why){
  if(failure.empty())failure=why;
  state=Failed;records.fill(0);std::string ignored;
#ifdef PIONEER_Q38_QGW8_TRACE_CLIENT_131
  traceCaptured=false;traceHandoff.reset();
#endif
  upload.Stop(ignored,why);return false;
 }
 template<class T> bool Call(unsigned op,T& value){
  return state!=Failed && upload.WeightsHeld() && upload.Call(op,value) && upload.WeightsHeld();
 }
 bool Control(unsigned action,unsigned count=0){
  namespace P=PioneerQ38TokenControl;
  std::memset(packet.get(),0,sizeof(*packet));auto& q=packet->request;
  q.magic=P::Magic;q.version=P::Version;q.operation=action;q.records=count;
  if(action==P::Start){q.prompts=prompts;q.limit=limit;
#ifdef PIONEER_Q38_QGW8_TRACE_CLIENT_131
   q.stopEos=0;
#else
   q.stopEos=1;
#endif
   q.eos0=PioneerQ38FullOffloadEog::Eos0;q.eos1=PioneerQ38FullOffloadEog::Eos1;
   std::copy(prompt.begin(),prompt.begin()+prompts,packet->prompt);}
  if(!PioneerQ38FullOffload::RequestShape(*packet))return false;
  const auto request=q;
  const bool passed=Call(TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL,*packet);
  const bool same=!std::memcmp(&q,&request,sizeof(q));
  lastControl.action=action;lastControl.callPassed=unsigned(passed);lastControl.requestUnchanged=unsigned(same);
  lastControl.reply=packet->reply;observedControl=true;
  if(!passed || !same)return false;
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
 }
#ifdef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
 bool HiddenCall(unsigned part){
  namespace H=PioneerQ38Qgw8Hidden;
  if(state!=Drained || part!=hiddenParts || part>=H::Parts || hiddenCaptured)return false;
  std::memset(hiddenPacket.get(),0,sizeof(*hiddenPacket));auto& q=hiddenPacket->request;
  q.magic=H::Magic;q.version=H::Version;q.part=part;q.words=H::PartWords(part);
  if(!H::RequestShape(*hiddenPacket))return false;const auto request=q;
  if(!Call(TT_PCI_PROBE_Q38_QGW8_HIDDEN_READ,*hiddenPacket) || std::memcmp(&q,&request,sizeof(q))
   || !H::ReplyShape(*hiddenPacket))return false;
  std::copy(hiddenPacket->words,hiddenPacket->words+q.words,hidden.begin()+part*H::ChunkWords);
  hiddenParts++;return true;
 } // untrusted DATA only; no cleanup, retry or device authority
#endif
public:
 std::string failure;
 PioneerQ38HybridFullSession(Pages& pages,IO& io):upload(pages,io){}
 PioneerQ38HybridFullSession(const PioneerQ38HybridFullSession&)=delete;
 PioneerQ38HybridFullSession& operator=(const PioneerQ38HybridFullSession&)=delete;
 bool TerminalReady()const{return state==Terminal;}
 bool Completed()const{return state==Complete;}
 bool FailedState()const{return state==Failed;}
 bool FailureObservation(ControlObservation& out)const{
  if(state!=Failed || !observedControl)return false;
  out=lastControl;return true;
 } // snapshot only,which may be zero,partial,stale or corrupt on a failed syscall
 bool Begin(const PioneerQ38CanonicalModelPlan& plan,const uint32_t* ids,unsigned count,unsigned outputs,
  const std::function<void(uint64_t,uint64_t)>& progress={},const std::function<bool()>& beforeStart={}){
  namespace F=PioneerQ38FullOffload;
  if(attempted || state!=Virgin || !ids || !count || count>F::MaxPrompt || !outputs || outputs>F::MaxOutput
   || count+outputs-1>F::MaxPositions)return false;
#ifdef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
  if(count!=1 || outputs!=1 || ids[0]!=PioneerQ38Qgw8Hidden::Input)return false;
#endif
#ifdef PIONEER_Q38_QGW8_TRACE_CLIENT_131
  if(count!=1 || outputs!=1 || ids[0]!=248319)return false;
#endif
#ifdef PIONEER_Q38_QGWC_CLIENT_136
  if(count!=1 || outputs!=1 || ids[0]!=248319)return false;
#endif
#ifdef PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION
  if(count!=1 || outputs!=1)return false; // synthetic one-token lifecycle only
#endif
  for(unsigned i=0;i<count;i++)if(ids[i]>=F::Vocabulary)return false;
  attempted=true;prompts=count;limit=count+outputs-1;std::copy(ids,ids+count,prompt.begin());
  std::string error;if(!upload.Begin(plan,error,progress)){failure=error;state=Failed;return false;}
  F::Support support={};
  if(!upload.Identity(1) || !Call(TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT,support) || !F::Supported(support))return Stop("support");
  if(beforeStart && !beforeStart())return Stop("start_clock");
  if(!Control(PioneerQ38TokenControl::Start))return Stop("start");
  const auto& r=packet->reply;const auto* d=r.detail;
  if(r.ready!=1 || r.stage!=8 || d[3]!=1 || d[4] || d[5] || d[6] || d[7])return Stop("start_reply");
  state=Running;return true;
 }
 bool Cancel(){
#if defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129) || defined(PIONEER_Q38_QGW8_TRACE_CLIENT_131)
  return false; // fixed one-token diagnostic never uses a cancellation boundary
#else
  if(state!=Running || cancelAttempted)return false;
  cancelAttempted=true;
  if(!Control(PioneerQ38TokenControl::Cancel))return Stop("cancel");
  const auto& r=packet->reply;const auto* d=r.detail;
  if(r.ready!=1 || r.stage!=7 || d[5] || d[6] || d[7] || d[8] || d[9]!=1
   || d[10] || d[11] || d[12] || d[13] || d[14] || d[15])return Stop("cancel_reply");
  return true;
#endif
 }
 bool Poll(const std::function<void(uint32_t,uint32_t)>& pendingObservation={}){
  if(state!=Running)return false;
  if(!Control(PioneerQ38TokenControl::Observe))return Stop("observe");
  const auto& r=packet->reply;const auto* d=r.detail;
  if(d[5] || d[6]!=r.ready || d[7]>1 || (d[7] && !cancelAttempted) || d[14] || d[15])return Stop("observe_reply");
  if(!r.ready){
   if(r.stage!=9 || d[3] || d[4]>63 || d[7] || d[8] || d[9] || d[10] || d[11])return Stop("pending_reply");
   // These program-counter samples are diagnostic only: neither a progress
   // guarantee nor permission to alter the held session or its hardware.
   if(pendingObservation)pendingObservation(d[12],d[13]);
   return true;
  }
  if(r.stage!=11 || d[3] || d[8]>limit || d[9]>=limit || !d[10] || d[10]>64)return Stop("terminal_reply");
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
#ifdef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
  if(cancelled || prompts!=1 || limit!=1 || committed!=1 || position!=0 || layers!=64 || terminalStage
   || prompt[0]!=PioneerQ38Qgw8Hidden::Input)return Stop("hidden_boundary");
  if(!HiddenCall(0) || !HiddenCall(1))return Stop("hidden_read");
  hiddenCaptured=true;
#endif
#ifdef PIONEER_Q38_QGW8_TRACE_CLIENT_131
  if(cancelled || prompts!=1 || limit!=1 || committed!=1 || position!=0 || layers!=64 || terminalStage || prompt[0]!=248319
   || !upload.canonical || !upload.canonical->Ready() || !upload.generation || !tracePacket)return Stop("trace_boundary");
  {uint32_t workspace=0;if(!upload.TraceWorkspace(workspace))return Stop("trace_workspace");
   traceHandoff.reset(new(std::nothrow) PioneerQ38Qgw8LayerTraceHandoff(upload.generation,workspace));}
  if(!traceHandoff || traceHandoff->Failed())return Stop("trace_allocation");
  for(unsigned ordinal=0;ordinal<PioneerQ38Qgw8LayerTraceGeometry::Chunks;ordinal++){
   *tracePacket={};if(!traceHandoff->Prepare(*tracePacket))return Stop("trace_prepare");
   const bool passed=Call(TT_PCI_PROBE_Q38_QGW8_TRACE_READ,*tracePacket);
   if(!traceHandoff->Accept(*tracePacket,passed))return Stop("trace_read");
  }
  traceCaptured=true;
#endif
  if(!Control(P::Finish))return Stop("finish");
  {const auto& r=packet->reply;const auto* d=r.detail;
   if(r.ready!=1 || r.stage!=11 || d[3]!=1 || d[4] || d[5]!=1 || d[6] || d[7])return Stop("finish_reply");}
  // FINISH already restores and releases the native owner: never BANK_END.
  const int owned=upload.fd;upload.fd=-1;
  if(upload.io.Close(owned))return Stop("close");
  upload.state=Upload::Done;state=Complete;return true;
 }
 bool CopyGeneratedTokens(uint32_t* output,unsigned capacity,unsigned& count)const{
  if(state!=Complete || !failure.empty()
#ifdef PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION
   || !upload.canonical || !upload.canonical->Ready()
#else
   || !upload.layout || !upload.layout->Ready()
#endif
   || !upload.pages.AllVerified()
   || !output || capacity<PioneerQ38FullOffload::MaxOutput)return false;
  if(!cancelled && (layers!=64 || terminalStage || committed!=position+1))return false;
  std::array<uint32_t,PioneerQ38FullOffload::MaxOutput> ids={};unsigned used=0;
  for(unsigned pos=prompts-1;pos<committed;pos++){
   const auto id=records[pos*64+4];if(PioneerQ38FullOffloadEog::Contains(id))break;
   if(used>=ids.size())return false;
   ids[used++]=id;
  }
  std::copy(ids.begin(),ids.begin()+used,output);count=used;return true;
 }
#ifdef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
 bool CopyHiddenSnapshot(PioneerQ38TokenHiddenSnapshot* output,size_t bytes)const{
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this);
  const uintptr_t transport=reinterpret_cast<uintptr_t>(&upload.io);
  if(state!=Complete || !failure.empty() || !hiddenCaptured || hiddenParts!=PioneerQ38Qgw8Hidden::Parts
   || !output || bytes!=sizeof(*output) || out>UINTPTR_MAX-bytes || self>UINTPTR_MAX-sizeof(*this)
   || transport>UINTPTR_MAX-sizeof(upload.io) || (out<self+sizeof(*this) && self<out+bytes)
   || (out<transport+sizeof(upload.io) && transport<out+bytes) || !upload.generation
   || !upload.pages.AllVerified() || !upload.pages.OutputSafe(output,bytes))return false;
  const uint64_t tag=PioneerQ38Qgw8Hidden::BuildTag,generation=upload.generation;
  const uint32_t h[]={PioneerQ38TokenHiddenSnapshotData::Magic,PioneerQ38TokenHiddenSnapshotData::Version,
   0,PioneerQ38Qgw8Hidden::Input,64,PioneerQ38Qgw8Hidden::Words,1,1,
   uint32_t(tag),uint32_t(tag>>32),uint32_t(generation),uint32_t(generation>>32),2,0,0,0};
  std::copy(h,h+16,output->header);std::copy(hidden.begin(),hidden.end(),output->words);
  output->footer=PioneerQ38TokenHiddenSnapshotData::Footer;return true;
 } // publication only AFTER both fixed reads and successful native FINISH+close
#endif
#ifdef PIONEER_Q38_QGW8_TRACE_CLIENT_131
 uint64_t TraceGeneration()const{return state==Complete && failure.empty() && traceCaptured?upload.generation:0;}
 bool CopyTrace(uint32_t* output,size_t words){
  namespace G=PioneerQ38Qgw8LayerTraceGeometry;
  const size_t bytes=size_t(G::Layers)*G::Words*sizeof(uint32_t);
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this),io=reinterpret_cast<uintptr_t>(&upload.io),
   control=reinterpret_cast<uintptr_t>(packet.get()),tracePacketAddress=reinterpret_cast<uintptr_t>(tracePacket.get());
  if(state!=Complete || !failure.empty() || !traceCaptured || !traceHandoff || !output || words!=size_t(G::Layers)*G::Words
   || out>UINTPTR_MAX-bytes || self>UINTPTR_MAX-sizeof(*this) || io>UINTPTR_MAX-sizeof(upload.io)
   || !control || control>UINTPTR_MAX-sizeof(Packet) || !tracePacketAddress || tracePacketAddress>UINTPTR_MAX-sizeof(*tracePacket)
   || (out<self+sizeof(*this) && self<out+bytes) || (out<io+sizeof(upload.io) && io<out+bytes)
   || (out<control+sizeof(Packet) && control<out+bytes) || (out<tracePacketAddress+sizeof(*tracePacket) && tracePacketAddress<out+bytes)
   || !upload.pages.AllVerified() || !upload.pages.OutputSafe(output,bytes) || !upload.io.OutputSafe(output,bytes))return false;
  return traceHandoff->Copy(output,words);
 }
#endif
 void Abort(){if(state!=Complete)Stop("aborted");}
};
#endif
