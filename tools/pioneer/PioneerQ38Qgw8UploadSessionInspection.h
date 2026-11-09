/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Qgw8TraceClientRole.h"
#if defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION) && PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION != 1
#error QGW8 full-session inspection selector must equal one
#endif
#ifndef PIONEER_Q38_QGW8_HIDDEN_UPLOAD_ROLE_BOUND
#define PIONEER_Q38_QGW8_HIDDEN_UPLOAD_ROLE_BOUND
#ifdef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
enum {PioneerQ38Qgw8HiddenUploadRole=1};
#else
enum {PioneerQ38Qgw8HiddenUploadRole=0};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
static_assert(PioneerQ38Qgw8HiddenUploadRole==1,"late QGW8 hidden upload selection");
#else
static_assert(PioneerQ38Qgw8HiddenUploadRole==0,"late QGW8 hidden upload removal");
#endif
#if defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) && (PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128 != 1 \
 || !defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION) || PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION != 1 \
 || defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) || defined(PIONEER_Q38_QGW8_COARSE_PROGRAMS_INSPECTION) || defined(_KERNEL_MODE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGW8 tag128 client requires its distinct candidate role
#endif
#ifndef PIONEER_Q38_QGW8_UPLOAD_SESSION_ROLE_BOUND
#define PIONEER_Q38_QGW8_UPLOAD_SESSION_ROLE_BOUND
#ifdef PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION
enum {PioneerQ38Qgw8UploadSessionRole=1};
#else
enum {PioneerQ38Qgw8UploadSessionRole=0};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION
static_assert(PioneerQ38Qgw8UploadSessionRole==1,"late QGW8 full-session selector");
#else
static_assert(PioneerQ38Qgw8UploadSessionRole==0,"late QGW8 upload-only selector");
#endif
#ifndef PIONEER_Q38_QGW8_UPLOAD_SESSION_INSPECTION_H
#define PIONEER_Q38_QGW8_UPLOAD_SESSION_INSPECTION_H
#if !defined(PIONEER_Q38_QGW8_USER_SESSION_INSPECTION) || PIONEER_Q38_QGW8_USER_SESSION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC) || PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC != 1 \
 || !defined(PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC) || PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC != 1 \
 || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_SPAN_INSPECTION) || PIONEER_Q38_QGW8_SPAN_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_CONTROL_INSPECTION) || PIONEER_Q38_QGW8_CONTROL_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_TRACKED_UPLOAD) || PIONEER_Q38_HYBRID_TRACKED_UPLOAD != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGW8 upload session is an isolated non-loadable inspection only
#endif
#include "PioneerQ38HybridSpanUpload.h"
#include "PioneerQ38HybridHeadLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadProtocol.h"
#ifdef PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h"
#endif

template<class Pages,class IO> class PioneerQ38HybridFullSession;
// One retained diagnostic session from verified QGW8 pages through the
// existing span builder to an explicitly distinct QGW8 receipt. The control
// uses the same IO.Call interface as the other driver requests. The default
// profiles use synthetic identities; only the explicitly selected tag128
// candidate uses its distinct unqualified physical-driver identity.
// After an uncertain operation, close once and never END/reset/retry here.
// IO.OutputSafe must refuse destinations overlapping any dynamically owned
// transport/ledger/receipt storage, even though it grants no device access.
template<class Pages,class IO>class PioneerQ38Qgw8UploadSessionInspection {
 friend class PioneerQ38HybridFullSession<Pages,IO>;
 using C=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridHeadModelPlan;
 enum State {Virgin,Open,Owned,SpansClosed,BanksClosed,Done,Failed};
 Pages& pages;IO& io;const C* canonical=nullptr;int fd=-1;State state=Virgin;uint64_t generation=0;
 PioneerQ38HybridSpanUpload<Pages,IO,PioneerQ38HybridHeadUploadPlan> upload;
 bool attempted=false;
 bool Stop(std::string& error,const char* reason){
  if(error.empty())error=reason;state=Failed;
  if(fd>=0){const int owned=fd;fd=-1;io.Close(owned);}return false;
 }
 template<class T>bool Call(unsigned op,T& packet){
  return fd>=0 && state!=Failed && pages.AllVerified() && io.Call(fd,op,&packet,sizeof(packet)) && pages.AllVerified();
 }
 bool Identity(unsigned owner){
  tt_pioneer_driver_reload q={};q.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,q) || !PioneerDriverReloadRequest(q) || q.expected_tag || q.expected_generation
   || q.driver!=55 || q.guard_version!=1 || q.build_tag!=InspectionTag || !q.generation
   || (generation && generation!=q.generation) || q.quiesced || q.cold || q.active_owner!=owner || q.open_count!=1)return false;
  generation=q.generation;return true;
 }
 bool Receipt(unsigned operation,unsigned expectedSpans,bool sealed){
  namespace Q=PioneerQ38Qgw8Upload;Q::Packet p;p.operation=operation;
  return Q::Request(p) && Call(TT_PCI_PROBE_Q38_QGW8_UPLOAD_CONTROL,p)
   && p.magic==Q::Magic && p.version==Q::Version && p.operation==operation && !p.reserved
   && p.accepted==1 && p.sealed==unsigned(sealed) && p.spans==expectedSpans && !p.recovery_required;
 }
#ifdef PIONEER_Q38_QGW8_TRACE_CLIENT_131
 // Bank zero is fixed by the native START adapter. Derive its relocated
 // workspace from the same verified QGW8 page plan, not the original model.
 bool TraceWorkspace(uint32_t& output)const{
  if(!WeightsHeld())return false;
  Plan::Proposal expected;if(!Plan::Build(*canonical,expected) || expected.parts.size()!=canonical->Count())return false;
  unsigned found=0;uint32_t address=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);if(!p)return false;
   if(p->kind!=C::WorkspaceKind || p->bank!=0)continue;
   const auto* actual=pages.Part(i);const auto& q=expected.parts[i];
   if(++found!=1 || !actual || actual->original!=q.original || actual->bank!=q.bank || actual->offset!=q.offset
    || actual->bytes!=q.bytes || actual->allocated!=q.allocated || actual->encoding!=q.encoding
    || q.bank!=0 || q.encoding!=PioneerQ38HybridModelPlan::Original || q.bytes!=67108864 || q.allocated!=67108864
    || q.offset>UINT32_MAX-C::Base)return false;
   address=C::Base+q.offset;
  }
  if(found!=1 || !WeightsHeld())return false;output=address;return true;
 }
#endif
public:
#ifdef PIONEER_Q38_QGW8_DENSE_VALUE_CLIENT_134
 static constexpr uint64_t InspectionTag=UINT64_C(0x2026110700010134);
#elif defined(PIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133)
 static constexpr uint64_t InspectionTag=UINT64_C(0x2026110700010133);
#elif defined(PIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132)
 static constexpr uint64_t InspectionTag=UINT64_C(0x2026110700010132);
#elif defined(PIONEER_Q38_QGW8_TRACE_CLIENT_131)
 static constexpr uint64_t InspectionTag=UINT64_C(0x2026110700010131);
#elif defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129)
 static constexpr uint64_t InspectionTag=UINT64_C(0x2026110700010129);
#elif defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128)
 static constexpr uint64_t InspectionTag=UINT64_C(0x2026110700010128);
#elif defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION)
 static constexpr uint64_t InspectionTag=UINT64_C(0x51544b4e4f445256); // exact renamed native-context fake tag
#else
 static constexpr uint64_t InspectionTag=UINT64_C(0x385747514f4e4452); // isolated upload-only fake tag
#endif
 enum {RootManifestWords=64*3*7};
 PioneerQ38Qgw8UploadSessionInspection(Pages& p,IO& transport):pages(p),io(transport){}
 PioneerQ38Qgw8UploadSessionInspection(const PioneerQ38Qgw8UploadSessionInspection&)=delete;
 PioneerQ38Qgw8UploadSessionInspection& operator=(const PioneerQ38Qgw8UploadSessionInspection&)=delete;
 ~PioneerQ38Qgw8UploadSessionInspection(){if(fd>=0)io.Close(fd);}
 bool Complete()const{return state==Done;}
 bool FailedState()const{return state==Failed;}
 bool WeightsHeld()const{return state==SpansClosed && fd>=0 && canonical && canonical->Ready()
  && pages.AllVerified() && upload.CompleteHeld();}
 const PioneerQ38HybridSpanUpload<Pages,IO,PioneerQ38HybridHeadUploadPlan>& Upload()const{return upload;}
 // Only metadata derived from this held synthetic-owner session. The
 // manifest is not itself a device pointer grant, ROOT launch or inference.
 bool RootManifest(uint32_t* output,unsigned words)const{
  if(!output || words!=RootManifestWords || !WeightsHeld()
   || !pages.OutputSafe(output,size_t(RootManifestWords)*sizeof(uint32_t))
   || !io.OutputSafe(output,size_t(RootManifestWords)*sizeof(uint32_t)))return false;
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this);
  const uintptr_t source=reinterpret_cast<uintptr_t>(&pages),model=reinterpret_cast<uintptr_t>(canonical);
  const uintptr_t transport=reinterpret_cast<uintptr_t>(&io);
  const size_t bytes=sizeof(uint32_t)*RootManifestWords;
  if((out&3) || out>UINTPTR_MAX-bytes || self>UINTPTR_MAX-sizeof(*this) || source>UINTPTR_MAX-sizeof(pages)
   || model>UINTPTR_MAX-sizeof(*canonical) || transport>UINTPTR_MAX-sizeof(io)
   || (out<self+sizeof(*this) && self<out+bytes) || (out<source+sizeof(pages) && source<out+bytes)
   || (out<model+sizeof(*canonical) && model<out+bytes) || (out<transport+sizeof(io) && transport<out+bytes))return false;
  Plan::Proposal expected;if(!Plan::Build(*canonical,expected) || expected.parts.size()!=canonical->Count())return false;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=pages.Part(i);if(!p)return false;const auto& q=expected.parts[i];
   const uintptr_t address=reinterpret_cast<uintptr_t>(p);
   if(address>UINTPTR_MAX-sizeof(*p) || (out<address+sizeof(*p) && address<out+bytes)
    || p->original!=q.original || p->bank!=q.bank || p->offset!=q.offset || p->bytes!=q.bytes
    || p->allocated!=q.allocated || p->encoding!=q.encoding)return false;
  }
  uint32_t staged[RootManifestWords]={};
  if(!Plan::Manifest(*canonical,expected,staged,RootManifestWords) || !WeightsHeld()
   || !pages.OutputSafe(output,bytes) || !io.OutputSafe(output,bytes))return false;
  std::memcpy(output,staged,bytes);return true;
 }
 // Descriptor data for the QGW8 matrix-head coordinator, derived only
 // from this held session's exact relocated pages. It does not initialize
 // the workspace, authenticate a boot image, start workers or grant a RUN.
 bool HeadMetadata(const uint32_t* rootBoot,unsigned bootWords,unsigned workspaceBank,uint32_t* output,unsigned words)const{
  namespace H=PioneerQ38HybridHeadLayout;namespace W=PioneerQ38HybridHeadWorkerLayout;
  if(!rootBoot || !output || bootWords!=64 || words!=H::Words || workspaceBank>=7 || !WeightsHeld()
   || !pages.OutputSafe(output,size_t(H::Words)*sizeof(uint32_t)) || !io.OutputSafe(output,size_t(H::Words)*sizeof(uint32_t)))return false;
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),boot=reinterpret_cast<uintptr_t>(rootBoot);
  const uintptr_t self=reinterpret_cast<uintptr_t>(this),source=reinterpret_cast<uintptr_t>(&pages),model=reinterpret_cast<uintptr_t>(canonical);
  const uintptr_t transport=reinterpret_cast<uintptr_t>(&io);
  const size_t outBytes=H::Words*sizeof(uint32_t),bootBytes=64*sizeof(uint32_t);
  if(((out|boot)&3) || out>UINTPTR_MAX-outBytes || boot>UINTPTR_MAX-bootBytes || self>UINTPTR_MAX-sizeof(*this)
   || source>UINTPTR_MAX-sizeof(pages) || model>UINTPTR_MAX-sizeof(*canonical) || transport>UINTPTR_MAX-sizeof(io)
   || (out<boot+bootBytes && boot<out+outBytes) || (out<self+sizeof(*this) && self<out+outBytes)
   || (out<source+sizeof(pages) && source<out+outBytes) || (out<model+sizeof(*canonical) && model<out+outBytes)
   || (out<transport+sizeof(io) && transport<out+outBytes))return false;
  uint32_t captured[64];std::memcpy(captured,rootBoot,sizeof(captured));if(!H::C::BootShape(captured))return false;
  Plan::Proposal expected;if(!Plan::Build(*canonical,expected) || expected.parts.size()!=canonical->Count())return false;
  uint32_t staged[H::Words]={};unsigned heads=0,work=0,norm=0,workspace=0,normBank=0,normAddress=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);const auto* actual=pages.Part(i);if(!p || !actual)return false;
   const auto& q=expected.parts[i];const uintptr_t address=reinterpret_cast<uintptr_t>(actual);
   if(address>UINTPTR_MAX-sizeof(*actual) || (out<address+sizeof(*actual) && address<out+outBytes)
    || actual->original!=q.original || actual->bank!=q.bank || actual->offset!=q.offset || actual->bytes!=q.bytes
    || actual->allocated!=q.allocated || actual->encoding!=q.encoding || q.offset>UINT32_MAX-C::Base)return false;
   if(p->kind==C::WorkspaceKind && p->bank==workspaceBank){
    if(++work!=1 || q.encoding!=PioneerQ38HybridModelPlan::Original || q.bytes!=H::C::WorkspaceBytes)return false;
    workspace=C::Base+q.offset;
   }else if(p->kind==C::WeightKind && p->layer==64 && p->role==2){
    if(++norm!=1 || q.encoding!=PioneerQ38HybridModelPlan::Original || q.bytes!=20480 || p->rows!=1 || p->firstRow)return false;
    normBank=q.bank;normAddress=C::Base+q.offset;
   }else if(p->kind==C::WeightKind && p->layer==64 && p->role==1){
    if(q.encoding!=Plan::HeadBfp8 || q.bank>=7 || staged[H::Stripe+q.bank*H::StripeWords+3]
     || q.bytes!=uint64_t(p->rows)*H::RowBytes)return false;
    uint32_t* stripe=staged+H::Stripe+q.bank*H::StripeWords;
    const uint32_t values[]={q.bank,C::Base+q.offset,p->firstRow,p->rows,H::RowBytes,q.bytes,q.allocated,139};
    for(unsigned j=0;j<H::StripeWords;j++)stripe[j]=values[j];heads++;
   }
  }
  if(work!=1 || norm!=1 || heads!=7 || workspace>UINT32_MAX-H::C::HeadOffset)return false;
  const uint32_t fixed[]={H::Magic,1,H::Words,workspaceBank,workspace,workspace+H::C::HeadOffset,
   normBank,normAddress,5120,248320,H::Jobs,H::Waves,119,8,H::RowBytes,20480};
  for(unsigned i=0;i<16;i++){staged[i]=fixed[i];staged[16+i]=captured[16+i];}
  for(unsigned i=0;i<8;i++)staged[32+i]=captured[8+i];
  const uint32_t witness[]={W::Magic,H::C::Magic,64,0,32,8,0,1};
  for(unsigned i=0;i<8;i++)staged[40+i]=witness[i];
  if(!H::Shape(staged,captured,workspaceBank,workspace) || !WeightsHeld() || !pages.OutputSafe(output,outBytes)
   || !io.OutputSafe(output,outBytes))return false;
  std::memcpy(output,staged,outBytes);return true;
 }
 bool Begin(const C& source,std::string& error,const std::function<void(uint64_t,uint64_t)>& progress={}){
  if(attempted){error="QGW8 inspection session is one-shot; no retry.";return false;}
  attempted=true;error.clear();
  if(!source.OriginalFormatLayout() || !pages.AllVerified())return Stop(error,"Preverified QGW8 source required before open.");
  fd=io.Open();if(fd<0)return Stop(error,"Cannot open QGW8 inspection transport.");state=Open;
  if(!Identity(0))return Stop(error,"QGW8 inspection identity or exclusive idle state refused.");
#ifdef PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION
  PioneerQ38FullOffload::Support support={};
  if(!Call(TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT,support) || !PioneerQ38FullOffload::Supported(support))
   return Stop(error,"QGW8 full-session capability refused before upload.");
#endif
  tt_q38_bank_support banks={};tt_q38_span_support spans={};
  if(!Call(TT_PCI_PROBE_Q38_BANK_SUPPORT,banks) || banks.driver!=45 || banks.version!=1 || banks.banks!=7
   || banks.page_bytes!=4096 || banks.max_pages!=4 || banks.storage_only!=1 || banks.bytes_per_bank!=C::Capacity
   || !Call(TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,spans) || spans.driver!=55 || spans.version!=1 || spans.banks!=7
   || spans.page_bytes!=4096 || spans.max_bytes!=TT_Q38_SPAN_PUBLIC_BYTES || spans.before_cohort!=1
   || spans.explicit_end!=1 || spans.reserved)return Stop(error,"QGW8 inspection bank/span capability refused.");
  tt_q38_bank_session begin={};begin.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_BEGIN,begin) || begin.version!=1 || begin.reserved || begin.result.version!=1
   || begin.result.state!=TT_RESIDENT_SESSION_READY || begin.result.active!=1 || begin.result.host_ready!=1
   || begin.result.cold || begin.result.capabilities || begin.result.bytes!=PioneerResidentBanks::Total)
   return Stop(error,"QGW8 inspection bank BEGIN uncertain; close only.");
  state=Owned;
  if(!Receipt(PioneerQ38Qgw8Upload::Begin,0,false))return Stop(error,"QGW8 receipt BEGIN uncertain; close only.");
  if(!upload.Run(pages,source,io,fd,error,progress) || !upload.CompleteHeld() || upload.UploadedParts()!=3551
   || upload.UploadedSpans()!=22035 || upload.UploadedBytes()!=UINT64_C(21167624192))
   return Stop(error,"QGW8 inspection upload uncertain; close only.");
  tt_q38_span_end spanEnd={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,spanEnd) || spanEnd.version!=1 || spanEnd.reserved
   || spanEnd.closed!=1 || spanEnd.recovery_required)return Stop(error,"QGW8 span END uncertain; close only.");
  if(!Receipt(PioneerQ38Qgw8Upload::Seal,upload.UploadedSpans(),true))return Stop(error,"QGW8 receipt SEAL uncertain; close only.");
  canonical=&source;state=SpansClosed;error.clear();return true;
 }
 bool Finish(std::string& error){
  if(state!=SpansClosed){error="No held QGW8 inspection upload to finish.";return false;}
  error.clear();if(!WeightsHeld())return Stop(error,"Held QGW8 source or upload changed; close only.");
  tt_q38_bank_session end={};end.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,end) || end.version!=1 || end.reserved || end.result.version!=1
   || end.result.state!=TT_RESIDENT_SESSION_CLOSED || end.result.active || end.result.cold
   || end.result.pci_restored!=1 || end.result.host_released!=1)return Stop(error,"QGW8 bank END uncertain; close only.");
  state=BanksClosed;if(!Identity(0))return Stop(error,"QGW8 post-END driver state uncertain; close only.");
  const int owned=fd;fd=-1;if(io.Close(owned)!=0)return Stop(error,"QGW8 inspection close failed.");
  state=Done;error.clear();return true;
 }
 bool Run(const C& source,std::string& error,const std::function<void(uint64_t,uint64_t)>& progress={}){
  return Begin(source,error,progress) && Finish(error);
 }
};
#endif
