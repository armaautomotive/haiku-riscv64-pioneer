/* SPDX-License-Identifier: MIT */
// Mocks for the ACTUAL extracted NextQgwdResidentRequest body. No Haiku,
// device, model bytes, or hardware-safety claim. The test runner inserts the
// current body at HANDOFF_BODY and compiles this translation unit in memory.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38QgwdResidentClock.h"

static unsigned selectedFault=0,events=0,retainedCount=0,deletedCount=0;
static bool hit=false,fakeWritableDram=false;
static bool Step(unsigned n){assert(!hit && n==++events);if(n==selectedFault){hit=true;return false;}return true;}
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION
static constexpr unsigned expectedPages=5067109;
#else
static constexpr unsigned expectedPages=5149957;
#endif
enum { B_OK=0 };
namespace PioneerQ38QgwdResident {struct Reply {unsigned version=1,operation=0;int status=0;
 unsigned started=0,ready=0,unsafe=0,stage=0,reservationRetained=0,hostsReleased=0,oldContextRetired=0,modelEnded=0,reserved[5]={};};}
struct PioneerQ38SpanLease {bool closed=false;unsigned nonce=0;};
struct tt_resident_session_result {};
struct FakeProof {};
struct PioneerResidentBanks {
 enum {Pages=5149957};static constexpr uint64_t Total=uint64_t(Pages)*4096;
 bool Bind(FakeProof&){return Step(5);}
};
struct FakeState {
 struct Plan {bool Ready()const{return true;}} canonical;
 bool HostBuffersReleased()const{return true;}bool Closed()const{return false;}
 template<class C,class F>bool MatchesBinding(C&,F*,uint32_t*,uint8_t*,int,uint64_t)const{return true;}
};
struct PioneerStoreContext;struct PioneerProbeCookie;
struct FakeReceipt {
 template<class... Args>bool RebindResident(Args...){return Step(11);}
};
struct PioneerDramSamplesIO {bool writeSamples=fakeWritableDram;void* data=nullptr;int dataArea=-1;};
struct PioneerHostDmaAuditIO:PioneerDramSamplesIO {
 bool AfterNodeId(unsigned){
  if(!Step(3))return false;
  if(selectedFault==15)dataArea=1; // fake audit incorrectly creates a writable sample mapping
  return true;
 }
};
struct FakeIO:PioneerHostDmaAuditIO {
 PioneerStoreContext* q38MutableContext=nullptr;
 bool TokenHostReleaseScope()const{return true;}
 bool TokenModelSourceScope(bool b){assert(b);return Step(12);}
};
struct FakeProfile {int status=0;};
struct FakePci {unsigned node_id[1]={2};};
struct Nest4 {FakeProof dram;};
struct Nest3 {Nest4 baseline;int noc=0;};
struct Nest2 {Nest3 baseline;};
struct Nest1 {Nest2 baseline;};
struct PioneerStoreContext {
 uint8_t* initialized;uint32_t* hashes;FakeState* q38TokenNative=nullptr;
 PioneerQ38SpanLease* q38Span=nullptr;FakeReceipt* q38QgwdReceipt=nullptr;FakeReceipt* q38SmallRecurrentReceipt=nullptr;
 unsigned uploadedPages=0;bool bankStorageOnly=false,qgwdResidentRequestClosed=false;
 uint64_t qgwdResidentSerial=0;int q38GdnCallThread=-1;void* q38Dispatch=nullptr;
 FakeProfile profile;FakePci pci;FakeIO io;PioneerResidentBanks banks;Nest1 baseline;
 int transaction=0,idle=0,host=0;bool quarantined=false;
 explicit PioneerStoreContext(uint64_t):initialized(new uint8_t[PioneerResidentBanks::Pages]()),
  hashes(new uint32_t[PioneerResidentBanks::Pages]()){io.q38MutableContext=this;}
 ~PioneerStoreContext(){deletedCount++;delete[] initialized;delete[] hashes;delete q38TokenNative;delete q38Span;delete q38QgwdReceipt;delete q38SmallRecurrentReceipt;}
 FakePci& Pci(){return pci;}bool Unsafe()const{return false;}
};
struct PioneerProbeCookie {unsigned magic=0x50313030;PioneerStoreContext* context=nullptr;
 PioneerStoreContext* qgwdResidentPending=nullptr;int team=3;};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sBankStorageOnly=true,sColdRecoveryRequired=false,sReloadQuiesced=false;
static uint64_t sReloadGeneration=4;static void* sDeviceManager=reinterpret_cast<void*>(1);static int sCard=0;
static int find_thread(void*){return 7;}
static bool PioneerMinimumBarLayoutMatches(FakeProfile&){return true;}
static bool Q38TokenNativeLearnedPresent(PioneerStoreContext&,FakeState::Plan&){return true;}
static int GetPioneerPciProfile(void*,int,FakeProfile&){return Step(1)?B_OK:-1;}
static bool BlackholePioneerMmioBegin(FakeIO&,FakePci&,int&){return Step(2);}
static bool PioneerResidentSessionIdle(FakeIO&,int&,int&){return Step(4);}
static bool BeginQ38NormalClock(PioneerStoreContext& c){assert(!c.q38Span);return Step(6);}
static bool PioneerPrepareHostPage(FakeIO&,int&){
 if(!Step(7))return false;
 if(selectedFault==13){sReloadGeneration++;hit=true;} // ownership identity changes during a successful operation
 return true;
}
static bool PioneerQ38SpanPrepareService(PioneerStoreContext& c){assert(c.q38Span && !c.q38Span->closed);return Step(8);}
static bool PioneerQ38SpanEndService(PioneerStoreContext& c){if(!Step(9))return false;c.q38Span->closed=true;return true;}
struct PioneerQ38QgwdNativeBinding {static bool Matches(PioneerStoreContext&,PioneerProbeCookie*,uint64_t){return Step(10);}};
struct PioneerQ38SmallRecurrentNativeBinding {
 static bool TransferInspection(PioneerStoreContext& old,PioneerStoreContext& next,PioneerProbeCookie* cookie,uint64_t){
  if(!Step(10) || !cookie || cookie->context!=&old || !old.q38Span || !old.q38SmallRecurrentReceipt
   || next.q38Span || next.q38SmallRecurrentReceipt || !Step(11))return false;
  next.q38Span=old.q38Span;old.q38Span=nullptr;
  next.q38SmallRecurrentReceipt=old.q38SmallRecurrentReceipt;old.q38SmallRecurrentReceipt=nullptr;return true;
 }
};
static void RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result&){c.quarantined=true;retainedCount++;}
class PioneerQ38TokenCallScope {
public:
 PioneerProbeCookie* cookie;PioneerStoreContext* context;int thread=7;bool destroyed=false;
 PioneerStoreContext* Context(){return context;}
 bool NextQgwdResidentRequest(PioneerQ38QgwdResident::Reply&);
};

// HANDOFF_BODY

int main(){
 for(unsigned fault=0;fault<=15;fault++){
  selectedFault=fault;events=retainedCount=deletedCount=0;hit=false;fakeWritableDram=false;sReloadGeneration=4;
  auto* old=new PioneerStoreContext(PioneerResidentBanks::Total);
  std::memset(old->initialized,1,expectedPages);
  old->uploadedPages=expectedPages;old->bankStorageOnly=true;old->qgwdResidentRequestClosed=true;
  old->q38GdnCallThread=7;old->q38TokenNative=new FakeState;old->q38Span=new PioneerQ38SpanLease;
  old->q38Span->closed=true;
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION
  old->q38SmallRecurrentReceipt=new FakeReceipt;
#else
  old->q38QgwdReceipt=new FakeReceipt;
#endif
  PioneerProbeCookie cookie;cookie.context=old;sRuntimeOwner=&cookie;
  PioneerQ38TokenCallScope scope{&cookie,old};PioneerQ38QgwdResident::Reply result;
  if(fault==14)fakeWritableDram=true; // only the fresh mock context has writable DRAM samples
  const bool okay=scope.NextQgwdResidentRequest(result);
  if(!fault){
   assert(okay && events==12 && !hit && !retainedCount && deletedCount==1 && result.ready && !result.unsafe);
   assert(result.stage==11 && result.oldContextRetired && result.reservationRetained && result.hostsReleased==3);
   assert(cookie.context==scope.context && cookie.context!=old && !cookie.qgwdResidentPending);
   assert(cookie.context->q38Span && cookie.context->q38Span->closed);
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION
   assert(cookie.context->q38SmallRecurrentReceipt && !cookie.context->q38QgwdReceipt);
#else
   assert(cookie.context->q38QgwdReceipt && !cookie.context->q38SmallRecurrentReceipt);
#endif
   delete cookie.context;
  }else{
   assert(!okay && (hit || fault==14 || fault==15) && events==(fault==15?3:fault==14?1:fault==13?7:fault)
    && retainedCount==2 && !deletedCount && result.unsafe && !result.ready);
   auto* first=cookie.context;auto* second=cookie.qgwdResidentPending;
   assert(first && second && first!=second && first->quarantined && second->quarantined);
   assert(first->q38Span || second->q38Span);
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION
   assert(bool(first->q38SmallRecurrentReceipt)!=bool(second->q38SmallRecurrentReceipt));
   auto* receiptOwner=first->q38SmallRecurrentReceipt?first:second;
   assert(receiptOwner->q38Span && receiptOwner->q38Span->closed);
#endif
   delete first;delete second; // mock cleanup ONLY after all assertions; real code retains both
  }
 }
 for(unsigned invalid=0;invalid<6;invalid++){
  selectedFault=events=retainedCount=deletedCount=0;hit=false;fakeWritableDram=false;
  auto* old=new PioneerStoreContext(PioneerResidentBanks::Total);
  std::memset(old->initialized,1,expectedPages);
  old->uploadedPages=expectedPages;old->bankStorageOnly=true;old->qgwdResidentRequestClosed=true;
  old->q38GdnCallThread=7;old->q38TokenNative=new FakeState;old->q38Span=new PioneerQ38SpanLease;
  old->q38Span->closed=true;
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION
  old->q38SmallRecurrentReceipt=new FakeReceipt;
#else
  old->q38QgwdReceipt=new FakeReceipt;
#endif
  PioneerProbeCookie cookie;cookie.context=old;sRuntimeOwner=&cookie;
  PioneerQ38TokenCallScope scope{&cookie,old};PioneerQ38QgwdResident::Reply result;
  if(invalid==0)result.ready=1;
  else if(invalid==1)old->qgwdResidentRequestClosed=false;
  else if(invalid==2)cookie.qgwdResidentPending=old;
  else if(invalid==3)old->qgwdResidentSerial=UINT64_MAX;
  else if(invalid==4)old->uploadedPages--;
  else{old->initialized[0]=0;old->hashes[0]=1;}
  assert(!scope.NextQgwdResidentRequest(result) && !events && !retainedCount && !deletedCount);
  assert(cookie.context==old && !old->quarantined);
  delete old;
 }
 puts("PASS actual resident handoff body with endpoint faults, generation loss, writable-DRAM and invalid-entry refusals; NO DEVICE OR MODEL");
}
