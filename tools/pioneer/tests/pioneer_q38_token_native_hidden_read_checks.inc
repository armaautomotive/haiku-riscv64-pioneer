/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_hidden_read_role.h"
static void CheckNativeHiddenReadout(Fixture& f){
 namespace C=PioneerQ38GlobalChainLayout;
 auto& c=*f.c;auto& io=f.io;auto& cpu=*io.executionCpu;auto* state=c.q38TokenNative;
 assert(!lockHeld && state->TerminalHeld() && !state->Unsafe() && !state->Closed()
  && cpu.pc==0x60004 && io.terminalHoldOrder.size()==120 && io.executionCancelWrites==1);
 const unsigned packets=io.runtimePackets,reads=io.runtimeReads,writes=io.runtimeWrites,rootPackets=cpu.layerPackets;
 const unsigned instructions=cpu.steps;
 auto part=std::make_unique<std::array<uint32_t,4096>>();
 auto captured=std::make_unique<std::array<uint32_t,5120>>();part->fill(0);captured->fill(0);
 unsigned before=io.touches;PioneerQ38TokenHiddenReadResult outside;
 assert(!ReadQ38TokenNativeHiddenChunk(c,0,part->data(),4096,outside) && io.touches==before);
 unsigned early=1;
 for(unsigned index=0;index<2;index++){
  part->fill(0);
  // TWO genuine fresh FD/team/thread/global-lock calls. No carried ready
  // hint,processor snapshot,CPU expected value or replacement history.
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  before=io.touches;PioneerQ38TokenHiddenReadResult result;const auto pristine=result;
  if(!index){
   for(unsigned i=0;i<16;i++){
    reinterpret_cast<uint32_t*>(&result)[i]^=1;
    assert(!ReadQ38TokenNativeHiddenChunk(c,0,part->data(),4096,result) && io.touches==before);
    result=pristine;early++;
   }
   for(unsigned wrong:{1U,2U,0xffffffffU}){
    assert(!ReadQ38TokenNativeHiddenChunk(c,wrong,part->data(),wrong==1?1024:4096,result) && io.touches==before);early++;
   }
   for(unsigned wrong:{0U,1U,4095U,4097U,0xffffffffU}){
    assert(!ReadQ38TokenNativeHiddenChunk(c,0,part->data(),wrong,result) && io.touches==before);early++;
   }
   for(void* alias:{static_cast<void*>(&c),static_cast<void*>(&io),static_cast<void*>(state),static_cast<void*>(&result),
      static_cast<void*>(c.initialized),static_cast<void*>(c.hashes)}){
    assert(!ReadQ38TokenNativeHiddenChunk(c,0,static_cast<uint32_t*>(alias),4096,result) && io.touches==before);early++;
   }
   auto* misaligned=reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(part->data())+1);
   auto* overflow=reinterpret_cast<uint32_t*>(UINTPTR_MAX&~uintptr_t(3));
   for(auto* bad:{static_cast<uint32_t*>(nullptr),misaligned,overflow}){
    assert(!ReadQ38TokenNativeHiddenChunk(c,0,bad,4096,result) && io.touches==before);early++;
   }
   (*part)[0]=1;assert(!ReadQ38TokenNativeHiddenChunk(c,0,part->data(),4096,result) && io.touches==before);early++;
   (*part)[0]=0;
  }
  const unsigned words=index?1024:4096;
  const bool okay=ReadQ38TokenNativeHiddenChunk(c,index,part->data(),words,result);
  if(!okay)fprintf(stderr,"HIDDEN_READ_DIAGNOSTIC part%u phase%u worker%u word%u unsafe%u packets%u bytes%u;NO hidden product supplied\n",
   index,result.phase,result.worker,result.word,result.unsafe,result.packets,result.bytesRead);
  assert(okay && result.version==1 && result.started && result.ready && !result.unsafe && result.phase==4
   && result.part==index && result.firstWord==index*4096 && result.wordCount==words && !result.position
   && result.completedLayers==1 && result.terminalKind==3 && !result.committed
   && result.packets==1 && result.bytesRead==words*4 && !state->Closed() && !state->Unsafe()
   && state->TerminalHeld() && io.TokenModelSourceScope(false));
  for(unsigned i=0;i<words;i++){
   // OBSERVER ONLY: compare actual returned DATA with the SAME actual
   // shared GDDR words written by the original instruction engines.
   assert((*part)[i]==io.runtimeDram.at(C::R::FinalOffset/4+index*4096+i));
   (*captured)[index*4096+i]=(*part)[i];
  }
  for(unsigned i=words;i<4096;i++)assert(!(*part)[i]);
  part->fill(0); // duplicate refusal must prove cursor ordering,not nonzero-buffer rejection
  const unsigned touches=io.touches;PioneerQ38TokenHiddenReadResult duplicate;
  assert(!ReadQ38TokenNativeHiddenChunk(c,index,part->data(),words,duplicate) && io.touches==touches);early++;
 }
 for(unsigned i=0;i<5120;i++)assert((*captured)[i]==io.runtimeDram.at(C::R::FinalOffset/4+i));
 assert(!lockHeld && io.runtimePackets==packets+2 && io.runtimeReads==reads+2 && io.runtimeWrites==writes
  && cpu.pc==0x60004 && cpu.layerPackets==rootPackets && cpu.steps==instructions
  && state->TerminalHeld() && !state->Unsafe() && !state->Closed() && !io.pciWrites && !io.HostReadMock::bmeWrites
  && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
 printf("EXECUTED native SAME original full851/layer0/cancel/119 actual STOP/120 actual HOLD/zero-record drain/TWO actual held hidden READ packets16384+4096/fresh call scopes/ALL5120 returned words EXACT same engine-written GDDR/%u early refusals/no model write/record grant/history transplant/CPU products;original full END STILL REQUIRED;NOT physical/numerical/Work/TPS qualification\n",early);
}
