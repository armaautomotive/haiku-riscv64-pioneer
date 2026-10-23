/* SPDX-License-Identifier: MIT */
// SAME original native context/cookie/heap model from unclaimed source
// through actual zero/QPE1/QRN1 and native QPR1+ALL119 QFP3 factories.
// Only physical PCI/L1/GDDR/clock endpoints are simulated.
#define PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
#define PIONEER_Q38_RMS_FACTORY_FIXTURE_ONLY
#include "pioneer_q38_learned_rms_native_factory_test.cpp"
struct ProjectionNativeFixture:NativeFactoryFixture {
 PioneerQ38LearnedRmsResult norm;PioneerQ38ProjectionGraphResult projection;
 std::unique_ptr<ProjectionNativeTransport> transport;
 ProjectionNativeFixture(const char* zero,const char* embedding,const char* rms,unsigned token=0,unsigned bank=0):NativeFactoryFixture(zero,embedding,token,bank){
  io.rmsImage=rms;auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);
   if(p->kind!=plan->WeightKind || p->layer || (p->role!=0 && p->role!=5 && p->role!=6 && p->role!=10 && p->role!=11))continue;
   unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
   for(unsigned n=0;n<p->allocated/4096;n++)learned[first+n]=1;
  }
 }
 unsigned MissingPagesRefusals(){
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned refusals=0,n=io.touches;
  uint64 epoch=sQ38EmbeddingModelEpoch;
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);
   if(p->kind!=plan->WeightKind || p->layer || (p->role!=5 && p->role!=6 && p->role!=10 && p->role!=11))continue;
   unsigned page=p->bank*unsigned(plan->Capacity/4096)+(p->offset+p->allocated-4096)/4096;assert(learned[page]==1);
   learned[page]=0;assert(!Bind() && !c->q38EmbeddingModel && !c->q38Dispatch && c->runtimeCoreMode==-1
    && sQ38EmbeddingModelEpoch==epoch && io.touches==n);learned[page]=1;refusals++;
  }
  assert(refusals==16);return refusals;
 }
 void Produce(){
  assert(!c->q38EmbeddingModel && !c->q38Dispatch && Bind());auto* same=c->q38EmbeddingModel;
  assert(Initialize() && Run() && RunQ38LearnedRmsModel(*c,norm));
  assert(c->q38EmbeddingModel==same && same->Closed() && same->rms.Completed()
   && same->phase==PioneerQ38EmbeddingNativeState::RmsComplete && !c->q38Dispatch && !c->Unsafe() && EndFactory(*c));
 }
 void Attach(const char* root,const char* peer){
  std::vector<uint32> addresses(73737),values(73737);uint32 sources[16],commands[10],counts[8];
  for(unsigned i=0;i<73737;i++){addresses[i]=PioneerQ38EmbeddingSnapshotAddress(i);values[i]=io.RawSlot(addresses[i]);}
  for(unsigned i=0;i<16;i++)sources[i]=i?io.source[i]:io.config;
  for(unsigned i=0;i<10;i++)commands[i]=io.command[i];
  for(unsigned i=0;i<8;i++)counts[i]=io.PanelIO::ResidentNocRead(PioneerQ38SpanCounter(i));
  transport.reset(new ProjectionNativeTransport(root,peer,io.wholeWorkspace));
  transport->SeedRoot(addresses.data(),values.data(),unsigned(values.size()),sources,commands,counts);
  transport->Observe(projection);
  io.projectionTransport=transport.get();
 }
 unsigned ResultRefusals(){
  static_assert(sizeof(projection)==31*sizeof(uint32),"audit EVERY public result word");
  uint32 words[31];memcpy(words,&projection,sizeof(words));unsigned n=io.touches,d=transport->Touches();
  for(unsigned i=0;i<31;i++){
   words[i]^=1;memcpy(&projection,words,sizeof(words));
   assert(!PrepareQ38ProjectionExecution(*c,projection) && !c->q38Dispatch && !c->q38EmbeddingModel->projectionCohort
    && !c->q38EmbeddingModel->projection.Claimed() && !c->q38EmbeddingModel->Unsafe()
    && c->q38EmbeddingModel->phase==PioneerQ38EmbeddingNativeState::RmsComplete && io.touches==n && transport->Touches()==d);
   words[i]^=1;
  }
  memcpy(&projection,words,sizeof(words));return 31;
 }
 unsigned ScopeRefusals(bool live=false){
  assert(io.ProjectionExecutionSourceScope(false) && io.ProjectionExecutionSourceScope(true)==live);
  unsigned touched=io.touches,refusals=0,device=transport->Touches();auto& m=*c->q38EmbeddingModel;
  auto refuse=[&](auto& value,auto bad){auto saved=value;value=bad;bool denied=!io.ProjectionExecutionSourceScope(live);value=saved;
   if(!denied)fprintf(stderr,"NATIVE_SCOPE_REFUSAL_UNEXPECTED_ACCEPT index%u live%u\n",refusals,unsigned(live));assert(denied);refusals++;};
  refuse(sColdRecoveryRequired,true);refuse(sReloadQuiesced,true);refuse(sReloadGeneration,sReloadGeneration+1);
  refuse(sRuntimeOwner,nullptr);refuse(cookie.magic,0U);refuse(cookie.context,nullptr);refuse(sBankStorageOnly,false);
  refuse(c->bankStorageOnly,false);refuse(c->bytes,c->bytes-4096);refuse(c->runtimeCoreMode,-1);
  refuse(io.runtimeCore,120U);refuse(io.fleetEnabled,!live);refuse(io.runtimeMapped,false);refuse(io.q38SpanAllowed,true);
  refuse(c->q38GdnCallThread,-1);refuse(currentThread,43);refuse(c->dispatchOwnerRetained,true);
  refuse(c->initialized,nullptr);refuse(c->hashes,nullptr);refuse(m.ledger,nullptr);refuse(io.q38MutableContext,nullptr);
  refuse(c->q38FfnModelPlan,static_cast<void*>(this));refuse(c->q38GraphMutable,static_cast<void*>(this));
  refuse(c->q38FfnZero,static_cast<void*>(this));refuse(c->q38FfnGraph,static_cast<void*>(this));refuse(c->q38FfnCode,static_cast<void*>(this));
  refuse(c->q38FfnModelIdentity.sequence,UINT64_C(1));refuse(c->q38GdnIdentity.session,UINT64_C(1));
  refuse(c->q38Cohort,static_cast<void*>(this));refuse(c->fleet,static_cast<void*>(this));refuse(sQ38LocalDeltaOwner,static_cast<const void*>(this));
  refuse(c->q38Delta.saved,true);refuse(c->q38Delta.awaiting,true);refuse(c->panelProbe.ready,true);refuse(c->streamProbe.dirty,true);
  refuse(c->q38Span,nullptr);refuse(c->span.closed,false);refuse(c->span.running,true);refuse(c->span.unsafe,true);
  refuse(c->lease.saved,nullptr);refuse(c->lease.ready,false);refuse(c->lease.pending,true);refuse(c->lease.recovery_required,true);
  refuse(c->bulk.saved,false);refuse(c->bulk.ready,false);refuse(c->bulk.dirty,true);refuse(c->bulk.unsafe,true);
  refuse(c->host.state,0U);refuse(c->host.released,1U);refuse(io.PioneerHostPageIO::quarantined,true);
  refuse(c->bulkHost.ready,false);refuse(c->bulkHost.pending,true);refuse(c->bulkHost.quarantined,true);refuse(c->bulkHost.released,true);
  refuse(c->wideHost.ready,false);refuse(c->wideHost.memory_type_set,false);refuse(c->wideHost.pending,true);refuse(c->wideHost.quarantined,true);
  refuse(io.wideQuarantined,true);refuse(m.zero.dirty,true);refuse(m.zero.closed,false);refuse(m.zero.nonce,1U);refuse(m.zero.proof,nullptr);
  refuse(c->q38Clock.required,false);refuse(c->q38Clock.value.state,0U);refuse(c->q38Clock.value.busy_requested,0U);
  refuse(c->q38Clock.value.busy_acknowledged,0U);refuse(c->q38Clock.value.restored,1U);
  refuse(c->q38DispatchRoutes,!live);
  if(live){
   refuse(m.projectionCohort,nullptr);refuse(c->q38Dispatch,nullptr);
   for(unsigned w:{0U,1U,119U}){auto& node=c->q38Dispatch->worker[w];
    refuse(node.image.saved,false);refuse(node.image.ready,false);refuse(node.image.unsafe,true);refuse(node.image.running,!bool(w));
    refuse(node.Store().ready,false);refuse(node.Store().pending,true);refuse(node.Store().recovery_required,true);
   }
  }else refuse(io.q38ProjectionMemoryMapAttempted,true);
  refuse(m.phase,PioneerQ38EmbeddingNativeState::Complete);
  assert(io.touches==touched && transport->Touches()==device && io.ProjectionExecutionSourceScope(false));return refusals;
 }
 void Unknown(){
  auto& m=*c->q38EmbeddingModel;
  assert(m.Unsafe() && !m.Closed() && c->dispatchOwnerRetained && owner.banks.Quarantined() && sColdRecoveryRequired
   && c->lease.recovery_required && transfer.quarantined && wide.quarantined && io.retained && c->q38Clock.quarantined);
  if(c->q38Dispatch){assert(c->q38Dispatch->unsafe);for(unsigned w=0;w<c->q38Dispatch->prepared;w++){
   assert(c->q38Dispatch->worker[w].image.unsafe && c->q38Dispatch->worker[w].Store().recovery_required);
  }}
  unsigned n=io.touches,d=transport->Touches();
  assert(!PrepareQ38ProjectionExecution(*c,projection) && !RunQ38ProjectionExecution(*c,10,projection)
   && !CloseQ38ProjectionExecution(*c,projection) && !EndFactory(*c)
   && io.touches==n && transport->Touches()==d); // NO subsequent endpoint read/STOP/restore/retry
 }
};
#ifndef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_ONLY
int main(int argc,char** argv){
 assert(argc==6 || argc==7);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 bool wide=argc==7 && !strcmp(argv[6],"--wide");int fault=argc==7 && !wide?atoi(argv[6]):-1;
 ProjectionNativeFixture f(argv[3],argv[4],argv[5]);
 printf("NATIVE_PROJECTION_PREBIND_REFUSALS%u NO IO/claim/epoch change\n",f.MissingPagesRefusals());
 puts("NATIVE_PROJECTION_GENUINE_SOURCE_ZERO_QPE1_QRN1");f.Produce();auto* sameModel=f.c->q38EmbeddingModel;
 f.Attach(argv[1],argv[2]);unsigned refused=f.ScopeRefusals();
 printf("NATIVE_PROJECTION_SCOPE_REFUSALS%u NO endpoint IO\n",refused);
 printf("NATIVE_PROJECTION_RESULT_REFUSALS%u NO IO/claim\n",f.ResultRefusals());
 if(fault>=20 && fault<=22)f.io.projectionFailMap=fault-20;
 if(fault>=23 && fault<=25)f.io.projectionNullMap=fault-23;
 puts("NATIVE_PROJECTION_ACTUAL_PREPARE_ALL120");bool prepared=PrepareQ38ProjectionExecution(*f.c,f.projection);
 if(fault>=20 && fault<=25){assert(!prepared && !f.transport->Boots());f.Unknown();printf("PASS native projection sticky partial map fault%d;ALL retained/inert\n",fault);return 0;}
 if(!prepared)fprintf(stderr,"NATIVE_PREPARE_FAILED phase%u worker%u word%u memory%u/%u/%u prepared%u\n",f.projection.phase,f.projection.worker,f.projection.word,
  f.projection.memory.phase,f.projection.memory.worker,f.projection.memory.word,f.c->q38Dispatch?f.c->q38Dispatch->prepared:0);
 assert(prepared && f.c->q38EmbeddingModel==sameModel && f.c->q38Dispatch==sameModel->projectionCohort
  && f.c->q38Dispatch->worker[0].master==&f.c->lease && f.c->q38DispatchRoutes && f.transport->Boots()==119
  && f.projection.memory.savedWords==8134072 && f.projection.memory.installedWords==989304);
 assert(f.c->Unsafe() && !f.c->Unsafe(true,false,true) && f.io.ProjectionExecutionSourceScope(true));
 printf("NATIVE_PROJECTION_LIVE_SCOPE_REFUSALS%u NO endpoint IO\n",f.ScopeRefusals(true));
 unsigned n=f.io.touches,d=f.transport->Touches();
 for(unsigned role:{0U,2U,4U,13U})assert(!RunQ38ProjectionExecution(*f.c,role,f.projection));
 assert(f.io.touches==n && f.transport->Touches()==d && !sameModel->Unsafe());
 if(fault==0)currentThread++;
 if(fault>0 && fault!=9 && fault!=10)f.transport->Fault(unsigned(fault));
 if(fault==13){assert(!EndFactory(*f.c));f.Unknown();puts("PASS native projection unfinished END retains/inert");return 0;}
 auto originalInput=f.io.wholeWorkspace;
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
 std::vector<uint32> preserved(originalInput.begin()+TT_Q38_PROJECTION_RESULT_BASE/4,
  originalInput.begin()+(TT_Q38_PROJECTION_RESULT_BASE+TT_Q38_PROJECTION_RESULT_BYTES)/4);
#endif
 for(unsigned role:wide?std::vector<unsigned>{10,11,5,6}:std::vector<unsigned>{10,11}){
  printf("NATIVE_PROJECTION_ACTUAL_RUN_ROLE%u\n",role);bool okay=RunQ38ProjectionExecution(*f.c,role,f.projection);
  if(fault>=0 && fault<=12 && fault!=9 && fault!=10){assert(!okay);f.Unknown();printf("PASS native projection fault%d;ALL retained/no subsequent endpoint IO\n",fault);return 0;}
  if(!okay){fprintf(stderr,"NATIVE_RUN_FAILED phase%u worker%u word%u\n",f.projection.phase,f.projection.worker,f.projection.word);f.transport->Diagnostic();}
  assert(okay && f.transport->Boots()==119);f.transport->VerifyOutput();
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
  unsigned at=(PioneerQ38ProjectionResultOffset(role)-TT_Q38_PROJECTION_RESULT_BASE)/4,rows=PioneerQ38ProjectionResultRows(role);
  for(unsigned i=0;i<preserved.size();i++)if(i<at || i>=at+rows)assert(f.io.wholeWorkspace[TT_Q38_PROJECTION_RESULT_BASE/4+i]==preserved[i]);
  for(unsigned i=0;i<rows;i++)preserved[at+i]=f.io.wholeWorkspace[TT_Q38_PROJECTION_RESULT_BASE/4+at+i];
#endif
  for(unsigned i=0;i<5120;i++)assert(f.io.wholeWorkspace[i]==originalInput[i]);
 }
 if(fault==9)f.transport->Fault(1);if(fault==10)f.transport->Fault(10);
 puts("NATIVE_PROJECTION_ACTUAL_CLOSE_STOP_RESTORE_END");bool closed=CloseQ38ProjectionExecution(*f.c,f.projection);
 if(fault==9 || fault==10){assert(!closed);f.Unknown();printf("PASS native projection close fault%d;ALL retained/no subsequent endpoint IO\n",fault);return 0;}
 assert(closed);
 assert(f.c->q38EmbeddingModel==sameModel && sameModel->Closed() && !f.c->Unsafe() && EndFactory(*f.c)
  && f.projection.stopped==119 && f.projection.held==119 && f.projection.memory.restoredWords==8134072
  && f.projection.memory.verifiedOriginalWords==16268144 && !f.c->dispatchOwnerRetained && !sColdRecoveryRequired);
 assert(f.c->q38Dispatch->closed && sameModel->Closed() && !f.c->Unsafe());
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
 for(unsigned i=0;i<preserved.size();i++)assert(f.io.wholeWorkspace[TT_Q38_PROJECTION_RESULT_BASE/4+i]==preserved[i]);
 puts("PASS QPS1 native factories preserve ALL earlier results/future slots/padding across roles and actual native END;SIMULATED HARDWARE ONLY");
#endif
 printf("PASS SAME native context source->zero2MiB->QPE1->QRN1->actual ALL120 native preparation/maps/scope/factory->QPR1+ALL119QFP3:projections%u/matrices%u/producers%u/boots%u/full restore/actual complete dispatch END;Work controller NO CPU numeric input/reference/fallback;SIMULATED_HARDWARE_NOT_CARD_SPEED\n",
  wide?4U:2U,f.transport->Matrices(),f.transport->Producers(),f.transport->Boots());
}
#endif
