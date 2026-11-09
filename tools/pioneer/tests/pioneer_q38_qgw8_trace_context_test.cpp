/* SPDX-License-Identifier: MIT */
// Actual extracted wrapper with deliberately synthetic owner and IO endpoints.
#include "../PioneerQ38Qgw8LayerTraceGeometry.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>

struct PioneerQ38TokenHiddenReadResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,part=0;
 uint32_t firstWord=0,wordCount=0,position=0,completedLayers=0,terminalKind=0,committed=0,packets=0,bytesRead=0;
};
struct PioneerResidentBanks {enum {Pages=4096};};
struct PioneerQ38TokenRope {inline static uint32_t Data[16]={};enum {Bytes=sizeof(Data)};};
struct Startup {bool permit=true;uint32_t Workspace()const{return 0x4000000;}
 bool PublishedOutput(const void*,size_t)const{return permit;}};
struct IO {bool source=true;unsigned calls=0;bool TokenModelSourceScope(bool){return source;}};
struct Runtime {bool drained=true,permit=true;unsigned calls=0;
 bool CommittedRecordsRead()const{return drained;}bool DispatchOutput(const void*,size_t)const{return permit;}
 bool ReadTraceChunk(IO&,unsigned ordinal,uint32_t* output,unsigned words,PioneerQ38TokenHiddenReadResult& result){
  if(ordinal!=calls)return false;calls++;output[words-1]=42;
  result.started=result.ready=1;result.phase=4;result.part=ordinal;result.firstWord=(ordinal/2)*5120+(ordinal%2)*4096;
  result.wordCount=words;result.completedLayers=64;result.terminalKind=result.committed=result.packets=1;result.bytesRead=words*4;return true;}};
struct PioneerQ38TokenNativeState {enum {RootHeld=1};unsigned phase=RootHeld;bool held=true;
 Startup* startup=nullptr;Runtime* runtime=nullptr;int* memory=nullptr;int* participants=nullptr;
 bool TerminalHeld()const{return held;}};
struct PioneerStoreContext {IO io;PioneerQ38TokenNativeState* q38TokenNative=nullptr;uint8_t* initialized=nullptr;uint32_t* hashes=nullptr;};
static bool Q38TokenExecutionOutput(const PioneerStoreContext&,const void*,size_t){return true;}
#include "actual_trace_context_wrapper.inc"

int main(){
 Startup startup;Runtime runtime;int memory=0,participants=0;
 PioneerQ38TokenNativeState state;state.startup=&startup;state.runtime=&runtime;state.memory=&memory;state.participants=&participants;
 std::array<uint8_t,PioneerResidentBanks::Pages> initialized={};std::array<uint32_t,PioneerResidentBanks::Pages> hashes={};
 PioneerStoreContext c;c.q38TokenNative=&state;c.initialized=initialized.data();c.hashes=hashes.data();
 std::array<uint32_t,4096> output={};PioneerQ38TokenHiddenReadResult r;
 auto check=[&](unsigned ordinal,unsigned words){assert(!ReadQ38TokenNativeTraceChunk(c,ordinal,output.data(),words,r));assert(!runtime.calls && output[0]==0);};
 check(128,4096);check(0,1024);check(1,4096);
 c.q38TokenNative=nullptr;check(0,4096);c.q38TokenNative=&state;
 assert(!ReadQ38TokenNativeTraceChunk(c,0,nullptr,4096,r) && !runtime.calls);
 state.phase=0;check(0,4096);state.phase=state.RootHeld;
 state.held=false;check(0,4096);state.held=true;
 runtime.drained=false;check(0,4096);runtime.drained=true;
 state.startup=nullptr;check(0,4096);state.startup=&startup;
 startup.permit=false;check(0,4096);startup.permit=true;
 runtime.permit=false;check(0,4096);runtime.permit=true;
 c.io.source=false;check(0,4096);c.io.source=true;
 output[0]=1;assert(!ReadQ38TokenNativeTraceChunk(c,0,output.data(),4096,r) && !runtime.calls);output[0]=0;
 assert(!ReadQ38TokenNativeTraceChunk(c,0,reinterpret_cast<uint32_t*>(&c),4096,r) && !runtime.calls);
 assert(!ReadQ38TokenNativeTraceChunk(c,0,reinterpret_cast<uint32_t*>(&r),4096,r) && !runtime.calls);
 r.started=1;check(0,4096);r.started=0;
 for(unsigned ordinal=0;ordinal<128;ordinal++){
  output.fill(0);r=PioneerQ38TokenHiddenReadResult{};const unsigned words=ordinal%2?1024:4096;
  assert(ReadQ38TokenNativeTraceChunk(c,ordinal,output.data(),words,r) && runtime.calls==ordinal+1 && output[words-1]==42);
 }
 puts("PASS actual trace context wrapper with synthetic owner: held/shape/alias/zero/source gates; NO REAL IO");
 return 0;
}
