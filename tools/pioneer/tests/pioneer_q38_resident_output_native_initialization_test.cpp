/* SPDX-License-Identifier: MIT */
// ACTUAL new native context/zero factories/scopes/ALL119 held audits.
// Frozen RV32 initializes real private ledgers. Only OS/byte endpoints TEST.
#define PIONEER_Q38_OUTPUT_BINDING_FIXTURE_ONLY
#include "pioneer_q38_resident_output_native_binding_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
static void Unknown(NativeBindingFixture& f){
 auto& c=*f.c;auto* state=c.q38ResidentOutputNative;auto* m=c.q38EmbeddingModel;
 assert(state && state->Unsafe() && m && m->Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired
  && c.banks.Quarantined() && c.lease.recovery_required && c.bulk.unsafe && f.transfer.quarantined && f.wide.quarantined
  && c.q38Clock.quarantined && f.io.PioneerHostPageIO::quarantined && !state->InitializationsClosed(c.banks,f.owner.dram));
 unsigned n=f.io.touches;PioneerQ38StateZeroResult gdn;PioneerQ38ConvHistoryZeroResult history;
 assert(!InitializeQ38ResidentOutputRecurrent(c,gdn) && !InitializeQ38ResidentOutputHistory(c,history)
  && !BindQ38ResidentOutputNativeState(c) && !f.Initialize() && !f.Run() && !EndFactory(c) && !EndFactory(c) && f.io.touches==n);
}
static void Faults(NativeBindingFixture& f,bool history){
 for(unsigned fault=0;fault<9;fault++){
  pid_t child=fork();assert(child>=0);if(!child){
   auto& c=*f.c;auto* m=c.q38EmbeddingModel;auto* g=m->graph.Recurrent(m->identity);auto* h=m->graph.ConvHistory(m->identity);
   assert(g && h);unsigned starts=f.io.rootStarts;
   if(fault<3){unsigned run=fault==0?1:fault==1?(history?168:336):(history?336:672);
    if(history)f.io.outputHistoryFailRun=run;else f.io.outputGdnFailRun=run;}
   if(fault==3){if(history)f.io.outputHistoryCloseFault=true;else f.io.outputGdnCloseFault=true;}
   if(fault==4)f.io.outputNonrootFault=true;
   if(fault==5)f.io.outputLoseTeamRun=1;
   if(fault==6){assert(f.io.clock<UINT64_MAX-100000);f.io.clock+=100000;f.io.outputBackwardsAfterStarts=starts;}
   if(fault==7)f.io.sourceFault=11;
   if(fault==8)f.io.lateCoreMapLoss=true;
   PioneerQ38StateZeroResult a;PioneerQ38ConvHistoryZeroResult b;
   assert(!(history?InitializeQ38ResidentOutputHistory(c,b):InitializeQ38ResidentOutputRecurrent(c,a)));
   if(fault==4)assert(f.io.rootStarts==starts);
   assert(!g->InitializationPublished(m->identity) && !h->InitializationPublished(m->identity));Unknown(f);
   printf("PASS NEW_NATIVE_%s fault%u runs%u/pages%u;actual controller retainsALL,NO publication or following IO/reset/STOP/END/restore/retry\n",
    history?"HISTORY":"GDN",fault,history?f.io.outputHistoryRuns:f.io.outputGdnRuns,f.io.outputGdnPages);
   _exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
}
int main(int argc,char** argv){
 assert(argc==5 && (!strcmp(argv[4],"--all") || !strcmp(argv[4],"--positive")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 NativeBindingFixture f(argv[1],argv[2]);f.io.rmsImage=argv[3];auto& c=*f.c;auto& m=*c.q38EmbeddingModel;
 assert(BindQ38ResidentOutputNativeState(c));auto* state=c.q38ResidentOutputNative;
 assert(state && !state->InitializationsClosed(c.banks,f.owner.dram));
 PioneerQ38StateZeroResult gdn;PioneerQ38ConvHistoryZeroResult history;
 unsigned n=f.io.touches;assert(!InitializeQ38ResidentOutputHistory(c,history) && !f.Initialize() && f.io.touches==n);
 assert(f.io.ResidentOutputInitializationSourceScope(false,true) && !f.io.StateZeroBeforeCohort()
  && !f.io.ConvHistoryZeroBeforeCohort() && !f.io.ResidentOutputInitializationSourceScope(true,true));
 // Wrong real source/team/host/clock at entry refuses BEFORE any new
 // phase/lease/card operation. A public result cannot mint private state.
 unsigned refusals=0;
 auto refuse=[&](auto& field,auto bad){auto saved=field;field=bad;
  assert(!InitializeQ38ResidentOutputRecurrent(c,gdn) && !f.io.ResidentOutputInitializationSourceScope(false,true)
   && !c.dispatchOwnerRetained && !state->Unsafe() && f.io.touches==n);field=saved;refusals++;};
 refuse(currentTeam,99);refuse(failThreadInfo,true);refuse(currentThread,99);refuse(f.cookie.magic,0U);refuse(f.cookie.context,nullptr);
 refuse(sRuntimeOwner,nullptr);refuse(sReloadGeneration,sReloadGeneration+1);refuse(c.initialized,nullptr);refuse(c.hashes,nullptr);
 refuse(m.identity.modelEpoch,m.identity.modelEpoch+1);refuse(c.q38GdnCallThread,-1);refuse(c.bulk.dirty,true);
 refuse(c.bulkHost.ready,false);refuse(c.wideHost.ready,false);refuse(c.q38Clock.value.busy_acknowledged,0U);
 if(!strcmp(argv[4],"--all"))Faults(f,false);
 assert(InitializeQ38ResidentOutputRecurrent(c,gdn) && !state->InitializationsClosed(c.banks,f.owner.dram)
  && f.io.outputGdnRuns==672 && f.io.outputGdnPages==73728 && !c.bulk.dirty);
 for(auto value:f.io.outputGdnWords)assert(!value);for(auto count:f.io.outputGdnTouched)assert(count==1);
 auto* g=m.graph.Recurrent(m.identity);auto* h=m.graph.ConvHistory(m.identity);assert(g && h);
 unsigned progress=0;assert(g->InitializationPublished(m.identity) && g->InitializationProgress(m.identity,progress) && progress==73728
  && !h->InitializationPublished(m.identity));
 n=f.io.touches;assert(!InitializeQ38ResidentOutputRecurrent(c,gdn) && !f.Initialize() && f.io.touches==n);
 if(!strcmp(argv[4],"--all"))Faults(f,true);
 assert(InitializeQ38ResidentOutputHistory(c,history) && state->InitializationsClosed(c.banks,f.owner.dram)
  && f.io.outputHistoryRuns==336 && h->InitializationPublished(m.identity) && !c.bulk.dirty);
 for(auto value:f.io.outputHistoryWords)assert(!value);for(auto count:f.io.outputHistoryTouched)assert(count==1);
 n=f.io.touches;assert(!InitializeQ38ResidentOutputHistory(c,history) && !InitializeQ38ResidentOutputRecurrent(c,gdn) && f.io.touches==n);
 assert(f.io.ResidentOutputPrefixInitializationScope());
 auto refusePrefix=[&](auto& field,auto bad){auto saved=field;field=bad;
  assert(!f.io.ResidentOutputPrefixInitializationScope() && !f.Initialize() && f.io.touches==n);field=saved;refusals++;};
 refusePrefix(m.token,m.token^1);refusePrefix(m.bank,(m.bank+1)%7);refusePrefix(m.identity.session,m.identity.session+1);
 refusePrefix(m.identity.modelEpoch,m.identity.modelEpoch+1);refusePrefix(m.ledger,nullptr);refusePrefix(c.hashes,nullptr);
 refusePrefix(f.cookie.team,99);refusePrefix(currentTeam,99);refusePrefix(sReloadGeneration,sReloadGeneration+1);
 PioneerQ38LearnedRmsResult rms;
 assert(f.Initialize() && f.Run() && RunQ38LearnedRmsModel(c,rms) && m.Closed() && !state->Closed(c.banks,f.owner.dram)
  && !state->Unsafe() && state->InitializationsClosed(c.banks,f.owner.dram));
 // A genuinely CLOSED old prefix MUST NOT free the still-unclosed new
 // lifetime. Actual native END refuses/retains before any legacy cleanup.
 n=f.io.touches;assert(!EndFactory(c) && f.io.touches==n);Unknown(f);
 printf("PASS ACTUAL NEW native FD/team/context BOTHslots GDN672regions/73728observed pages +history336x5observed pages/full zero-CLOSE private seals->workspace512/QPE1/QRN1 genuine prefix->old CLOSED cannot END new lifetime;entryRefusals%u/faultCases%u;NO fabricated ready or synthetic success/Work CPUreference. OS/byte endpoints TEST,NOT silicon/new output factory/full64layer or model-rate qualification\n",refusals,!strcmp(argv[4],"--all")?18:0);
}
