/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_MANIFEST_H
#define PIONEER_Q38_HYBRID_MANIFEST_H
#include "PioneerQ38CanonicalModelPlan.h"

// Fixed FFN address table derived from a retained hybrid model plan. The
// caller must independently validate the actual upload ledger and owner.
// Geometry alone is neither source authentication nor a launch capability.
struct PioneerQ38HybridManifest {
 enum : unsigned {Words=64*3*7};
 static bool Build(const PioneerQ38CanonicalModelPlan& plan,uint32_t* output,unsigned words){
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),model=reinterpret_cast<uintptr_t>(&plan);
  constexpr size_t bytes=Words*sizeof(uint32_t);
  if(!output || words!=Words || (out&3) || out>UINTPTR_MAX-bytes || model>UINTPTR_MAX-sizeof(plan)
   || (out<model+sizeof(plan) && model<out+bytes))return false;
  const bool legacy=plan.HybridFormatLayout();
  bool qgw8=false;
#ifdef PIONEER_Q38_QGW8_LAYOUT_INSPECTION
  qgw8=plan.Qgw8InspectionLayout();
#endif
  if(!legacy && !qgw8)return false;
  // Validate every entry before publishing any output. The actual owner
  // holds the model and lock across this two-pass derivation.
  for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++){
   unsigned banks=0;const unsigned width=role==4?17408:5120;
   const bool eight=(legacy && layer>=48 && layer<=60) || (qgw8 && layer>=50 && layer<=60);
   for(unsigned stripe=0;stripe<7;stripe++){
    const auto* p=plan.ProjectionStripe(layer,role,stripe);
    if(!p || p->kind!=plan.WeightKind || p->layer!=layer || p->role!=role || p->type!=12
     || p->bank>=7 || (banks&(1U<<p->bank)) || !p->rows || ((p->firstRow|p->rows)&31)
     || p->rowBytes!=width*9/16 || p->bytes!=uint64_t(p->rows)*width*(eight?17:9)/16
     || p->allocated!=((uint64_t(p->bytes)+4095)&~UINT64_C(4095)) || (p->offset&4095)
     || p->allocated>plan.Capacity || p->offset>plan.Capacity-p->allocated
     || p->offset>UINT32_MAX-plan.Base)return false;
    banks|=1U<<p->bank;
   }
   if(banks!=127)return false;
  }
  for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++)for(unsigned stripe=0;stripe<7;stripe++){
   const auto* p=plan.ProjectionStripe(layer,role,stripe);
   output[(layer*3+role-2)*7+p->bank]=plan.Base+p->offset;
  }
  return true;
 }
};
#endif
