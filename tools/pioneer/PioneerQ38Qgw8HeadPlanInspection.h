/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QgweNativeInspectionRole.h"
#if defined(PIONEER_Q38_QGWE_BATCH_DENSE_PLAN) && (PIONEER_Q38_QGWE_BATCH_DENSE_PLAN != 1 \
 || !defined(PIONEER_Q38_QGWD_PLAN_SIMULATION) || PIONEER_Q38_QGWD_PLAN_SIMULATION != 1 \
 || ((defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG)) && !defined(PIONEER_Q38_QGWE_NATIVE_INSPECTION)) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE))
#error QGWE head metadata is an isolated simulated plan only
#endif
#ifndef PIONEER_Q38_QGW8_HEAD_PLAN_INSPECTION_H
#define PIONEER_Q38_QGW8_HEAD_PLAN_INSPECTION_H
#if !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1
#error QGW8 runtime head plan requires the non-loadable inspection layout
#endif
#include "PioneerQ38GlobalChainLayout.h"
#include "PioneerQ38QgwaBatchRootReceiptRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#if !defined(PIONEER_Q38_QGW8_CANONICAL_PRESENT) || PIONEER_Q38_QGW8_CANONICAL_PRESENT != 1
#error QGW8 inspection role must be selected before the canonical model header
#endif

// Serialize the matrix-head descriptor from the SAME QGW8 canonical layout
// used by the retained model owner. Geometry only, never a launch grant.
// The root grammar is independently checked against the actual emitted ROOT
// in tests; converted data/source authenticity and device bytes remain owed.
class PioneerQ38Qgw8HeadPlanInspection {
 uint32_t data[128]={},boot[64]={};const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 bool attempted=false,ready=false;
 static bool Make(const PioneerQ38CanonicalModelPlan& c,unsigned bank,const uint32_t* b,uint32_t* out){
  namespace C=PioneerQ38GlobalChainLayout;
  if(!out || !c.Qgw8InspectionLayout() || bank>=7 || !C::BootShape(b))return false;
  const auto* w=c.Workspace(bank);
  if(!w || w->kind!=c.WorkspaceKind || w->bank!=bank || w->bytes!=C::WorkspaceBytes
   || w->allocated!=C::WorkspaceBytes || (w->offset&4095) || w->offset>c.Capacity-C::WorkspaceBytes)return false;
  const uint32_t base=c.Base+w->offset;
  uint32_t staged[128]={};
  const uint32_t fixed[]={0x38485651,1,128,bank,base,base+C::HeadOffset,0,0,5120,248320,973,9,119,8,5440,20480};
  for(unsigned i=0;i<16;i++){staged[i]=fixed[i];staged[16+i]=b[16+i];}
  for(unsigned i=0;i<8;i++)staged[32+i]=b[8+i];
  // ROOT checks this witness against its selected head-worker grammar.
  const uint32_t witness[]={
#if defined(PIONEER_Q38_QGWE_BATCH_DENSE_PLAN)
   0x45574751,
#elif defined(PIONEER_Q38_QGWD_PLAN_SIMULATION)
   0x44574751,
#elif defined(PIONEER_Q38_QGWC_PLAN_SIMULATION) || defined(PIONEER_Q38_QGWC_NATIVE_136)
   0x43574751,
#elif defined(PIONEER_Q38_QGWA_BATCH_ROOT_RECEIPT_INSPECTION)
   0x41475751,
#else
   0x38574751,
#endif
   C::Magic,64,0,32,8,0,1};
  for(unsigned i=0;i<8;i++)staged[40+i]=witness[i];
  unsigned norms=0,stripes=0,rows=0;
  for(unsigned i=0;i<c.Count();i++){
   const auto* p=c.Part(i);if(!p)return false;
   if(p->kind!=c.WeightKind || p->layer!=64)continue;
   if(p->role==2){
    if(++norms!=1 || p->bank>=7 || p->type || p->firstRow || p->rows!=1 || p->rowBytes!=20480
     || p->bytes!=20480 || p->allocated!=20480 || (p->offset&63) || p->offset>c.Capacity-20480)return false;
    staged[6]=p->bank;staged[7]=c.Base+p->offset;
   }else if(p->role==1){
    if(p->bank>=7 || p->type!=14 || p->rowBytes!=4240 || !p->rows || ((p->firstRow|p->rows)&63)
     || (p->rows!=35456 && p->rows!=35520) || p->firstRow!=rows || p->rows>248320-rows
     || uint64_t(p->rows)*5440!=p->bytes || p->allocated!=((uint64_t(p->bytes)+4095)&~UINT64_C(4095))
     || (p->offset&4095) || p->allocated>c.Capacity || p->offset>c.Capacity-p->allocated)return false;
    auto* stripe=staged+64+p->bank*8;if(stripe[3])return false;
    const uint32_t values[]={p->bank,c.Base+p->offset,p->firstRow,p->rows,5440,p->bytes,p->allocated,139};
    for(unsigned j=0;j<8;j++)stripe[j]=values[j];rows+=p->rows;stripes++;
   }
  }
  if(norms!=1 || stripes!=7 || rows!=248320)return false;
  if(staged[6]==bank && staged[7]<base+C::WorkspaceBytes && base<staged[7]+20480)return false;
  for(unsigned i=0;i<7;i++){
   const auto* p=staged+64+i*8;
   if(!p[3] || (i==bank && p[1]<base+C::WorkspaceBytes && base<p[1]+p[6])
    || (i==staged[6] && p[1]<staged[7]+20480 && staged[7]<p[1]+p[6]))return false;
  }
  for(unsigned i=0;i<128;i++)out[i]=staged[i];
  return true;
 }
public:
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned bank,const uint32_t* rootBoot){
  if(attempted)return false;attempted=true;
  if(!Make(c,bank,rootBoot,data))return false;
  for(unsigned i=0;i<64;i++)boot[i]=rootBoot[i];
  canonical=&c;ready=true;return true;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  if(!ready || canonical!=&c)return false;
  uint32_t expected[128];if(!Make(c,data[3],boot,expected))return false;
  for(unsigned i=0;i<128;i++)if(data[i]!=expected[i])return false;
  return true;
 }
 const uint32_t* Data()const{return ready?data:nullptr;}
};
#endif
