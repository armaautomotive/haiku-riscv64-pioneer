/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_QGW8_TRACE_ASSEMBLER_INSPECTION) || PIONEER_Q38_QGW8_TRACE_ASSEMBLER_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION) || defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) \
 || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) || defined(PIONEER_Q38_QGW8_HIDDEN_NATIVE_129) \
 || defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129) || defined(PIONEER_Q38_QGW8_LAYER_TRACE_IMAGE) \
 || defined(PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION) \
 || defined(PIONEER_Q38_QGW8_TRACE_READ_INSPECTION) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Trace assembly is separate offline DATA inspection only
#endif
#ifndef PIONEER_Q38_QGW8_LAYER_TRACE_ASSEMBLER_H
#define PIONEER_Q38_QGW8_LAYER_TRACE_ASSEMBLER_H
#include "PioneerQ38Qgw8LayerTraceGeometry.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>

// Orders raw DATA only. The caller-provided source key is not authenticated
// provenance, a driver generation, a held-owner proof, or device authority.
// Native transport, safe lifetime, FINISH and close remain separate work.
class PioneerQ38Qgw8LayerTraceAssembler {
 using G=PioneerQ38Qgw8LayerTraceGeometry::Chunk;
 static constexpr size_t Total=64U*5120U;
 std::unique_ptr<uint32_t[]> data;
 const uint64_t sourceKey;const uint32_t workspace;
 uint32_t cursor=0;bool failed=false,delivered=false;
 bool Refuse(){failed=true;if(data)std::fill_n(data.get(),Total,0);return false;}
 bool Overlaps(const void* ptr,size_t bytes)const{
  const uintptr_t begin=reinterpret_cast<uintptr_t>(ptr),self=reinterpret_cast<uintptr_t>(this),
   internal=reinterpret_cast<uintptr_t>(data.get());
  if(!ptr || !bytes || (begin&3) || begin>UINTPTR_MAX-bytes || self>UINTPTR_MAX-sizeof(*this)
   || internal>UINTPTR_MAX-Total*sizeof(uint32_t))return true;
  return (begin<self+sizeof(*this) && self<begin+bytes)
   || (begin<internal+Total*sizeof(uint32_t) && internal<begin+bytes);
 }
public:
 explicit PioneerQ38Qgw8LayerTraceAssembler(uint64_t key,uint32_t base):
  data(new(std::nothrow) uint32_t[Total]()),sourceKey(key),workspace(base){
  G first={};if(!data || !key || !PioneerQ38Qgw8LayerTraceGeometry::Describe(base,0,first))failed=true;
 }
 PioneerQ38Qgw8LayerTraceAssembler(const PioneerQ38Qgw8LayerTraceAssembler&)=delete;
 PioneerQ38Qgw8LayerTraceAssembler& operator=(const PioneerQ38Qgw8LayerTraceAssembler&)=delete;
 bool Failed()const{return failed;}uint32_t Cursor()const{return cursor;}
 bool ExternalBufferSafe(const void* p,size_t bytes)const{return !Overlaps(p,bytes);}
 bool Invalidate(){return Refuse();} // DATA fail-closed only, not device cleanup
 bool Append(uint64_t key,uint32_t ordinal,const uint32_t* words,size_t count){
  G chunk={};
  if(failed || delivered)return false;
  if(key!=sourceKey || ordinal!=cursor || !PioneerQ38Qgw8LayerTraceGeometry::Describe(workspace,ordinal,chunk)
   || count!=chunk.words || Overlaps(words,count*sizeof(uint32_t)))return Refuse();
  const size_t first=size_t(chunk.layer)*5120+chunk.firstWord;
  if(first>Total || count>Total-first)return Refuse();
  std::memcpy(data.get()+first,words,count*sizeof(uint32_t));cursor++;return true;
 }
 bool Copy(uint32_t* output,size_t count){
  if(failed || delivered || cursor!=PioneerQ38Qgw8LayerTraceGeometry::Chunks || count!=Total
   || Overlaps(output,count*sizeof(uint32_t)))return false;
  std::memcpy(output,data.get(),Total*sizeof(uint32_t));delivered=true;return true;
 }
};
#endif
