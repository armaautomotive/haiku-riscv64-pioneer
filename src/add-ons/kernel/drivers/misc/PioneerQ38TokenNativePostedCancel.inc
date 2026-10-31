/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_POSTED_CANCEL
#include "PioneerQ38PostedCancel.h"
bool PioneerSessionIO::PostedCancelScope(const PioneerQ38PostedCancel& request){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 return &request==&q38PostedCancel && !request.Failed() && !runtimeCore
  && s && s->phase==PioneerQ38TokenNativeState::RootRunning && s->runtime
  && s->runtime->CancellationRequested() && !s->runtime->TerminalObserved()
  && TokenMemoryMappingStable(0) && TokenModelSourceScope(false)
  && TokenRuntimeScope(*s->runtime,*s->memory,s->canonical,*s->startup,s->identity,c->initialized,PioneerResidentBanks::Pages);
} // SAME FD/team/thread/generation/reservations/clock/hosts/VM/PCI owner;CPU-only
bool PioneerSessionIO::PostedCancelTlbRead(unsigned slot,unsigned word,uint32_t& value){
 if(slot>=2 || word>=3 || !PostedCancelScope(q38PostedCancel))return false;
 value=DramTlbRead(slot,word);return true;
}
bool PioneerSessionIO::PostedCancelControlWrite(bool posted){
 if(q38PostedCancel.Current()!=(posted?PioneerQ38PostedCancel::Switching:PioneerQ38PostedCancel::Restoring)
  || !PostedCancelScope(q38PostedCancel))return false;
 DramTlbWrite(0,2,PioneerQ38PostedCancel::Expected(0,2,posted));return true;
} // fixed existing TLB200 mode ONLY;no new coordinate,address,window,NoC or reset
bool PioneerSessionIO::PostedCancelWordRead(uint32_t& value){
 if(!PostedCancelScope(q38PostedCancel))return false;
 value=Q38PanelRead(PioneerQ38TokenLayout::CancelLocal);return true;
}
bool PioneerSessionIO::PostedCancelWordWrite(){
 if(q38PostedCancel.Current()!=PioneerQ38PostedCancel::Posting || !PostedCancelScope(q38PostedCancel))return false;
 Q38PanelWrite(PioneerQ38TokenLayout::CancelLocal,PioneerQ38TokenLayout::CancelMagic);return true;
} // single fixed RAW32 monotonic request;never model/code/state/terminal DATA
bool PioneerSessionIO::PostedCancelSourceGuard(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 if(!s || !s->participants || s->participants!=c->q38Dispatch)return false;
 for(unsigned pass=0;pass<2;pass++){
  if(!PostedCancelScope(q38PostedCancel) || ResetRead()!=0x46800)return false;
 }
 for(unsigned i=0;i<16;i++){
  if(!PostedCancelScope(q38PostedCancel)
   || ResidentNocRead(BlackholeDramSourceOffset(i))!=s->participants->authority.source[0][i])return false;
 }
 for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++){
  if(!PostedCancelScope(q38PostedCancel) || SourceRead(i)!=dram.source[i])return false;
 }
 bool allowed=true;
 auto read=[&](unsigned peer,uint16_t offset,uint8_t bytes)->uint32_t{
  if(peer>=2 || !PostedCancelScope(q38PostedCancel)){allowed=false;return UINT32_MAX;}
  return HostPciRead(peer,offset,bytes);
 };
 struct Pci {decltype(read)& call;uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t b){return call(p,o,b);}} pci{read};
 for(unsigned peer=0;peer<2;peer++)if(!PioneerQ38GraphCsrPciQuiet(pci,peer) || !allowed)return false;
 return PostedCancelScope(q38PostedCancel);
} // reads use the unchanged TLB201 NIU alias,not the temporary L1 ordering mode
#endif
