/* SPDX-License-Identifier: MIT */
// Actual extracted owner/read methods over MOCK lifecycle and NoC only.
#define PIONEER_Q38_TRACE_TEST 1
#define PIONEER_Q38_QGW8_HIDDEN_NATIVE_129 1
#define main previous_hidden_test_main
#include "pioneer_q38_hidden_read_method_test.cpp"
#undef main

static bool ReadTrace(Fixture& f,unsigned ordinal,PioneerQ38TokenHiddenReadResult& r){
 PioneerQ38Qgw8LayerTraceGeometry::Chunk chunk={};
 if(!PioneerQ38Qgw8LayerTraceGeometry::Describe(f.owner->plan.Workspace(),ordinal,chunk))return false;
 return f.owner->ReadTraceChunk(f.io,ordinal,f.output.data(),chunk.words,r);
}
int main(){
 namespace G=PioneerQ38Qgw8LayerTraceGeometry;unsigned observed=0,refused=0;
 for(unsigned rollover=0;rollover<2;rollover++){
  Fixture f;if(rollover)for(unsigned w=0;w<120;w++)for(unsigned i=0;i<8;i++)
   f.owner->cores[w].counter[i]=f.owner->terminalCounters[w][i]=0xfffffff0U;
  for(unsigned ordinal=0;ordinal<G::Chunks;ordinal++){
   f.output.fill(0);PioneerQ38TokenHiddenReadResult r;assert(ReadTrace(f,ordinal,r));
   G::Chunk chunk={};assert(G::Describe(f.owner->plan.Workspace(),ordinal,chunk));
   assert(r.ready && !r.unsafe && r.phase==4 && r.part==ordinal && r.firstWord==chunk.layer*G::Words+chunk.firstWord
    && r.wordCount==chunk.words && r.completedLayers==64 && r.position==0 && r.terminalKind==1 && r.committed==1
    && r.packets==1 && r.bytesRead==chunk.words*4 && f.owner->traceReadCursor==ordinal+1);
   for(unsigned i=0;i<chunk.words;i++)assert(f.output[i]==f.io.hidden[chunk.firstWord+i]);
   for(unsigned i=chunk.words;i<4096;i++)assert(!f.output[i]);observed++;
  }
  f.output.fill(0);PioneerQ38TokenHiddenReadResult r;unsigned calls=f.io.calls;
  assert(!f.owner->ReadTraceChunk(f.io,128,f.output.data(),4096,r) && f.io.calls==calls && !f.owner->unsafe);
 }
 for(unsigned mode=0;mode<7;mode++){
  Fixture f;PioneerQ38TokenHiddenReadResult r;unsigned words=4096,ordinal=0;uint32_t* out=f.output.data();
  if(mode==0)ordinal=1;if(mode==1)words=1024;if(mode==2)out=nullptr;
  if(mode==3)f.output[0]=1;if(mode==4)f.owner->observedTerminal[6]=63;
  if(mode==5)f.owner->drained=false;if(mode==6)f.owner->plan.workspace++;
  const auto before=f.output;unsigned calls=f.io.calls;
  assert(!f.owner->ReadTraceChunk(f.io,ordinal,out,words,r) && f.io.calls==calls
   && !f.owner->unsafe && f.output==before);refused++;
 }
 {
  Fixture f;PioneerQ38TokenHiddenReadResult r;const unsigned calls=f.io.calls;
  assert(!f.owner->ReadTraceChunk(f.io,0,reinterpret_cast<uint32_t*>(f.owner.get()),4096,r)
   && f.io.calls==calls && !f.owner->unsafe);refused++;
 }
 {
  Fixture f;auto* r=new(f.output.data()) PioneerQ38TokenHiddenReadResult;const unsigned calls=f.io.calls;
  assert(!f.owner->ReadTraceChunk(f.io,0,f.output.data(),4096,*r) && f.io.calls==calls && !f.owner->unsafe);refused++;
 }
 for(unsigned stop:{0U,1U,63U,126U,127U}){
  Fixture f;
  for(unsigned i=0;i<stop;i++){f.output.fill(0);PioneerQ38TokenHiddenReadResult r;assert(ReadTrace(f,i,r));}
  f.output.fill(0);f.io.fault=2;f.io.faultBegin=stop+1;PioneerQ38TokenHiddenReadResult r;
  assert(!ReadTrace(f,stop,r) && f.owner->unsafe && r.unsafe && !r.ready && f.owner->traceReadCursor==stop);
  for(auto v:f.output)assert(!v);const unsigned calls=f.io.calls;
  PioneerQ38TokenHiddenReadResult retry;assert(!ReadTrace(f,stop,retry) && f.io.calls==calls);refused++;
 }
 for(unsigned mode=1;mode<=11;mode++){
  Fixture f;f.io.fault=mode;f.io.faultWord=0;PioneerQ38TokenHiddenReadResult r;
  assert(!ReadTrace(f,0,r) && f.owner->unsafe && r.unsafe && !r.ready && !f.owner->traceReadCursor);
  for(auto v:f.output)assert(!v);const unsigned calls=f.io.calls;
  PioneerQ38TokenHiddenReadResult retry;assert(!ReadTrace(f,0,retry) && f.io.calls==calls);refused++;
 }
 for(unsigned at:{1U,128U,129U,1000U,4224U}){
  Fixture f;f.io.failRead=at;PioneerQ38TokenHiddenReadResult r;
  assert(!ReadTrace(f,0,r) && f.owner->unsafe && r.unsafe && !r.ready && !f.owner->traceReadCursor);
  for(auto v:f.output)assert(!v);const unsigned calls=f.io.calls;
  PioneerQ38TokenHiddenReadResult retry;assert(!ReadTrace(f,0,retry) && f.io.calls==calls);refused++;
 }
 printf("PASS extracted trace read: %u ordered mock chunks, counter wrap, %u early/fault refusals; NO REAL OWNER, ROOT, DEVICE OR SPEED\n",observed,refused);
}
