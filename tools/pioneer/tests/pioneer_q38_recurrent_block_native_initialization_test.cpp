/* SPDX-License-Identifier: MIT */
// Actual new native source/initializers + frozen zero machine. OS/MMIO
// simulated. NO QRB1 launch,CPU reference inference or hardware permission.
#define PIONEER_Q38_RECURRENT_BINDING_FIXTURE_ONLY
#define PIONEER_Q38_RECURRENT_BLOCK_INITIALIZATION_FIXTURE
#include "pioneer_q38_recurrent_block_native_binding_test.cpp"
int main(int argc,char** argv){
 assert(argc==4 && (!strcmp(argv[3],"--initialize") || !strcmp(argv[3],"--init-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 bool faults=!strcmp(argv[3],"--init-faults");unsigned cases=faults?3:1;
 for(unsigned fault=0;fault<cases;fault++){
  RecurrentBindingFixture f(argv[1],argv[2]);f.Prefix();auto& c=*f.c;auto& io=f.io;auto& m=*c.q38EmbeddingModel;
  assert(BindQ38RecurrentBlockNative(c));auto* state=c.q38RecurrentBlockNative;auto& dram=c.baseline.baseline.baseline.baseline.dram;
  PioneerQ38StateZeroResult gdn;PioneerQ38ConvHistoryZeroResult history;
  assert(!state->InitializationsClosed(c.banks,dram) && !io.StateZeroBeforeCohort() && !io.ConvHistoryZeroBeforeCohort());
  unsigned touches=io.touches;
  assert(!InitializeQ38RecurrentBlockHistory(c,history) && io.touches==touches);
  auto refuse=[&](auto& field,auto bad){auto saved=field;field=bad;
   assert(!InitializeQ38RecurrentBlockState(c,gdn) && !InitializeQ38RecurrentBlockHistory(c,history)
    && io.touches==touches && !state->Unsafe() && !state->InitializationsClosed(c.banks,dram));field=saved;
  };
  refuse(c.q38GdnCallThread,-1);refuse(currentThread,99);refuse(currentTeam,99);refuse(c.bulk.dirty,true);
  refuse(sReloadGeneration,sReloadGeneration+1);refuse(m.position,1U);refuse(io.fleetEnabled,true);refuse(c.q38DispatchRoutes,true);
  assert(io.RecurrentBlockBaseScope() && io.RecurrentBlockInitializationScope(false,true));
  auto learned=f.learned;auto embedding=io.wholeWorkspace;
  auto panel=io.view;auto scratch=io.l1;uint32 entry=io.entry;
  if(faults){
   if(fault==0)io.recurrentBlockFailRun=1;
   if(fault==1)io.recurrentBlockNonrootFault=true;
   if(fault==2)io.recurrentBlockLoseTeamRun=1;
   assert(!InitializeQ38RecurrentBlockState(c,gdn) && state->Unsafe() && !state->InitializationsClosed(c.banks,dram)
    && c.dispatchOwnerRetained && c.banks.Quarantined() && c.bulk.unsafe && c.q38Clock.quarantined && sColdRecoveryRequired
    && f.transfer.quarantined && f.wide.quarantined && io.retained);
   touches=io.touches;
   assert(!InitializeQ38RecurrentBlockState(c,gdn) && !InitializeQ38RecurrentBlockHistory(c,history) && !EndFactory(c)
    && io.touches==touches && f.learned==learned && io.wholeWorkspace==embedding && !io.pciWrites);
   printf("PASS QRB native actual-init unknown%u/ALL owners retained/no publication,cleanup,retry or post-failure IO\n",fault);continue;
  }
  assert(InitializeQ38RecurrentBlockState(c,gdn) && io.recurrentBlockGdnRuns==672 && io.recurrentBlockGdnPages==73728
   && !state->InitializationsClosed(c.banks,dram) && !state->Unsafe() && io.view==panel && io.l1==scratch && io.entry==entry);
  for(auto value:io.recurrentBlockGdnWords)assert(!value);
  for(auto count:io.recurrentBlockGdnTouched)assert(count==1);
  touches=io.touches;assert(!InitializeQ38RecurrentBlockState(c,gdn) && io.touches==touches);
  assert(InitializeQ38RecurrentBlockHistory(c,history) && io.recurrentBlockHistoryRuns==336
   && state->InitializationsClosed(c.banks,dram) && io.view==panel && io.l1==scratch && io.entry==entry);
  for(auto value:io.recurrentBlockHistoryWords)assert(!value);
  for(auto count:io.recurrentBlockHistoryTouched)assert(count==1);
  assert(f.learned==learned && io.wholeWorkspace==embedding && !io.pciWrites && !io.HostReadMock::bmeWrites
   && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
  touches=io.touches;assert(!InitializeQ38RecurrentBlockState(c,gdn) && !InitializeQ38RecurrentBlockHistory(c,history)
   && io.touches==touches && io.RecurrentBlockBaseScope() && !state->Unsafe());
  // Initialized state is NOT a completed execution/END grant.
  assert(!EndFactory(c) && state->Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired && io.touches==touches);
  printf("PASS QRB native SAME completed-embedding owner/actual frozen zero 672 GDN regions73728pages+BOTHslots/336historyregions1680pages/clean private CLOSE/publication/ALL original code+scratch restored/embedding+learned unchanged/no BME;SIMULATED endpoints,NOT QRB1 launch/card/model TPS\n");
 }
}
