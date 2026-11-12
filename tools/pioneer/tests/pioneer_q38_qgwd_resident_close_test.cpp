/* SPDX-License-Identifier: MIT */
// Exercises the actual closure orchestration against fake endpoints only.
// It does not prove the endpoints' effects or device-memory preservation.
#include <cassert>
#include <cstddef>
#include <cstdio>

namespace PioneerQ38QgwdResident {
struct Reply {unsigned started=0,stage=0,unsafe=0,ready=0,hostsReleased=0,reservationRetained=0;};
}
struct FakeState {
 bool held=true,hosts=true,closed=false;
 bool TerminalHeld()const{return held;}
 bool HostBuffersReleased()const{return hosts;}
 bool Closed()const{return closed;}
};
struct FakeIO {bool hostScope=true;bool TokenHostReleaseScope()const{return hostScope;}};
struct Context {
 FakeState* q38TokenNative=nullptr;FakeIO io;bool qgwdResidentRequestClosed=false;
 unsigned fail=0,calls=0,retained=0;
};
struct PioneerQ38TokenCallScope {Context* c;Context* Context(){return c;}};
struct tt_resident_session_result {};
static void RetainQ38DispatchOwner(Context& c,tt_resident_session_result&){c.retained++;}
#define FAKE_STAGE(type,fn,index) \
 struct type {};static bool fn(Context& c,type&){assert(c.calls+1==index);c.calls++;return c.fail!=index;}
FAKE_STAGE(PioneerQ38TokenRestoreResult,RestoreQ38TokenNativeL1,1)
FAKE_STAGE(PioneerQ38TokenDispatchRestoreResult,RestoreQ38TokenNativeDispatchImages,2)
FAKE_STAGE(PioneerQ38TokenPeerRestoreResult,RestoreQ38TokenNativePeerStores,3)
FAKE_STAGE(PioneerQ38TokenRootStoreRestoreResult,RestoreQ38TokenNativeRootStore,4)
FAKE_STAGE(PioneerQ38TokenSharedTlbRestoreResult,RestoreQ38TokenNativeSharedMappings,5)
FAKE_STAGE(PioneerQ38TokenClockResult,EndQ38TokenNativeClock,6)
FAKE_STAGE(PioneerQ38TokenHardwareSealResult,SealQ38TokenNativeHardware,7)
FAKE_STAGE(PioneerQ38TokenViewsResult,CloseQ38TokenNativeViews,8)
FAKE_STAGE(PioneerQ38TokenPciResult,CloseQ38TokenNativePci,9)
FAKE_STAGE(PioneerQ38TokenHostsResult,CloseQ38TokenNativeHosts,10)
#undef FAKE_STAGE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38QgwdResidentClose.inc"

int main(){
 for(unsigned invalid=0;invalid<4;invalid++){
  FakeState s;Context c;c.q38TokenNative=&s;PioneerQ38TokenCallScope scope{&c};
  if(invalid==0)scope.c=nullptr;else if(invalid==1)c.q38TokenNative=nullptr;
  else if(invalid==2)c.qgwdResidentRequestClosed=true;else s.held=false;
  PioneerQ38QgwdResident::Reply r;
  assert(!CloseQ38QgwdResidentRequest(scope,r));
  assert(!c.calls && !c.retained && !r.started && !r.ready && !r.unsafe);
 }
 for(unsigned fail=1;fail<=10;fail++){
  FakeState s;Context c;c.q38TokenNative=&s;c.fail=fail;
  PioneerQ38TokenCallScope scope{&c};PioneerQ38QgwdResident::Reply r;
  assert(!CloseQ38QgwdResidentRequest(scope,r));
  assert(c.calls==fail && c.retained==1 && !c.qgwdResidentRequestClosed);
  assert(r.started==1 && r.stage==fail && r.unsafe==1 && !r.ready && !r.reservationRetained);
 }
 for(unsigned invalid=0;invalid<3;invalid++){
  FakeState s;Context c;c.q38TokenNative=&s;
  if(invalid==0)s.hosts=false;else if(invalid==1)s.closed=true;else c.io.hostScope=false;
  PioneerQ38TokenCallScope scope{&c};PioneerQ38QgwdResident::Reply r;
  assert(!CloseQ38QgwdResidentRequest(scope,r));
  assert(c.calls==10 && c.retained==1 && !c.qgwdResidentRequestClosed && r.unsafe && !r.ready);
 }
 FakeState s;Context c;c.q38TokenNative=&s;PioneerQ38TokenCallScope scope{&c};
 PioneerQ38QgwdResident::Reply r;
 assert(CloseQ38QgwdResidentRequest(scope,r));
 assert(c.calls==10 && !c.retained && c.qgwdResidentRequestClosed && r.ready && !r.unsafe
  && r.hostsReleased==3 && r.reservationRetained==1);
 PioneerQ38QgwdResident::Reply duplicate;
 assert(!CloseQ38QgwdResidentRequest(scope,duplicate) && c.calls==10 && !duplicate.started);
 puts("PASS resident request close: 10 failed stages quarantined, postconditions and duplicate rejected; FAKE ENDPOINTS ONLY");
}
