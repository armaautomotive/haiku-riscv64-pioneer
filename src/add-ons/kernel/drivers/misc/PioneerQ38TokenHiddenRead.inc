/* SPDX-License-Identifier: MIT */
// Included INSIDE the private native owner. No arbitrary bank,address,length
// or caller-ready input. Exact two-part DATA snapshot of the SAME stopped
// session's canonical final-hidden region. Existing records must be drained.
#include "PioneerQ38TokenHiddenReadRole.h"
static_assert(PioneerQ38TokenHiddenReadSelection==1 || PioneerQ38TokenHiddenReadSelection==2,
 "Explicit private OR separate tag75 held-hidden diagnostic required");
private:
 unsigned hiddenReadCursor=0;
 bool HiddenReadOutput(const void* output,size_t bytes)const{
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),owner=reinterpret_cast<uintptr_t>(this);
  if(!output || !bytes || (out&3) || out>UINTPTR_MAX-bytes || owner>UINTPTR_MAX-sizeof(*this)
   || (out<owner+sizeof(*this) && owner<out+bytes)
   || !startup || !startup->PublishedOutput(output,bytes) || !DispatchOutput(output,bytes))return false;
  const void* objects[]={memory,cohort,canonical,banks,proof,ledger,PioneerQ38TokenRope::Data};
  const size_t sizes[]={sizeof(*memory),sizeof(*cohort),sizeof(*canonical),sizeof(*banks),sizeof(*proof),
   PioneerResidentBanks::Pages,PioneerQ38TokenRope::Bytes};
  for(unsigned i=0;i<7;i++){
   const uintptr_t source=reinterpret_cast<uintptr_t>(objects[i]);
   if(!source || source>UINTPTR_MAX-sizes[i] || (out<source+sizes[i] && source<out+bytes))return false;
  }
  return true;
 } // trusted kernel buffer validation ONLY,not user-pointer validation or execution authority
 template<class IO> bool ReadHiddenPacket(IO& io,unsigned part,PioneerQ38TokenReadResult& r){
  namespace C=PioneerQ38GlobalChainLayout;
  if(part>=2 || part!=hiddenReadCursor || !CommittedRecordsRead() || restoring || l1Restored)return false;
  const unsigned bytes=part?4096U:16384U;
  const uint64_t base=uint64_t(plan.Workspace())+C::R::FinalOffset;
  const uint64_t wide=base+part*UINT64_C(16384);
  if(base>UINT32_MAX || base+20480>uint64_t(plan.Workspace())+C::WorkspaceBytes
   || wide>UINT32_MAX || wide+bytes>UINT64_C(0x100000000) || ((uint32_t(wide)|bytes)&63)
   || !ReadIdle(io,r))return false;
  const uint32_t address=uint32_t(wide),route=PioneerQ38ProjectionRoute(plan.Bank());
  const uint32_t command[]={address,0,route,C::R::Scratch,0,129,0,0,bytes,0};
  uint32_t before[8],expected[8];
  for(unsigned i=0;i<8;i++){
   before[i]=terminalCounters[0][i];
   expected[i]=before[i]+(i==1?bytes/64:i==0 || i==2 || i==3 || i==6?1U:0U);
  }
  // SAME owned READ-only counter/command journal as committed readout.
  // Never recapture a baseline from live hardware or a token record.
  readoutCommandKnown=true;
  for(unsigned i=0;i<10;i++){
   readoutCommand[i]=command[i];io.ResidentNocWrite(i*4,command[i]);
   if(!Sources(io,0) || io.ResidentNocRead(i*4)!=command[i])return false;
  }
  const uint64_t began=io.Q38CohortNow();uint64_t previous=began;
  if(began>UINT64_MAX-UINT64_C(30000000) || !ReadIdle(io,r))return false;
  readoutPending=true;io.ResidentNocWrite(0x40,1);r.packets++;bool done=false;
  for(unsigned poll=0;poll<65536;poll++){
   if(!BootTime(io,began,previous) || !Sources(io,0))return false;
   const uint32_t busy=io.ResidentNocRead(0x40);if(busy>1)return false;done=!busy;
   for(unsigned i=0;i<8;i++){
    const uint32_t actual=io.ResidentNocRead(PioneerQ38PanelCounter(i));
    if(uint32_t(actual-before[i])>uint32_t(expected[i]-before[i]))return false;
    done &= actual==expected[i];
   }
   if(done)break;
   io.Pause();
  }
  if(!done || !Sources(io,0) || !PioneerQ38GraphCsrIdle(io))return false;
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[i])return false;
  for(unsigned i=0;i<8;i++)terminalCounters[0][i]=expected[i];
  readoutPending=false;r.bytesRead+=bytes;return ReadIdle(io,r);
 }
public:
 template<class IO> __attribute__((noinline)) bool ReadHiddenChunk(IO& io,unsigned part,uint32_t* output,
  unsigned words,PioneerQ38TokenHiddenReadResult& r){
  const PioneerQ38TokenHiddenReadResult pristine;
  const auto* a=reinterpret_cast<const uint8_t*>(&r);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(part>=2 || part!=hiddenReadCursor || words!=(part?1024U:4096U) || !CommittedRecordsRead()
   || restoring || l1Restored || !observedTerminal[6] || observedTerminal[6]>64
   || !HiddenReadOutput(output,size_t(words)*4) || !HiddenReadOutput(&r,sizeof(r)) || !Scope(io))return false;
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),result=reinterpret_cast<uintptr_t>(&r);
  if(out<result+sizeof(r) && result<out+size_t(words)*4)return false;
  for(unsigned i=0;i<words;i++)if(output[i])return false;
  r.started=1;r.phase=1;r.part=part;r.firstWord=part*4096;r.wordCount=words;
  r.position=observedTerminal[4];r.completedLayers=observedTerminal[6];r.terminalKind=observedTerminal[2];r.committed=observedTerminal[3];
  PioneerQ38TokenReadResult check;
  auto fail=[&](){r.worker=check.worker;r.word=check.word;r.packets=check.packets;r.bytesRead=check.bytesRead;
   r.unsafe=1;r.ready=0;return FailOwner(io);};
  for(unsigned w=0;w<120;w++){check.worker=w;if(!HoldGuard(io,w,check))return fail();}
  check.worker=0;uint32_t boot[64];
  if(!startup->ReadBoot(0,boot) || !ReadFrontier(io,boot,check))return fail();
  r.phase=2;
  if(!ReadHiddenPacket(io,part,check))return fail();
  r.packets=check.packets;r.bytesRead=check.bytesRead;
  for(unsigned i=0;i<words;i++){
   check.word=i;if(!io.TokenMemoryReadChecked(0,PioneerQ38GlobalChainLayout::R::Scratch+i*4,staging[i]))return fail();
  }
  r.phase=3;
  for(unsigned w=0;w<120;w++){check.worker=w;if(!HoldGuard(io,w,check))return fail();}
  check.worker=0;if(!ReadFrontier(io,boot,check))return fail();
  // Publish the ENTIRE part atomically only after current guards pass.
  // Raw bits,including nonfinite/corrupt values,are diagnostic DATA only.
  for(unsigned i=0;i<words;i++)output[i]=staging[i];
  hiddenReadCursor++;r.ready=1;r.phase=4;return true;
 } // does NOT change record cursor/drain,terminal/STOP/restore or Work authority
private:
