/* SPDX-License-Identifier: MIT */
#include "PioneerQ38FullOffloadResidentRole.h"
#include "PioneerQ38FullOffloadResidentProtocol.h"

// Request closure deliberately stops BEFORE device unreserve/context END.
// Every actual launched journal,clock,VM alias,PCI change and host buffer is
// still restored/released. The OS reservation and immutable weight ledger
// remain in the SAME FD-owned context;this is NOT the old full END receipt.
static __attribute__((noinline)) bool
CloseQ38FullResidentRequest(PioneerQ38TokenCallScope& scope,PioneerQ38FullResident::Reply& out)
{
 auto* c=scope.Context();auto* s=c?c->q38TokenNative:NULL;
 if(!c || !s || c->fullResidentRequestClosed || !s->TerminalHeld())return false;
 out.started=1;
#define PIONEER_FULL_RESIDENT_CLOSE(number,Type,operation) \
 {Type result;out.stage=number;if(!operation(*c,result)){out.unsafe=c->Unsafe(false,false,true);return false;}}
 PIONEER_FULL_RESIDENT_CLOSE(1,PioneerQ38TokenRestoreResult,RestoreQ38TokenNativeL1)
 PIONEER_FULL_RESIDENT_CLOSE(2,PioneerQ38TokenDispatchRestoreResult,RestoreQ38TokenNativeDispatchImages)
 PIONEER_FULL_RESIDENT_CLOSE(3,PioneerQ38TokenPeerRestoreResult,RestoreQ38TokenNativePeerStores)
 PIONEER_FULL_RESIDENT_CLOSE(4,PioneerQ38TokenRootStoreRestoreResult,RestoreQ38TokenNativeRootStore)
 PIONEER_FULL_RESIDENT_CLOSE(5,PioneerQ38TokenSharedTlbRestoreResult,RestoreQ38TokenNativeSharedMappings)
 PIONEER_FULL_RESIDENT_CLOSE(6,PioneerQ38TokenClockResult,EndQ38TokenNativeClock)
 PIONEER_FULL_RESIDENT_CLOSE(7,PioneerQ38TokenHardwareSealResult,SealQ38TokenNativeHardware)
 PIONEER_FULL_RESIDENT_CLOSE(8,PioneerQ38TokenViewsResult,CloseQ38TokenNativeViews)
 PIONEER_FULL_RESIDENT_CLOSE(9,PioneerQ38TokenPciResult,CloseQ38TokenNativePci)
 PIONEER_FULL_RESIDENT_CLOSE(10,PioneerQ38TokenHostsResult,CloseQ38TokenNativeHosts)
#undef PIONEER_FULL_RESIDENT_CLOSE
 if(!s->HostBuffersReleased() || s->Closed() || !c->io.TokenHostReleaseScope()){
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);out.unsafe=1;return false;
 }
 c->fullResidentRequestClosed=true;
 out.hostsReleased=3;out.reservationRetained=1;out.ready=1;return true;
}

__attribute__((noinline)) bool
PioneerQ38TokenCallScope::NextResidentRequest(PioneerQ38FullResident::Reply& out)
{
 const PioneerQ38FullResident::Reply pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&out);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(out);i++)if(a[i]!=b[i])return false;
 auto* old=Context();auto* state=old?old->q38TokenNative:NULL;
 if(!old || !state || destroyed || !cookie || cookie->context!=old || cookie->residentPending
  || sRuntimeOwner!=cookie || old->q38GdnCallThread!=thread || find_thread(NULL)!=thread
  || !old->fullResidentRequestClosed || old->fullResidentRequestSerial==UINT64_MAX
  || !state->HostBuffersReleased() || state->Closed() || !old->io.TokenHostReleaseScope()
  || old->uploadedPages!=4736090 || !state->canonical.Ready()
  || !PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(state->canonical,old->initialized,PioneerResidentBanks::Pages)
  || !sDeviceManager || !PioneerMinimumBarLayoutMatches(old->profile))return false;
 const uint64_t generation=sReloadGeneration;
 unsigned learned=0;
 for(unsigned p=0;p<PioneerResidentBanks::Pages;p++){
  if(old->initialized[p]>1 || (!old->initialized[p] && old->hashes[p]))return false;
  learned+=old->initialized[p];
 }
 if(learned!=old->uploadedPages)return false;
 // All allocations precede any fresh physical access. An allocation refusal
 // leaves the OLD fully restored model owner unchanged and still unloadable.
 auto* next=new(std::nothrow) PioneerStoreContext(PioneerResidentBanks::Total);
 if(!next)return false;
 if(!next->initialized || !next->hashes){delete next;return false;}
 next->q38Span=new(std::nothrow) PioneerQ38SpanLease;
 if(!next->q38Span){delete next;return false;}
 memcpy(next->initialized,old->initialized,PioneerResidentBanks::Pages);
 memcpy(next->hashes,old->hashes,size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t));
 next->uploadedPages=old->uploadedPages;next->bankStorageOnly=true;
 next->fullResidentRequestSerial=old->fullResidentRequestSerial+1;
 cookie->residentPending=next; // publish BOTH owners BEFORE first fresh PCI/VM/clock/host access
 out.started=1;out.stage=1;
 auto current=[&](){
  return cookie && cookie->magic==0x50313030 && cookie->context==old && cookie->residentPending==next
   && sRuntimeOwner==cookie && sBankStorageOnly && !sColdRecoveryRequired && !sReloadQuiesced
   && generation && generation==sReloadGeneration && old->q38GdnCallThread==thread && find_thread(NULL)==thread
   && state==old->q38TokenNative
   && state->MatchesBinding(*old,cookie,old->hashes,old->initialized,cookie->team,generation)
   && old->io.TokenHostReleaseScope(); // CPU-only historical closure/real FD ownership;NO old mapping access
 };
 auto fail=[&](){
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*old,retained);RetainQ38DispatchOwner(*next,retained);
  out.unsafe=1;out.ready=0;return false; // NO subsequent restore,release,unreserve,delete or retry
 };
 if(!current())return fail();
 next->profile.status=GetPioneerPciProfile(sDeviceManager,sCard,next->profile);
 if(!current() || next->profile.status!=B_OK || !PioneerMinimumBarLayoutMatches(next->profile))return fail();
 out.stage=2;
 if(!current() || !BlackholePioneerMmioBegin(next->io,next->Pci(),next->transaction) || !current())return fail();
 out.stage=3;
 if(!next->io.PioneerHostDmaAuditIO::AfterNodeId(next->Pci().node_id[0]) || !current())return fail();
 if(!PioneerResidentSessionIdle(next->io,next->baseline.baseline.baseline.noc,next->idle) || !current())return fail();
 if(!next->banks.Bind(next->baseline.baseline.baseline.baseline.dram) || !current())return fail();
 out.stage=4;
 if(!BeginQ38NormalClock(*next) || !current())return fail();
 out.stage=5;
 if(!PioneerPrepareHostPage(next->io,next->host) || !current())return fail();
 out.stage=6;
 // A genuinely NEW span preparation/close creates new host/NIU journals.
 // No payload is uploaded,no old span/clock/VM/host lease is reset or reused.
 if(!PioneerQ38SpanPrepareService(*next) || !current())return fail();
 if(!PioneerQ38SpanEndService(*next) || !current() || next->q38Span->nonce || !next->q38Span->closed
  || next->Unsafe() || next->q38TokenNative || next->q38Dispatch || next->fullResidentRequestClosed
  || next->uploadedPages!=learned || memcmp(next->initialized,old->initialized,PioneerResidentBanks::Pages)
  || memcmp(next->hashes,old->hashes,size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t)))return fail();
 // One atomic private ownership transfer under the actual FD call lock.
 // The OS reservation has NEVER been unreserved or re-acquired. Old mutable
 // claims die with the OLD object;new state/KV/history owners are still VIRGIN.
 old->q38GdnCallThread=-1;next->q38GdnCallThread=thread;
 cookie->context=next;cookie->residentPending=old;context=next;
 if(!next->io.TokenModelSourceScope(true) || sColdRecoveryRequired || sReloadQuiesced || generation!=sReloadGeneration){
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*old,retained);RetainQ38DispatchOwner(*next,retained);
  out.unsafe=1;return false; // BOTH current+pending remain reachable from this FD
 }
 cookie->residentPending=NULL;old->io.q38MutableContext=NULL;
 delete old; // CPU journals ONLY;all old VM/hosts/PCI/clock already actually closed
 out.stage=11;out.oldContextRetired=1;out.reservationRetained=1;out.hostsReleased=3;out.ready=1;
 return true; // prepared NEW context,NOT initialized recurrent/KV/metadata,ROOT start or model END
}

static __attribute__((noinline)) status_t Q38FullResidentSupport(void* data,size_t length)
{
 namespace R=PioneerQ38FullResident;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!data || length!=sizeof(R::Support))return B_BAD_VALUE;
 R::Support result;status_t status=user_memcpy(&result,data,sizeof(result));
 if(status!=B_OK || !R::SupportRequest(result))return status!=B_OK?status:B_BAD_VALUE;
 result.driver=55;result.version=R::Version;result.requestClosure=result.modelReservation=result.freshRequest=result.explicitModelEnd=1;
 return user_memcpy(data,&result,sizeof(result)); // metadata ONLY
}

static __attribute__((noinline)) status_t
Q38FullResidentControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
 namespace R=PioneerQ38FullResident;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!cookie || cookie->magic!=0x50313030 || !data || length!=sizeof(R::Packet))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 R::Packet packet;status=user_memcpy(&packet,data,sizeof(packet));
 if(status!=B_OK || !R::RequestShape(packet))return status!=B_OK?status:B_BAD_VALUE;
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 if(!c)return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
 auto& out=packet.reply;bool okay=false;
 if(packet.request.operation==R::RequestClose)okay=CloseQ38FullResidentRequest(scope,out);
 else if(packet.request.operation==R::NextRequest)okay=scope.NextResidentRequest(out); // c MAY now dangle
 else if(c->fullResidentRequestClosed && !cookie->residentPending){
  PioneerQ38TokenEndResult result;const bool ended=scope.Finish(result); // c MAY now dangle
  out.started=result.started;out.unsafe=result.unsafe;out.hostsReleased=result.hosts;out.stage=12;
  okay=ended && result.ready && result.context && result.reservation && result.hosts==3;
  out.ready=okay;out.modelEnded=okay;
 }
 out.operation=packet.request.operation;
 out.status=out.unsafe?B_DEV_NOT_READY:okay && out.ready?B_OK:B_NOT_ALLOWED;
 const status_t copied=user_memcpy(data,&packet,sizeof(packet));
 if(copied!=B_OK){
  if(auto* actual=scope.Context()){
   tt_resident_session_result retained={};RetainQ38DispatchOwner(*actual,retained);
   if(cookie->residentPending)RetainQ38DispatchOwner(*cookie->residentPending,retained);
  }
  return copied;
 }
 return out.status;
}
