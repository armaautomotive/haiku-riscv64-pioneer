/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
__attribute__((noinline)) bool PioneerSessionIO::TokenPciScope(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 if(!s || !s->pciCloseAttempted || (s->phase!=PioneerQ38TokenNativeState::PciClosing && s->phase!=PioneerQ38TokenNativeState::PciClosed)
  || !s->viewsCloseAttempted || !s->viewsClosed || !TokenFinalLifetimeScope() || !q38TokenViews.Complete() || !q38TokenViews.Matches(*this)
  || runtimeMapped || fleetEnabled || runtimeEntry || runtimePc || scratchData
  || !PioneerMinimumBarLayoutMatches(c->profile) || profile.bar0_host_start!=c->profile.bar0_host_start
  || !s->pci.Matches(*this,c->Pci(),c->transaction))return false;
 if(s->phase==PioneerQ38TokenNativeState::PciClosed)return s->PciRestored();
 const uint64_t now=Q38CohortNow();
 if(s->pciClosed || now<s->pciPrevious || now<s->pciBegan || now-s->pciBegan>=UINT64_C(300000000))return false;
 s->pciPrevious=now;return true;
} // no inherited MMIO aliases;all hosts/model reservations still retained
__attribute__((noinline)) bool PioneerSessionIO::TokenPciReadChecked(bool root,unsigned offset,unsigned width,uint32& value){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 const bool ordinary=width==4 && !(offset&3) && (root?(offset==0 || offset==8 || offset==0x10 || offset==0x14 || offset==0x18
  || offset==0x20 || offset==0x24 || offset==0x28 || offset==0x2c || offset==0x38)
  :(offset==0 || offset==4 || (offset>=0x10 && offset<=0x24) || offset==0x2c || offset==0x30));
 const bool capabilities=width==2 && !(offset&1) && offset>=0x40 && offset<=0xfe;
 if(!s || s->phase!=PioneerQ38TokenNativeState::PciClosing
  || !(ordinary || capabilities || (offset==4 && width==2) || (offset==0x34 && width==1)) || !TokenPciScope())return false;
 value=root?RootRead(offset,width):CardRead(offset,width);return true;
}
__attribute__((noinline)) bool PioneerSessionIO::TokenPciWriteChecked(bool root,unsigned offset,unsigned width,uint32 value){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 if(!s || s->phase!=PioneerQ38TokenNativeState::PciClosing || !TokenPciScope() || !s->pci.PermitsWrite(root,offset,width,value))return false;
 if(root)RootWrite(offset,width,value);else CardWrite(offset,width,value);return true;
} // fixed exact-next original config word,NEVER BME/MMIO/NoC writes
[[maybe_unused]] static __attribute__((noinline)) bool CloseQ38TokenNativePci(PioneerStoreContext& c,PioneerQ38TokenPciResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenPciResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::ViewsClosed || !s->ViewsReleased() || s->pciCloseAttempted || s->pciClosed
  || !s->pci.Prepared() || !s->pci.Matches(c.io,c.Pci(),c.transaction) || !Q38TokenExecutionOutput(c,&result,sizeof(result))
  || !c.io.TokenViewsScope())return false;
 s->pciBegan=s->pciPrevious=c.io.Q38CohortNow();
 if(s->pciBegan>UINT64_MAX-UINT64_C(300000000))return false;
 s->pciCloseAttempted=true;s->phase=PioneerQ38TokenNativeState::PciClosing;result.started=1;
 bool okay=s->pci.Close(c.io,c.Pci(),c.transaction);result.writes=s->pci.Writes();result.reads=s->pci.Reads();
 if(okay && s->pci.Complete()){
  s->pciClosed=true;s->phase=PioneerQ38TokenNativeState::PciClosed;okay=c.io.TokenPciScope();
 }
 if(!okay){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;return false;}
 result.ready=1;return true;
} // PCI config ONLY;no host/reservation release/full native END grant
#endif
