/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
static_assert(TT_PIONEER_MMIO_READ_OK==PioneerQ38TokenPciLease::LiveState && TT_PIONEER_MMIO_RESTORE_FAILED==PioneerQ38TokenPciLease::FailureState,"PCI result ABI");
bool PioneerSessionIO::TokenPciLeaseIdentity(const PioneerQ38TokenPciLease& pci)const{
 const auto* c=q38MutableContext;return c && &c->io==this && c->q38TokenNative && &c->q38TokenNative->pci==&pci;
}
#ifndef PIONEER_Q38_TOKEN_NATIVE_VIEW_ENDPOINT_TEST
const void* PioneerSessionIO::TokenPciTransport(){return sPCI;}
uint64 PioneerSessionIO::TokenPciEndpoint(){return (uint64(profile.root_bus)<<24)|(uint64(sCard.bus)<<16)|(uint64(sCard.device)<<8)|sCard.function;}
#endif
__attribute__((noinline)) bool PioneerSessionIO::TokenPciCaptureScope(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 return s && s->phase==PioneerQ38TokenNativeState::RuntimePreparing && s->pci.Pristine()
  && s->memory && s->memory->Installed() && s->runtime && s->InitializationsClosed(c->banks,dram)
  && PioneerMinimumBarLayoutMatches(c->profile) && profile.bar0_host_start==c->profile.bar0_host_start
  && TokenPciTransport() && TokenModelSourceScope(false);
} // CPU-only capture of actual BEGIN record,BEFORE worker bootstrap/ROOT launch
#endif
