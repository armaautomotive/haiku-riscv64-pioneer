/* SPDX-License-Identifier: MIT */
// TEST clock endpoints and one actual native zero controller operation.
// Never seeds a private lease/progress/publication or success receipt.
#define PIONEER_Q38_OUTPUT_BINDING_FIXTURE_ONLY
#include "pioneer_q38_resident_output_native_binding_test.cpp"
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 NativeBindingFixture f(argv[1],argv[2]);assert(BindQ38ResidentOutputNativeState(*f.c));
 // Only the TEST time/counter endpoint is varied,not native initialization.
 for(unsigned inherited:{0U,672U}){
  f.io.rootStarts=inherited;f.io.clock=100000;f.io.outputBackwardsAfterStarts=inherited;
  assert(f.io.Q38CohortNow()==100000);f.io.rootStarts++;assert(f.io.Q38CohortNow()==0);
 }
 f.io.rootStarts=0;f.io.clock=100000;f.io.outputBackwardsAfterStarts=0;
 PioneerQ38StateZeroResult result;
 assert(!InitializeQ38ResidentOutputRecurrent(*f.c,result) && result.started && !result.completed
  && result.recovery_required && f.io.outputGdnRuns==1 && f.io.rootStarts==1 && !f.io.rootStops && !f.io.rootHolds);
 auto* state=f.c->q38ResidentOutputNative;assert(state && state->Unsafe() && f.c->dispatchOwnerRetained);
 unsigned before=f.io.touches;PioneerQ38ConvHistoryZeroResult history;
 assert(!InitializeQ38ResidentOutputRecurrent(*f.c,result) && !InitializeQ38ResidentOutputHistory(*f.c,history)
  && !f.Initialize() && !EndFactory(*f.c) && f.io.touches==before);
 puts("PASS TEST clock starts at positive baseline/drops ONLY after NEW release with0 or672 inherited starts;ACTUAL new native controller refuses backward clock AFTER one frozen-RV32 GDN job,retainsALL,NO STOP/hold/retry/END/cleanup. NOT silicon or rate evidence");
}
