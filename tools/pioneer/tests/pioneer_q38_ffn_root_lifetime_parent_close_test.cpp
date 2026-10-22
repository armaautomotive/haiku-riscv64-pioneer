/* SPDX-License-Identifier: MIT */
// Actual new lifetime and ACTUAL original parent CLOSE controllers.
// Endpoint execution is SYNTHETIC here,not native/model/timing evidence.
#define PIONEER_Q38_FFN_ROOT_LIFETIME_FIXTURE_ONLY
#define PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC 1
#define B_DEVICE_OP_CODES_END 0x1000
#include "pioneer_q38_ffn_root_lifetime_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchClose.h"
int main() {
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 RootLifetimeFixture<RootLifetimeIO> f;
 // Begin with root resident L1 clean. The NEW status writes themselves,
 // not an unrelated model upload,must obligate outer restoration.
 auto& master=f.cohort->worker[0].Store();assert(!master.l1_dirty && master.l1_restored && master.saved);
 uint32_t before[3];for(unsigned i=0;i<3;i++)before[i]=master.saved[2032+i];
 assert(f.Run() && f.result.passed && master.l1_dirty && !master.l1_restored);
 f.io.life=nullptr;f.io.track=false;
 assert(PioneerQ38DispatchClose(f.io,f.dram,f.banks,*f.cohort,true) && f.cohort->closed && !f.banks.Quarantined());
 assert(!f.cohort->worker[0].image.ready && !f.cohort->worker[0].image.dirty);
 assert(PioneerQ38DispatchPeerLeaseEnd(f.io,f.cohort->worker[0].noc,master));
 assert(master.l1_restored && master.niu_restored && master.tlb_restored && !master.ready && !master.recovery_required);
 for(unsigned w=0;w<120;w++) {
  auto& c=*f.io.core[w];auto& n=f.cohort->worker[w];
  assert(c.coreReset==0x47800 && !n.image.running && !n.image.ready && !n.image.unsafe && !n.image.dirty
   && !n.Store().ready && !n.Store().recovery_required);
  assert(c.entry==c.saved[0]);for(unsigned i=0;i<TT_Q38_PANEL_VIEW_WORDS;i++)assert(c.view[i]==c.saved[1+i]);
 }
 for(unsigned i=0;i<3;i++)assert(f.io.core[0]->RuntimeStatusRead(i)==before[i]);
 puts("PASS ACTUAL ROOT lifetime->original parent CLOSE->root resident lease END:ALL120outer panel/boot snapshots restored,initially CLEAN root residentL1 marked dirty BEFORE new status writes and restored on END,rootNoC8806reads238writes NEVER counter-restored;SIMULATED_ENDPOINTS_NOT_NATIVE_HOST_CLOCK_MMIO_END_OR_SPEED_AUTHORITY");
}
