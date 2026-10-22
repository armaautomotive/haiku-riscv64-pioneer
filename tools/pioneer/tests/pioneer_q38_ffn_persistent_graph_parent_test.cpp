/* SPDX-License-Identifier: MIT */
// ACTUAL new owner+119 boot/ROOT STOP+original outer cleanup. This separate
// no-FFN case tests closing an unused READY session,NOT numeric/speed work.
#define PIONEER_Q38_PERSISTENT_GRAPH_FIXTURE_ONLY
#include "pioneer_q38_ffn_persistent_graph_test.cpp"
int main(int argc,char** argv){
 assert(argc==4);GraphImage=argv[1];PeerImage=argv[2];setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&FreshInputs;
 PersistentFixture f(argv[3]);assert(f.Prepare() && f.Close() && f.graphOwner->Closed() && f.code->Closed()
  && f.result.closed && !f.result.runs && f.result.installedWords==867448 && f.result.restoredWords==867448
  && f.io.bootCount==119 && f.io.rootCount==1 && f.result.stopped==119 && f.result.held==119);
 assert(!f.mutableOwner->Ffn(f.id)->OwnsGraphLease(f.graphOwner.get()));
 PersistentOuterEnd(f);
 puts("PASS unused persistent full-FFN session:actual119boots/ROOT STOP/ALL current terminals BEFORE FIRSThold/typed graph claim released ONLY after exact867448word restore/original parentCLOSE+root residentEND restore ALL120outer snapshots;NO_FFN_NUMERIC_NATIVE_OR_SPEED_CLAIM");
}
