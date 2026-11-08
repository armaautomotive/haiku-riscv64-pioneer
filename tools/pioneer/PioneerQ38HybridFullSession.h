/* SPDX-License-Identifier: MIT */
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
#include <algorithm>
#include <array>
#include <memory>

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
 State state=Virgin;
 ControlObservation lastControl={};bool observedControl=false;
 unsigned prompts=0,limit=0,committed=0,position=0,layers=0,terminalStage=0;
 bool attempted=false,cancelAttempted=false,cancelled=false;
 bool Stop(const char* why){
  if(failure.empty())failure=why;
  state=Failed;records.fill(0);std::string ignored;
  upload.Stop(ignored,why);return false;
 }
 template<class T> bool Call(unsigned op,T& value){
  return state!=Failed && upload.WeightsHeld() && upload.Call(op,value) && upload.WeightsHeld();
 }
 bool Control(unsigned action,unsigned count=0){
  namespace P=PioneerQ38TokenControl;
  std::memset(packet.get(),0,sizeof(*packet));auto& q=packet->request;
  q.magic=P::Magic;q.version=P::Version;q.operation=action;q.records=count;
  if(action==P::Start){q.prompts=prompts;q.limit=limit;q.stopEos=1;
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
  const std::function<void(uint64_t,uint64_t)>& progress={}){
  namespace F=PioneerQ38FullOffload;
  if(attempted || state!=Virgin || !ids || !count || count>F::MaxPrompt || !outputs || outputs>F::MaxOutput
   || count+outputs-1>F::MaxPositions)return false;
  for(unsigned i=0;i<count;i++)if(ids[i]>=F::Vocabulary)return false;
  attempted=true;prompts=count;limit=count+outputs-1;std::copy(ids,ids+count,prompt.begin());
  std::string error;if(!upload.Begin(plan,error,progress)){failure=error;state=Failed;return false;}
  F::Support support={};
  if(!upload.Identity(1) || !Call(TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT,support) || !F::Supported(support))return Stop("support");
  if(!Control(PioneerQ38TokenControl::Start))return Stop("start");
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
 }
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
 void Abort(){if(state!=Complete)Stop("aborted");}
};
#endif
