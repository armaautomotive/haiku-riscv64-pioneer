/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchMmioEnd.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveFinalPciState.h"
// Necessary CPU phase predicate only. These metadata endpoints are NOT
// genuine closed-native/clock/source proofs and never authorize ANY IO.
struct PciStateFixture{
 struct Native{bool closed=true;bool Closed()const{return closed;}} native;
 Native* q38MixedLiveNative=&native;
 struct IO{bool q38MixedLiveMemoryMapAttempted=true,q38MixedLiveFinalUnmapAttempted=false,scope=true;
  bool MixedLiveCallScopeMatches(){return scope;}}io;
 struct Clock{bool unsafe=false,required=false;tt_arc_clock_guard_result value={};
  bool Unsafe()const{return unsafe;}bool RequiresExplicitEnd()const{return required;}const auto& Snapshot()const{return value;}}q38Clock;
 bool bankStorageOnly=true;unsigned q38EndPhase=50;
 tt_pioneer_mmio_transaction transaction={};tt_pioneer_mmio_result pci={};
 auto& Pci(){return pci;}
 PciStateFixture(){transaction.map_attempted=true;transaction.config_attempted=false;pci.state=TT_PIONEER_MMIO_RESTORE_FAILED;q38Clock.value.state=TT_ARC_CLOCK_RESTORED;q38Clock.value.restored=1;}
};
int main(){
 PciStateFixture valid;const auto before=valid.pci;
 assert(!PioneerQ38MixedLiveFinalPciUncertain(valid) && valid.pci.state==before.state && !valid.pci.restored);
 unsigned refused=0;
 for(unsigned fault=0;fault<14;fault++){
  PciStateFixture f;
  if(fault==0)f.bankStorageOnly=false;if(fault==1)f.q38EndPhase=40;if(fault==2)f.q38EndPhase=60;
  if(fault==3)f.transaction.map_attempted=false;if(fault==4)f.transaction.config_attempted=true;
  if(fault==5)f.q38MixedLiveNative=nullptr;if(fault==6)f.native.closed=false;
  if(fault==7)f.io.q38MixedLiveMemoryMapAttempted=false;if(fault==8)f.io.q38MixedLiveFinalUnmapAttempted=true;
  if(fault==9)f.io.scope=false;if(fault==10)f.q38Clock.unsafe=true;if(fault==11)f.q38Clock.required=true;
  if(fault==12)f.q38Clock.value.state=TT_ARC_CLOCK_BUSY_VERIFIED;if(fault==13)f.q38Clock.value.restored=0;
  assert(PioneerQ38MixedLiveFinalPciUncertain(f) && f.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED && !f.pci.restored);refused++;
 }
 PciStateFixture recovery;recovery.q38Clock.value.recovery_required=1;assert(PioneerQ38MixedLiveFinalPciUncertain(recovery));refused++;
 printf("PASS actual final PCI CPU predicate:expected pre-Unmap failure marker does not self-block;refusals%u/PCI status NEVER cleared or published/restored;NO native or IO authority\n",refused);
}
