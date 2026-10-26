/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_LEARNED_BYTES_TEST_H
#define PIONEER_Q38_RESIDENT_LAYER_LEARNED_BYTES_TEST_H
#include "pioneer_q38_resident_output_learned_bytes.h" // generic Conv/GDN learned BYTES only,not old native ownership
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerPlan.h"
// TEST original-byte endpoints ONLY. These helpers cannot mint native
// owners, initialization, products, ready bits, receipts or cleanup grants.
// A present original source MUST satisfy EVERY request; no synthetic fallback.
// Callers must abort before executing an incompletely imported byte fixture.
template<class Put>bool ResidentLayerImportLearnedBytes(const PioneerQ38ResidentLayerPlan& plan,
 ProjectionExecutionLearnedBytes& source,Put&& put){
 const auto* m=plan.Manifest();const auto* q=plan.PostManifest();const auto* l=plan.LayerManifest();
 if(!m || !q || !l || m[3] || m[4] || m[14] || !plan.CanonicalRecipe()
  || !PioneerQ38ResidentLayerShape(l,m,q) || !source.Ready())return false;
 if(!ResidentOutputImportLearnedBytes(m,source,put))return false;
 std::vector<uint8_t> bytes(16384);
 for(unsigned offset:{0U,16384U}){
  const unsigned length=offset?4096:16384;
  if(!source.Ready() || !source.Read(l[14],l[13]+offset,bytes.data(),length) || !source.Ready())return false;
  for(unsigned i=0;i<length;i+=4){
   uint32_t value=uint32_t(bytes[i])|(uint32_t(bytes[i+1])<<8)|(uint32_t(bytes[i+2])<<16)|(uint32_t(bytes[i+3])<<24);
   put(l[14],l[13]+offset+i,value);
  }
 }
 return source.Ready();
}
static inline bool ResidentLayerImportFfnMatrixBytes(const PioneerQ38ResidentLayerPlan& plan,unsigned role,unsigned worker,
 ProjectionExecutionLearnedBytes& source,std::vector<std::vector<uint8_t>>& tiles){
 const auto* m=plan.Manifest();const auto* q=plan.PostManifest();const auto* l=plan.LayerManifest();
 if(role>=3 || !worker || worker>=120 || !m || !q || !l || m[3] || m[4] || m[14] || !plan.CanonicalRecipe()
  || !PioneerQ38ResidentLayerShape(l,m,q) || !source.Ready())return false;
 const auto* r=l+64+PioneerQ38FfnResidualGraphRoleOffset(role);const auto* p=r+16+(worker-1)*8;
 const unsigned blocks=r[3]/256;
 if(!blocks || r[3]!=(role==2?17408U:5120U) || p[1]>=7 || !p[2] || p[2]>16 || p[3] || p[6] || p[7]
  || p[4]!=PioneerQ38ProjectionRoute(p[1]) || !p[5] || (p[0]&63) || p[0]<0x04000000
  || uint64_t(p[0])+uint64_t(p[2])*blocks*2304>UINT64_C(0xc4000000))return false;
 std::vector<std::vector<uint8_t>> stage(p[2]*blocks,std::vector<uint8_t>(2304));
 for(unsigned tile=0;tile<stage.size();tile++){
  const uint64_t address=uint64_t(p[0])+uint64_t(tile)*2304;
  if(address>UINT32_MAX || !source.Ready() || !source.Read(p[4],uint32_t(address),stage[tile].data(),2304) || !source.Ready())return false;
 }
 if(!source.Ready())return false;
 tiles.swap(stage);return true; // original tiled BYTES only;atomic cache replacement,never a matrix result
}
#endif
