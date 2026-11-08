/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38HybridKernelCompileOnly.h"
#ifndef PIONEER_Q38_CANONICAL_MODEL_PLAN_H
#define PIONEER_Q38_CANONICAL_MODEL_PLAN_H
#if defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) && (PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1 \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION) \
  && (!defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY != 1)) \
 || ((defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION)) \
 || (defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) && !defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128)) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGW8 canonical layout is non-loadable inspection data only
#endif
#ifdef PIONEER_Q38_QGW8_LAYOUT_INSPECTION
#define PIONEER_Q38_QGW8_CANONICAL_PRESENT 1
#endif
#if defined(PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC) && (PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC != 1 \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT))
#error Hybrid canonical repack is diagnostic DATA only, not a native or Work capability
#endif
#include "PioneerQ38GdnBanks.h"
// KERNEL-PRIVATE,HEAP ONLY. Reconstruct the FIXED public27B original-TC
// tiled layout,8192 context,TWO recurrent slots. No GGUF descriptor order,
// caller coordinates,NoC route,host pointer or claimed digest is accepted.
// This is immutable GEOMETRY,NOT source authentication,bank ownership,
// initialization,an input producer or launch/terminal authority. A native
// caller must separately bind the SAME authenticated retained model and
// actual bank/upload ledger under its exclusive context lock.
struct PioneerQ38CanonicalPart {
 uint32_t kind,layer,role,type,bank,firstRow,rows,rowBytes,offset,bytes,allocated;
};
class PioneerQ38CanonicalModelPlan {
 struct Shape {uint32_t inputs,outputs,type;};
 PioneerQ38CanonicalPart parts[6144]={};
 PioneerQ38GdnRegion recurrent[672]={};
 uint32_t firstFfn[64][3]={},workspace[7]={};
 uint64_t used[7]={};
 unsigned count=0,recurrentCount=0,tensors=0;
 bool attempted=false,ready=false;
 enum LayoutKind {UnbuiltLayout,OriginalLayout,HybridLayout,Qgw8Layout};
 LayoutKind layout=UnbuiltLayout;
 static bool Tensor(unsigned layer,unsigned role,Shape& s) {
  static constexpr Shape common[]={{5120,1,0},{5120,1,0},{5120,17408,12},{5120,17408,12},{17408,5120,12}};
  static constexpr Shape delta[]={{5120,6144,8},{5120,10240,8},{48,1,0},{48,1,0},{128,1,0},
   {5120,48,8},{5120,48,8},{4,10240,0},{6144,5120,8}};
  static constexpr Shape attention[]={{5120,12288,8},{5120,1024,8},{5120,1024,8},{256,1,0},{256,1,0},{6144,5120,14}};
  static constexpr Shape global[]={{5120,248320,12},{5120,248320,14},{5120,1,0}};
  if(layer==64){if(role>=3)return false;s=global[role];return true;}
  if(layer>=64 || role>=(layer%4==3?11U:14U))return false;
  s=role<5?common[role]:layer%4==3?attention[role-5]:delta[role-5];return true;
 }
 static unsigned RowBytes(const Shape& s) {
  if(!s.type)return s.inputs*4;
  unsigned block=s.type==8?32:256,tile=s.type==8?576:s.type==12?2304:s.type==14?3392:0;
  return tile && !(s.inputs%block)?s.inputs/block*(tile/16):0;
 }
 bool Stripe(unsigned kind,unsigned layer,unsigned role,unsigned type,unsigned rows,unsigned rowBytes,unsigned unit) {
  if(!rows || !rowBytes || !unit)return false;
  unsigned chunks=rows/unit,n=chunks<7?1:7,counts[7]={},banks[7]={0,1,2,3,4,5,6};
  if(n==1)counts[0]=rows;
  else {for(unsigned b=0;b<7;b++)counts[b]=(chunks/7+unsigned(b<chunks%7))*unit;counts[6]+=rows%unit;}
  // Exact loader ordering:least-used bank first,bank index breaks ties.
  // Sort this snapshot ONCE;appending one stripe must not reorder later ones.
  for(unsigned a=0;a<7;a++)for(unsigned b=a+1;b<7;b++)
   if(used[banks[b]]<used[banks[a]] || (used[banks[b]]==used[banks[a]] && banks[b]<banks[a])){
    unsigned swap=banks[a];banks[a]=banks[b];banks[b]=swap;}
  unsigned first=0;
  for(unsigned p=0;p<n;p++) {
   unsigned bank=banks[p];uint64_t bytes=uint64_t(counts[p])*rowBytes,allocated=(bytes+4095)&~UINT64_C(4095);
   if(count>=6144 || !counts[p] || bytes>UINT32_MAX || used[bank]>Capacity || allocated>Capacity-used[bank])return false;
   parts[count++]={kind,layer,role,type,bank,first,counts[p],rowBytes,uint32_t(used[bank]),uint32_t(bytes),uint32_t(allocated)};
   if(kind==RecurrentKind){if(recurrentCount>=672)return false;
    recurrent[recurrentCount++]={layer,role,first,counts[p],bank,uint32_t(used[bank])};}
   used[bank]+=allocated;first+=counts[p];
  }
  return first==rows;
 }
public:
 static constexpr uint64_t Capacity=UINT64_C(3)*1024*1024*1024;
 static constexpr uint32_t Base=0x04000000,Context=8192;
 enum {WeightKind=0,RecurrentKind=1,ConvKind=2,KvKind=3,WorkspaceKind=4};
 PioneerQ38CanonicalModelPlan()=default;
 PioneerQ38CanonicalModelPlan(const PioneerQ38CanonicalModelPlan&)=delete;
 PioneerQ38CanonicalModelPlan& operator=(const PioneerQ38CanonicalModelPlan&)=delete;
 bool Build() {
  if(attempted)return false;
  attempted=true;
  // Same semantic order as the independently implemented authenticated
  // host loader:global tensors first,then exact layer/role order.
  for(unsigned ordinal=0;ordinal<=64;ordinal++) {
   unsigned layer=ordinal?ordinal-1:64;
   for(unsigned role=0;;role++) {
    Shape s={};if(!Tensor(layer,role,s))break;
    if(layer<64 && role>=2 && role<=4)firstFfn[layer][role-2]=count;
    if(!Stripe(WeightKind,layer,role,s.type,s.outputs,RowBytes(s),64))return false;
    tensors++;
   }
  }
  if(tensors!=851)return false;
  for(unsigned layer=0;layer<64;layer++) {
   if(layer%4==3){if(!Stripe(KvKind,layer,0,0,8,Context*256*2,1))return false;}
   else {
    for(unsigned slot=0;slot<2;slot++)if(!Stripe(RecurrentKind,layer,slot,0,48,65536,1))return false;
    if(!Stripe(ConvKind,layer,0,0,10240,12,64))return false;
   }
  }
  if(recurrentCount!=672)return false;
  for(unsigned bank=0;bank<7;bank++) {
   constexpr unsigned bytes=67108864;
   if(count>=6144 || used[bank]>Capacity || bytes>Capacity-used[bank])return false;
   workspace[bank]=count;parts[count++]={WorkspaceKind,64,0,0,bank,0,1,bytes,uint32_t(used[bank]),bytes,bytes};used[bank]+=bytes;
  }
  layout=OriginalLayout;ready=true;return true;
 }
private:
 friend class PioneerQ38HybridUploadReceipt;
#ifdef PIONEER_Q38_QGW8_LAYOUT_INSPECTION
 friend class PioneerQ38Qgw8UploadReceipt;
 // Translate the independently constructed QGW8 receipt layout into the
 // canonical shape used by the runtime, including relocated recurrent
 // regions. Original type/rowBytes describe logical source geometry, NOT
 // the encoding of converted bytes. This is inspection data only: it does
 // not authorize a native worker, upload, initialization or execution.
 template<class Layout> bool BuildQgw8Inspection(const Layout& proposal) {
  if(!proposal.Ready() || !Build())return false;
  ready=false;layout=UnbuiltLayout;
  if(proposal.Count()!=count)return false;
  uint64_t next[7]={};unsigned region=0,four=0,eight=0,head=0;
  for(unsigned i=0;i<count;i++){
   auto& p=parts[i];const auto* q=proposal.At(i);
   if(!q || q->original!=i || p.bank>=7 || q->bank!=p.bank || q->offset!=next[p.bank]
    || !q->bytes || q->bytes>q->allocated || (q->offset&4095) || (q->allocated&4095)
    || q->allocated-q->bytes>=4096 || next[p.bank]>Capacity || q->allocated>Capacity-next[p.bank])return false;
   uint32_t expected=Layout::Original;uint64_t bytes=p.bytes;
   if(p.kind==WeightKind && p.layer<64 && p.role>=2 && p.role<=4){
    const uint64_t width=p.role==4?17408:5120;
    if(p.type!=12 || !p.rows || ((p.firstRow|p.rows)&31) || p.rowBytes!=width*9/16
     || uint64_t(p.rows)*p.rowBytes!=p.bytes)return false;
    if(p.layer>=50 && p.layer<=60){expected=Layout::Bfp8;bytes=uint64_t(p.rows)*width*17/16;eight++;}
    else{expected=Layout::Bfp4;four++;}
   }else if(p.kind==WeightKind && p.layer==64 && p.role==1){
    if(p.type!=14 || !p.rows || ((p.firstRow|p.rows)&63) || p.rowBytes!=4240
     || uint64_t(p.rows)*p.rowBytes!=p.bytes)return false;
    expected=Layout::HeadBfp8;bytes=uint64_t(p.rows)*5440;head++;
   }
   if(q->encoding!=expected || bytes!=q->bytes || ((bytes+4095)&~UINT64_C(4095))!=q->allocated)return false;
   if(p.kind==RecurrentKind){
    if(region>=recurrentCount)return false;auto& r=recurrent[region++];
    if(r.layer!=p.layer || r.slot!=p.role || r.firstHead!=p.firstRow || r.heads!=p.rows
     || r.bank!=p.bank || r.offset!=p.offset)return false;
    r.offset=q->offset;
   }
   p.offset=q->offset;p.bytes=q->bytes;p.allocated=q->allocated;next[p.bank]+=q->allocated;
  }
  if(four!=1113 || eight!=231 || head!=7 || region!=672)return false;
  for(unsigned b=0;b<7;b++){
   if(workspace[b]>=count || parts[workspace[b]].kind!=WorkspaceKind || parts[workspace[b]].bank!=b
    || next[b]!=proposal.Used(b))return false;
   used[b]=next[b];
  }
  layout=Qgw8Layout;ready=true;return true;
 }
#endif
 // Fixed full-layout repack for the distinct hybrid upload receipt. Start
 // from the fixed canonical order and banks; enlarge exactly the thirteen
 // BFP8 FFN layers and relocate every following part, including state and
 // workspace. Existing rowBytes/type retain the original logical Q4 job
 // geometry: these fields MUST NOT authorize a Q4 worker on converted bytes.
 // One-shot failure leaves the object unavailable. It does not authenticate or upload weights, select worker
 // firmware, initialize state, own memory, or launch inference.
 bool BuildHybridLayout() {
  if(!Build())return false;
  ready=false;layout=UnbuiltLayout;
  uint64_t next[7]={};unsigned converted=0,region=0;
  for(unsigned i=0;i<count;i++){
   auto& p=parts[i];if(p.bank>=7 || !p.bytes || p.bytes>p.allocated
    || (p.offset&4095) || (p.allocated&4095) || p.allocated-p.bytes>=4096)return false;
   const bool eight=p.kind==WeightKind && p.layer>=48 && p.layer<=60 && p.role>=2 && p.role<=4;
   uint64_t bytes=p.bytes,allocated=p.allocated;
   if(eight){
    const uint64_t width=p.role==4?17408:5120;
    if(p.type!=12 || !p.rows || ((p.firstRow|p.rows)&31) || p.rowBytes!=width*9/16
     || uint64_t(p.rows)*p.rowBytes!=p.bytes)return false;
    bytes=uint64_t(p.rows)*width*17/16;allocated=(bytes+4095)&~uint64_t(4095);converted++;
   }
   if(!bytes || bytes>UINT32_MAX || allocated>UINT32_MAX || next[p.bank]>Capacity
    || allocated>Capacity-next[p.bank])return false;
   if(p.kind==RecurrentKind){
    if(region>=recurrentCount)return false;
    auto& r=recurrent[region++];
    if(r.layer!=p.layer || r.slot!=p.role || r.firstHead!=p.firstRow || r.heads!=p.rows
     || r.bank!=p.bank || r.offset!=p.offset)return false;
    r.offset=uint32_t(next[p.bank]);
   }
   p.offset=uint32_t(next[p.bank]);p.bytes=uint32_t(bytes);p.allocated=uint32_t(allocated);
   next[p.bank]+=allocated;
  }
  if(converted!=13*3*7 || region!=672)return false;
  for(unsigned bank=0;bank<7;bank++){
   if(workspace[bank]>=count || parts[workspace[bank]].kind!=WorkspaceKind
    || parts[workspace[bank]].bank!=bank)return false;
   used[bank]=next[bank];
  }
  layout=HybridLayout;ready=true;return true;
 }
public:
#ifdef PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC
 bool BuildHybridDiagnostic() { return BuildHybridLayout(); }
#endif
 bool Ready()const{return ready;}
 // Immutable geometry identity only: it does not authenticate uploaded bytes.
 bool OriginalFormatLayout()const{return ready && layout==OriginalLayout;}
 bool HybridFormatLayout()const{return ready && layout==HybridLayout;}
#ifdef PIONEER_Q38_QGW8_LAYOUT_INSPECTION
 bool Qgw8InspectionLayout()const{return ready && layout==Qgw8Layout;}
#endif
 unsigned Count()const{return ready?count:0;}
 const PioneerQ38CanonicalPart* Part(unsigned i)const{return ready && i<count?&parts[i]:nullptr;}
 const PioneerQ38CanonicalPart* FfnStripe(unsigned layer,unsigned role,unsigned stripe)const {
  if(!ready || layer>=64 || role>=3 || stripe>=7)return nullptr;
  return &parts[firstFfn[layer][role]+stripe];
 }
 const PioneerQ38CanonicalPart* Workspace(unsigned bank)const{return ready && bank<7?&parts[workspace[bank]]:nullptr;}
 const PioneerQ38GdnRegion* RecurrentRegions()const{return ready?recurrent:nullptr;}
 unsigned RecurrentCount()const{return ready?recurrentCount:0;}
 uint64_t Used(unsigned bank)const{return ready && bank<7?used[bank]:0;}
 // General learned-matrix geometry for the SAME future persistent family,
 // including small recurrent projections and multi-wave vocabulary rows.
 // Never an input/source/initialized page or native execution grant.
 bool ProjectionGeometry(unsigned selected,unsigned role,unsigned& type,unsigned& inputs,unsigned& outputs)const {
  Shape s={};if(!ready || !Tensor(selected,role,s) || (s.type!=8 && s.type!=12 && s.type!=14))return false;
  type=s.type;inputs=s.inputs;outputs=s.outputs;return true;
 }
 const PioneerQ38CanonicalPart* ProjectionStripe(unsigned selected,unsigned role,unsigned stripe)const {
  unsigned type=0,inputs=0,outputs=0;
  if(!ProjectionGeometry(selected,role,type,inputs,outputs) || stripe>=(outputs/64<7?1U:7U))return nullptr;
  for(unsigned i=0;i<count;i++){
   const auto& p=parts[i];if(p.kind!=WeightKind || p.layer!=selected || p.role!=role)continue;
   if(!stripe)return &p;
   stripe--;
  }
  return nullptr;
 }
 bool ProjectionPagesPresent(unsigned selected,unsigned role,const uint8_t* initialized,unsigned pages)const {
  unsigned type=0,inputs=0,outputs=0;
  if(!ProjectionGeometry(selected,role,type,inputs,outputs) || !initialized || pages!=PioneerResidentBanks::Pages)return false;
  for(unsigned stripe=0;stripe<(outputs/64<7?1U:7U);stripe++){
   const auto* p=ProjectionStripe(selected,role,stripe);if(!p)return false;
   unsigned at=p->bank*unsigned(Capacity/4096)+p->offset/4096;
   for(unsigned i=0;i<p->allocated/4096;i++)if(initialized[at+i]!=1)return false;
  }
  return true;
 }
 // Exact canonical per-layer learned geometry,NOT a page/producer grant.
 // A same-shaped OTHER layer or a shifted learned range cannot pass.
 bool FfnSpan(unsigned layer,unsigned role,unsigned bank,uint64_t offset,unsigned first,unsigned rows)const {
  if(!ready || layer>=64 || role>=3 || bank>=7 || !rows || ((first|rows)&15))return false;
  for(unsigned stripe=0;stripe<7;stripe++) {
   const auto& p=*FfnStripe(layer,role,stripe);
   if(p.bank==bank && first>=p.firstRow && first-p.firstRow<=p.rows && rows<=p.rows-(first-p.firstRow)
    && offset==uint64_t(p.offset)+uint64_t(first-p.firstRow)*p.rowBytes)return true;
  }
  return false;
 }
 // Presence in the ACTUAL retained upload ledger is necessary,but still
 // not authentication of a model/source or proof of an input producer.
 // Refusal never writes the caller's bitmap or performs any device IO.
 bool FfnPagesPresent(unsigned layer,const uint8_t* initialized,unsigned pages)const {
  if(!ready || layer>=64 || !initialized || pages!=PioneerResidentBanks::Pages)return false;
  for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++) {
   const auto& p=*FfnStripe(layer,role,stripe);unsigned first=p.bank*unsigned(Capacity/4096)+p.offset/4096;
   for(unsigned i=0;i<p.allocated/4096;i++)if(initialized[first+i]!=1)return false;
  }
  return true;
 }
};
static_assert(sizeof(PioneerQ38CanonicalModelPlan)>262144 && sizeof(PioneerQ38CanonicalModelPlan)<327680,"complete canonical layout is HEAP ONLY");
#endif
