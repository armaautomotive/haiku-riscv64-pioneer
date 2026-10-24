/* SPDX-License-Identifier: MIT */
// ACTUAL SAME native context/factories/zero RV32 bytes/ALL119 audit/CLOSE.
// Only OS/PCI/L1/NoC/GDDR endpoints simulated;no private ready seal minted.
#define PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_ONLY
#include "pioneer_q38_resident_decoder_native_factory_test.cpp"
int main(int argc,char** argv){
 assert(argc==8 && (!strcmp(argv[7],"--gdn-state-init") || !strcmp(argv[7],"--gdn-state-init-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 DecoderNativeFixture f(argv[3],argv[4],argv[5]);assert(BindQ38EmbeddingModel(*f.c,0,0,0));auto* m=f.c->q38EmbeddingModel;
 auto* state=m->graph.Recurrent(m->identity);assert(state && !m->MatchesInitializedGdn(f.owner.banks,f.owner.dram));
 unsigned before=f.io.touches;PioneerQ38StateZeroResult result;
 assert(!InitializeQ38ConvHistoryModel(*f.c,f.historyResult) && !f.Initialize() && f.io.touches==before);
 // Public counts/flags cannot substitute for the private full zero ledger.
 {auto saved=m->gdnZero;m->gdnZero.saved=m->gdnZero.closed=true;m->gdnZero.nonce=672;m->gdnZero.identity=m->identity;
  assert(!m->MatchesInitializedGdn(f.owner.banks,f.owner.dram) && !InitializeQ38ConvHistoryModel(*f.c,f.historyResult)
   && !f.Initialize() && f.io.touches==before);m->gdnZero=saved;}
 m->phase=PioneerQ38EmbeddingNativeState::GdnZeroing;assert(f.io.RecurrentZeroSourceScope());unsigned refusals=0;
 auto refuse=[&](auto& field,auto bad){auto saved=field;field=bad;assert(!f.io.RecurrentZeroSourceScope());field=saved;refusals++;};
 refuse(currentTeam,8);refuse(f.cookie.team,8);refuse(failThreadInfo,true);refuse(currentThread,43);
 refuse(sRuntimeOwner,nullptr);refuse(f.cookie.context,nullptr);refuse(f.cookie.magic,0U);
 refuse(sColdRecoveryRequired,true);refuse(sReloadQuiesced,true);refuse(sReloadGeneration,sReloadGeneration+1);
 refuse(f.c->initialized,nullptr);refuse(f.c->hashes,nullptr);refuse(m->ledger,nullptr);
 refuse(f.io.q38MutableContext,nullptr);refuse(f.io.runtimeMapped,false);refuse(f.io.runtimeCore,1U);
 refuse(f.io.fleetEnabled,true);refuse(f.io.q38SpanAllowed,true);refuse(f.c->q38GdnCallThread,-1);
 refuse(f.c->q38DispatchRoutes,true); // checked separately below:may NOT add competing route authority
 refuse(sBankStorageOnly,false);refuse(f.c->bankStorageOnly,false);refuse(f.c->bytes,f.c->bytes-4096);refuse(f.c->runtimeCoreMode,-1);
 refuse(f.c->dispatchOwnerRetained,true);refuse(m->identity.modelEpoch,m->identity.modelEpoch+1);
 refuse(m->identity.driverGeneration,m->identity.driverGeneration+1);refuse(m->position,1U);refuse(m->token,248320U);refuse(m->bank,7U);
 refuse(f.c->q38FfnModelPlan,static_cast<void*>(m));refuse(f.c->q38GraphMutable,static_cast<void*>(m));
 refuse(f.c->q38FfnZero,static_cast<void*>(m));refuse(f.c->q38FfnGraph,static_cast<void*>(m));refuse(f.c->q38FfnCode,static_cast<void*>(m));
 refuse(f.c->q38FfnModelIdentity.session,UINT64_C(1));refuse(f.c->q38FfnModelIdentity.sequence,UINT64_C(1));
 refuse(f.c->q38FfnModelIdentity.modelEpoch,UINT64_C(1));refuse(f.c->q38FfnModelIdentity.driverGeneration,UINT64_C(1));
 refuse(f.c->q38GdnBanks,state);refuse(f.c->q38StateZero,&m->gdnZero);
 refuse(f.c->q38GdnIdentity.session,UINT64_C(1));refuse(f.c->q38GdnIdentity.sequence,UINT64_C(1));
 refuse(f.c->q38GdnIdentity.modelEpoch,UINT64_C(1));refuse(f.c->q38GdnIdentity.driverGeneration,UINT64_C(1));
 refuse(f.c->q38Cohort,static_cast<void*>(m));refuse(f.c->fleet,static_cast<void*>(m));refuse(sQ38LocalDeltaOwner,static_cast<const void*>(m));
 refuse(f.c->q38Delta.saved,true);refuse(f.c->q38Delta.dirty,true);refuse(f.c->q38Delta.ready,true);refuse(f.c->q38Delta.running,true);refuse(f.c->q38Delta.awaiting,true);
 refuse(f.c->panelProbe.saved,true);refuse(f.c->panelProbe.dirty,true);refuse(f.c->panelProbe.ready,true);
 refuse(f.c->streamProbe.saved,true);refuse(f.c->streamProbe.dirty,true);refuse(f.c->streamProbe.ready,true);
 refuse(f.c->q38Span,nullptr);refuse(f.c->span.closed,false);refuse(f.c->span.dirty,true);refuse(f.c->span.ready,true);refuse(f.c->span.running,true);refuse(f.c->span.unsafe,true);
 refuse(f.c->lease.saved,nullptr);refuse(f.c->lease.ready,false);refuse(f.c->lease.pending,true);refuse(f.c->lease.recovery_required,true);
 refuse(f.c->bulk.saved,false);refuse(f.c->bulk.ready,false);refuse(f.c->bulk.unsafe,true);
 refuse(f.c->host.state,0U);refuse(f.c->host.released,1U);refuse(f.io.PioneerHostPageIO::quarantined,true);
 refuse(f.c->bulkHost.allocated,false);refuse(f.c->bulkHost.ready,false);refuse(f.c->bulkHost.pending,true);refuse(f.c->bulkHost.quarantined,true);refuse(f.c->bulkHost.released,true);
 refuse(f.c->wideHost.allocated,false);refuse(f.c->wideHost.ready,false);refuse(f.c->wideHost.memory_type_set,false);refuse(f.c->wideHost.pending,true);
 refuse(f.c->wideHost.quarantined,true);refuse(f.c->wideHost.released,true);refuse(f.io.wideQuarantined,true);
 refuse(m->zero.saved,true);refuse(m->zero.closed,true);refuse(m->historyZero.saved,true);refuse(m->historyZero.closed,true);
 refuse(m->gdnZero.closed,true);refuse(m->gdnZero.saved,true);refuse(m->gdnZero.nonce,1U);refuse(m->gdnZero.identity.session,UINT64_C(1));
 refuse(f.c->q38Clock.required,false);refuse(f.c->q38Clock.value.state,0U);refuse(f.c->q38Clock.value.busy_requested,0U);
 refuse(f.c->q38Clock.value.busy_acknowledged,0U);refuse(f.c->q38Clock.value.restored,1U);refuse(f.c->otherUnsafe,true);
 assert(f.io.touches==before);m->phase=PioneerQ38EmbeddingNativeState::Bound;
 if(!strcmp(argv[7],"--gdn-state-init-faults")){
  for(unsigned fault=0;fault<6;fault++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    if(!fault)f.io.recurrentFailRun=1;if(fault==1)f.io.dropEntry=f.io.entryWrites+1;
    if(fault==2)currentTeam=8;if(fault==3)f.io.recurrentFailRun=336;if(fault==4)f.io.recurrentFailRun=672;
    if(fault==5)f.io.recurrentCloseFault=true;
    assert(!InitializeQ38GdnStateModel(*f.c,result) && !state->InitializationPublished(m->identity)
     && m->Unsafe() && !m->Closed() && f.c->dispatchOwnerRetained && sColdRecoveryRequired && f.c->bulk.unsafe);
    unsigned n=f.io.touches;assert(!InitializeQ38GdnStateModel(*f.c,result) && !InitializeQ38ConvHistoryModel(*f.c,f.historyResult)
     && !f.Initialize() && !EndFactory(*f.c) && f.io.touches==n);
    printf("PASS native GDN state-init fault%u retainedALL/no initialized publication/ZERO laterIO\n",fault);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
  puts("PASS6 ACTUAL native GDN zero/init factory unknown-first-middle-last-packet/lostboot/team/FULL73728pages-but-uncleanCLOSE faults;SIMULATED endpoints,NOT_CARD");return 0;
 }
 assert(InitializeQ38GdnStateModel(*f.c,result) && m==f.c->q38EmbeddingModel && m->phase==PioneerQ38EmbeddingNativeState::Bound
  && m->MatchesInitializedGdn(f.owner.banks,f.owner.dram) && f.io.recurrentPages==73728 && f.io.recurrentRuns==672
  && m->gdnZero.nonce==672 && m->gdnZero.saved && m->gdnZero.closed && !m->gdnZero.running);
 for(auto word:f.io.recurrentWords)assert(!word);for(auto count:f.io.recurrentTouched)assert(count==1);
 unsigned progress=0;assert(state->InitializationProgress(m->identity,progress) && progress==73728);
 before=f.io.touches;assert(!InitializeQ38GdnStateModel(*f.c,result) && f.io.touches==before);
 auto proof=f.owner.dram;assert(!m->MatchesInitializedGdn(f.owner.banks,proof));
 assert(InitializeQ38ConvHistoryModel(*f.c,f.historyResult) && f.Initialize() && f.Run() && RunQ38LearnedRmsModel(*f.c,f.norm)
  && m->Closed() && EndFactory(*f.c) && !f.c->Unsafe() && !sColdRecoveryRequired);
 printf("PASS ACTUAL SAME native GDN BOTHslots zero672regions/73728pages/301989888bytes/ALLdata+pagecoverage+zeroCLOSE publication->history336x5->workspace512->QPE1/QRN1/actualEND;scopeRefusals%u/no fabricated ready/no Work_CPUreference;SIMULATED endpoints NOT_CARD_RECURRENCE_SPEED\n",refusals);
}
