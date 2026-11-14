/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38QgwdResidentRole.h"
#include "PioneerQ38QgwdResidentProtocol.h"
#include "PioneerQ38QgwdResidentClock.h"

#include "PioneerQ38QgwdResidentClose.inc"

// The small-recurrent branch is an extracted-body mock only. It must not be
// combined with a kernel, a loadable driver or the existing native profile.
#if defined(PIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION) && \
 (PIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION != 1 || defined(_KERNEL_MODE) \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_QGWD_RESIDENT_NATIVE_146) \
 || !defined(PIONEER_Q38_SMALL_RECURRENT_RESIDENT_REBIND_INSPECTION) \
 || PIONEER_Q38_SMALL_RECURRENT_RESIDENT_REBIND_INSPECTION != 1 \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE))
#error Small recurrent resident handoff is extracted-body inspection only
#endif

__attribute__((noinline)) bool
PioneerQ38TokenCallScope::NextQgwdResidentRequest(PioneerQ38QgwdResident::Reply& out)
{
 const PioneerQ38QgwdResident::Reply pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&out);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(out);i++)if(a[i]!=b[i])return false;
 auto* old=Context();auto* state=old?old->q38TokenNative:NULL;
 if(!old || !state || destroyed || !cookie || cookie->context!=old || cookie->qgwdResidentPending
  || sRuntimeOwner!=cookie || old->q38GdnCallThread!=thread || find_thread(NULL)!=thread
  || !old->qgwdResidentRequestClosed || old->qgwdResidentSerial==UINT64_MAX
  || !state->HostBuffersReleased() || state->Closed() || !old->io.TokenHostReleaseScope()
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION
  || old->uploadedPages!=5067109 || !state->canonical.Ready()
#else
  || old->uploadedPages!=5149957 || !state->canonical.Ready()
#endif
  || !Q38TokenNativeLearnedPresent(*old,state->canonical)
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
 auto* freshSpan=new(std::nothrow) PioneerQ38SpanLease;
 if(!freshSpan){delete next;return false;}
 memcpy(next->initialized,old->initialized,PioneerResidentBanks::Pages);
 memcpy(next->hashes,old->hashes,size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t));
 next->uploadedPages=old->uploadedPages;next->bankStorageOnly=true;
 next->qgwdResidentSerial=old->qgwdResidentSerial+1;
 cookie->qgwdResidentPending=next; // publish BOTH owners BEFORE first fresh PCI/VM/clock/host access
 out.started=1;out.stage=1;
 auto current=[&](){
  return cookie && cookie->magic==0x50313030 && cookie->context==old && cookie->qgwdResidentPending==next
   && sRuntimeOwner==cookie && sBankStorageOnly && !sColdRecoveryRequired && !sReloadQuiesced
   && generation && generation==sReloadGeneration && old->q38GdnCallThread==thread && find_thread(NULL)==thread
   && state==old->q38TokenNative
   && state->MatchesBinding(*old,cookie,old->hashes,old->initialized,cookie->team,generation)
   && old->io.TokenHostReleaseScope(); // CPU-only historical closure/real FD ownership;NO old mapping access
 };
 auto fail=[&](){
  // Clock BEGIN requires no published span. Preserve the preallocated span
  // in the quarantined context on every failure before it is attached.
  if(freshSpan){next->q38Span=freshSpan;freshSpan=NULL;}
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*old,retained);RetainQ38DispatchOwner(*next,retained);
  out.unsafe=1;out.ready=0;return false; // NO subsequent restore,release,unreserve,delete or retry
 };
 if(!current())return fail();
 next->profile.status=GetPioneerPciProfile(sDeviceManager,sCard,next->profile);
 if(!current() || next->profile.status!=B_OK || !PioneerMinimumBarLayoutMatches(next->profile))return fail();
 out.stage=2;
 // This audit walks the DRAM probe hierarchy. A writable sample mapping is
 // never permissible while a model's payload is retained on the card.
 if(next->io.PioneerDramSamplesIO::writeSamples || next->io.PioneerDramSamplesIO::data
  || next->io.PioneerDramSamplesIO::dataArea>=B_OK)return fail();
 if(!current() || !BlackholePioneerMmioBegin(next->io,next->Pci(),next->transaction) || !current())return fail();
 out.stage=3;
 if(!next->io.PioneerHostDmaAuditIO::AfterNodeId(next->Pci().node_id[0]) || !current()
  || next->io.PioneerDramSamplesIO::writeSamples || next->io.PioneerDramSamplesIO::data
  || next->io.PioneerDramSamplesIO::dataArea>=B_OK)return fail();
 if(!PioneerResidentSessionIdle(next->io,next->baseline.baseline.baseline.noc,next->idle) || !current())return fail();
 if(!next->banks.Bind(next->baseline.baseline.baseline.baseline.dram) || !current())return fail();
 out.stage=4;
 if(!PioneerQ38QgwdResidentClockBeforeSpan(*next,freshSpan,
   [](PioneerStoreContext& c){return BeginQ38NormalClock(c);},current))return fail();
 out.stage=5;
 if(!PioneerPrepareHostPage(next->io,next->host) || !current())return fail();
 out.stage=6;
 // A genuinely NEW span preparation/close creates new host/NIU journals.
 // No payload is uploaded,no old span/clock/VM/host lease is reset or reused.
 if(!PioneerQ38SpanPrepareService(*next) || !current())return fail();
 if(!PioneerQ38SpanEndService(*next) || !current() || next->q38Span->nonce || !next->q38Span->closed
  || next->Unsafe() || next->q38TokenNative || next->q38Dispatch || next->qgwdResidentRequestClosed
  || next->uploadedPages!=learned || memcmp(next->initialized,old->initialized,PioneerResidentBanks::Pages)
  || memcmp(next->hashes,old->hashes,size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t)))return fail();
 // Move the sealed QGWD receipt and its original closed span, not a copied
 // ledger alone. The temporary next-context span has been actually closed.
 // Rebinding validates both full ledgers, route identity and the SAME
 // FD/team/generation; physical reservation continuity is held by this scope.
 auto* temporarySpan=next->q38Span;
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION
 if(!temporarySpan)return fail();
 next->q38Span=NULL; // keep closed temporary bookkeeping until transfer commits
 if(!PioneerQ38SmallRecurrentNativeBinding::TransferInspection(*old,*next,cookie,generation)){
  next->q38Span=temporarySpan;return fail();
 }
#else
 auto* retained=old->q38QgwdReceipt;auto* oldSpan=old->q38Span;
 if(!retained || !oldSpan || !temporarySpan || next->q38QgwdReceipt
  || !PioneerQ38QgwdNativeBinding::Matches(*old,cookie,generation)
  || !retained->RebindResident(old,cookie,old->hashes,old->initialized,cookie->team,generation,oldSpan,
   next,next->hashes,next->initialized,next->banks,next->baseline.baseline.baseline.baseline.dram,PioneerResidentBanks::Pages))return fail();
 next->q38Span=oldSpan;old->q38Span=NULL;next->q38QgwdReceipt=retained;old->q38QgwdReceipt=NULL;
#endif
 delete temporarySpan; // closed host-only lease bookkeeping; never new IO
 // One atomic private ownership transfer under the actual FD call lock.
 // The OS reservation has NEVER been unreserved or re-acquired. Old mutable
 // claims die with the OLD object;new state/KV/history owners are still VIRGIN.
 old->q38GdnCallThread=-1;next->q38GdnCallThread=thread;
 cookie->context=next;cookie->qgwdResidentPending=old;context=next;
 if(!next->io.TokenModelSourceScope(true) || sColdRecoveryRequired || sReloadQuiesced || generation!=sReloadGeneration){
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*old,retained);RetainQ38DispatchOwner(*next,retained);
  out.unsafe=1;return false; // BOTH current+pending remain reachable from this FD
 }
 cookie->qgwdResidentPending=NULL;old->io.q38MutableContext=NULL;
 delete old; // CPU journals ONLY;all old VM/hosts/PCI/clock already actually closed
 out.stage=11;out.oldContextRetired=1;out.reservationRetained=1;out.hostsReleased=3;out.ready=1;
 return true; // prepared NEW context,NOT initialized recurrent/KV/metadata,ROOT start or model END
}

static __attribute__((noinline)) status_t Q38QgwdResidentSupport(void* data,size_t length)
{
 namespace R=PioneerQ38QgwdResident;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!data || length!=sizeof(R::Support))return B_BAD_VALUE;
 R::Support result;status_t status=user_memcpy(&result,data,sizeof(result));
 if(status!=B_OK || !R::SupportRequest(result))return status!=B_OK?status:B_BAD_VALUE;
 result.driver=55;result.version=R::Version;result.requestClosure=result.modelReservation=result.freshRequest=result.explicitModelEnd=1;
 return user_memcpy(data,&result,sizeof(result)); // metadata ONLY
}

static __attribute__((noinline)) status_t
Q38QgwdResidentControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
 namespace R=PioneerQ38QgwdResident;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!cookie || cookie->magic!=0x50313030 || !data || length!=sizeof(R::Packet))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 R::Packet packet;status=user_memcpy(&packet,data,sizeof(packet));
 if(status!=B_OK || !R::RequestShape(packet))return status!=B_OK?status:B_BAD_VALUE;
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 if(!c)return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
 auto& out=packet.reply;bool okay=false;
 if(packet.request.operation==R::RequestClose)okay=CloseQ38QgwdResidentRequest(scope,out);
 else if(packet.request.operation==R::NextRequest)okay=scope.NextQgwdResidentRequest(out); // c MAY now dangle
 else if(c->qgwdResidentRequestClosed && !cookie->qgwdResidentPending){
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
   if(cookie->qgwdResidentPending)RetainQ38DispatchOwner(*cookie->qgwdResidentPending,retained);
  }
  return copied;
 }
 return out.status;
}
