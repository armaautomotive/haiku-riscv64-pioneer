/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_LEARNED_BYTES_TEST_H
#define PIONEER_Q38_RESIDENT_OUTPUT_LEARNED_BYTES_TEST_H
#include "pioneer_q38_projection_execution_machine.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputCohortShape.h"
// TEST original-byte endpoints ONLY. Never initializes native owners,state,
// receipts or products. A present source MUST succeed: no synthetic fallback.
template<class Put>bool ResidentOutputImportLearnedBytes(const uint32_t* manifest,
 ProjectionExecutionLearnedBytes& source,Put&& put)
{
 if(!PioneerQ38ResidentCohortShape(manifest) || !source.Ready())return false;
 auto copy=[&](unsigned route,uint32_t address,unsigned bytes){
  uint8_t data[512];
  if(!source.Ready() || !source.Read(route,address,data,bytes) || !source.Ready())return false;
  for(unsigned i=0;i<bytes;i+=4){
   uint32_t value=uint32_t(data[i])|(uint32_t(data[i+1])<<8)|(uint32_t(data[i+2])<<16)|(uint32_t(data[i+3])<<24);
   put(route,address+i,value);
  }return true;
 };
 for(unsigned worker=0;worker<120;worker++){
  const auto* table=PioneerQ38CohortConv(manifest,worker);
  for(unsigned job=0;job<PioneerQ38ConvBatchCount(worker);job++){
   const auto* d=table+job*64;if(!copy(d[13],d[12],256))return false;
  }
 }
 for(unsigned worker=1;worker<=96;worker++){
  const auto* in=PioneerQ38CohortGdn(manifest,worker);const auto* gated=in+128;
  if(!copy(in[13],in[12],64) || !copy(in[41],in[15],64) || !copy(gated[13],gated[12],512))return false;
 }
 return source.Ready();
}
static inline bool ResidentOutputImportMatrixBytes(const uint32_t* part,
 ProjectionExecutionLearnedBytes& source,std::vector<std::vector<uint8_t>>& tiles)
{
 if(!part || part[1]>=7 || part[2]<2 || part[2]>3 || part[3] || part[6] || part[7]
  || part[4]!=PioneerQ38ProjectionRoute(part[1]) || !part[5] || (part[0]&63)
  || part[0]<0x04000000 || part[0]>0xc0000000 || !source.Ready())return false;
 std::vector<std::vector<uint8_t>> stage(part[2]*192,std::vector<uint8_t>(576));
 for(unsigned tile=0;tile<stage.size();tile++){
  uint64_t address=uint64_t(part[0])+tile*UINT64_C(576);
  if(address>UINT32_MAX || !source.Ready() || !source.Read(part[4],uint32_t(address),stage[tile].data(),576) || !source.Ready())return false;
 }
 if(!source.Ready())return false;
 tiles.swap(stage);return true; // byte cache only,not a producer/compute result
}
#endif
