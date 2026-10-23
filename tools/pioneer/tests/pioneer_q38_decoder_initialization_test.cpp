/* SPDX-License-Identifier: MIT */
// Complete zero coverage is not readiness if the final physical restore
// fails. Also execute both real zero controllers sequentially against ONE
// actual joint reservation:neither role may initialize the other's ledger.
#define PIONEER_Q38_CONV_HISTORY_ZERO_FIXTURE_ONLY
#include "pioneer_q38_conv_history_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionZeroController.h"
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 {ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero() && f.Zero(335,0,2) && f.CloseZero());
  auto* projection=f.graph->Projection(f.id);assert(projection);PioneerQ38ProjectionExtent e;
  assert(projection->InitializeSpan(f.id,0,0,512,e));f.io.projectionMode=true;f.io.projectionBase=e.address;f.io.projectionBank=e.bank;
  PioneerQ38ProjectionZeroLease lease;PioneerQ38ProjectionZeroResult result;
  assert(PioneerQ38ProjectionZeroPrepare(f.io,f.owner.dram,f.owner.noc,*projection,f.owner.banks,f.id,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,lease));
  for(unsigned first:{0U,256U}){
   assert(PioneerQ38ProjectionZeroRun(f.io,f.owner.dram,f.owner.noc,*projection,f.owner.banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,
    lease,f.id,0,first,256,result) && result.pages==256 && result.completed && result.held);
   for(auto word:f.io.lastDeviceWords)assert(!word);
   unsigned history=0;assert(f.state->InitializationProgress(f.id,history) && history==2 && !f.state->InitializationPublished(f.id));
  }
  assert(!projection->InitializationPublished(f.id));
  assert(PioneerQ38ProjectionZeroClose(f.io,f.owner.dram,f.owner.noc,*projection,f.owner.banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,lease,true)
   && projection->InitializationPublished(f.id));
  f.io.projectionMode=false;PioneerQ38ConvHistoryZeroLease next;f.UseLease(next);
  assert(f.PrepareZero() && f.Zero(335,2,3));unsigned progress=0;
  assert(f.state->InitializationProgress(f.id,progress) && progress==5 && !f.state->InitializationPublished(f.id));
  assert(projection->InitializationProgress(f.id,progress) && progress==512 && projection->InitializationPublished(f.id));
  assert(f.graph->Recurrent(f.id)->InitializationProgress(f.id,progress) && !progress && !f.graph->Recurrent(f.id)->InitializationPublished(f.id));
  f.Finish();printf("PASS actual joint projection512+history5page sequential initializers/independent ledgers/clean partial resume/learned unchanged\n");
 }
 {ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero());
  for(unsigned region=0;region<336;region++)assert(f.Zero(region));unsigned progress=0;
  assert(f.state->InitializationProgress(f.id,progress) && progress==1680 && !f.state->InitializationPublished(f.id));
  for(auto word:f.io.wholeHistory)assert(!word);for(auto count:f.io.touchedPages)assert(count==1);
  // All actual device writes/readbacks succeeded. A dropped final boot
  // restore MUST quarantine instead of publishing an initialization seal.
  f.io.dropEntry=f.io.entryWrites+1;assert(!f.CloseZero());f.Unknown();
  printf("PASS actual ALL1680page history coverage plus failed physical CLOSE;NO initialization seal/retainALL/ZERO later IO\n");
 }
 printf("PASS decoder initialization ACTUAL frozen RV32/one joint reservation/separate state ledgers/full coverage cannot bypass physical restore;NO_NATIVE_FACTORY_COMPUTE_INPUT_OR_SPEED\n");
}
