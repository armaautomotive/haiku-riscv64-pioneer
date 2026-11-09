/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Qgw8TraceInitRole.h"
#if defined(PIONEER_Q38_QGW8_TRACE_INIT_DATA_INSPECTION)
#if PIONEER_Q38_QGW8_TRACE_INIT_DATA_INSPECTION != 1 || defined(_KERNEL_MODE) \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Trace runtime DATA test is isolated and not a native or Work role
#endif
#elif defined(PIONEER_Q38_QGW8_TRACE_NATIVE_131)
#include "PioneerQ38Qgw8TraceNativeProgramRole.h"
#elif !defined(PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION)
#error Explicit trace runtime DATA or fake-owner inspection required
#endif
#ifndef PIONEER_Q38_QGW8_TRACE_RUNTIME_PLAN_H
#define PIONEER_Q38_QGW8_TRACE_RUNTIME_PLAN_H
#include "PioneerQ38TokenRuntimePlan.h"
#include "PioneerQ38Qgw8LayerTraceGeometry.h"

// DATA proposal only. Existing owner lifecycle must still reserve the SAME
// workspace and verify every write/read before launch. No access is granted.
class PioneerQ38Qgw8TraceRuntimePlan {
 using Base=PioneerQ38TokenRuntimePlan;
 Base base;const PioneerQ38TokenStartupPlan* startup=nullptr;bool attempted=false,ready=false;
 bool Output(const void* output,unsigned bytes)const{
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this),
   rope=reinterpret_cast<uintptr_t>(PioneerQ38TokenRope::Data);
  return output && bytes && out<=UINTPTR_MAX-bytes && self<=UINTPTR_MAX-sizeof(*this)
   && rope<=UINTPTR_MAX-PioneerQ38TokenRope::Bytes
   && !(out<self+sizeof(*this) && self<out+bytes)
   && !(out<rope+PioneerQ38TokenRope::Bytes && rope<out+bytes)
   && startup && startup->PublishedOutput(output,bytes);
 }
public:
 using Chunk=Base::Chunk;
 enum {TraceChunks=80,Chunks=Base::Chunks+TraceChunks,
  PayloadBytes=Base::PayloadBytes+TraceChunks*16384};
 bool Bind(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& s,
  const PioneerQ38GdnIdentity& id,const uint32_t* source){
  namespace G=PioneerQ38Qgw8LayerTraceGeometry;
  if(attempted)return false;attempted=true;
  if(!base.Bind(c,s,id,source))return false;
  const uint32_t workspace=base.Workspace();G::Chunk first={},last={};
  if(!G::Describe(workspace,0,first) || !G::Describe(workspace,G::Chunks-1,last)
   || first.address!=workspace+G::Offset || last.address+last.words*4!=workspace+G::Offset+G::Layers*G::Bytes)return false;
  const uint64_t begin=first.address,end=uint64_t(last.address)+last.words*4;
  for(unsigned i=0;i<Base::Chunks;i++){
   Chunk d;if(!base.Describe(i,d) || (d.bank==base.Bank() && uint64_t(d.address)<end && begin<uint64_t(d.address)+d.bytes))return false;
  }
  startup=&s;ready=true;return true;
 }
 bool MatchesBinding(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& s,
  const PioneerQ38GdnIdentity& id,const uint32_t* source)const{
  return ready && startup==&s && base.MatchesBinding(c,s,id,source);
 }
 bool Describe(unsigned ordinal,Chunk& output)const{
  namespace G=PioneerQ38Qgw8LayerTraceGeometry;
  if(!ready || !startup || ordinal>=Chunks)return false;
  if(ordinal<Base::Chunks)return base.Describe(ordinal,output);
  Chunk witness;if(!base.Describe(0,witness))return false;
  const unsigned offset=(ordinal-Base::Chunks)*16384;
  Chunk next={7,64,offset,base.Bank(),base.Workspace()+G::Offset+offset,16384};
  if(next.bank>=7 || next.address<base.Workspace() || next.address>base.Workspace()+G::WorkspaceBytes-next.bytes)return false;
  output=next;return true;
 }
 bool Read(unsigned ordinal,uint8_t* output,unsigned bytes)const{
  Chunk d;if(!Describe(ordinal,d) || bytes!=d.bytes || !Output(output,bytes))return false;
  if(ordinal<Base::Chunks)return base.Read(ordinal,output,bytes);
  for(unsigned i=0;i<bytes;i++)output[i]=0;return true;
 }
 unsigned Bank()const{return ready?base.Bank():7;}
 unsigned Workspace()const{return ready?base.Workspace():0;}
};
static_assert(PioneerQ38Qgw8TraceRuntimePlan::TraceChunks*16384==
 PioneerQ38Qgw8LayerTraceGeometry::Layers*PioneerQ38Qgw8LayerTraceGeometry::Bytes,"entire trace zeroed once");
#endif
