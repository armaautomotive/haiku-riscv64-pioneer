/* SPDX-License-Identifier: MIT */
// SAME actual native source/factories/scope/context/retention/END through
// history-zero/QPE1/QRN1/QPS1/QCB1. Only kernel/hardware/learned byte endpoints
// simulated. No private producer/ready capability is manufactured here.
#define PIONEER_Q38_PROJECTION_NATIVE_FACTORY_ONLY
#include "pioneer_q38_projection_execution_native_factory_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
struct DecoderNativeFixture:ProjectionNativeFixture {
 PioneerQ38ConvHistoryZeroResult historyResult;PioneerQ38ResidentDecoderResult decoderResult;
 DecoderNativeFixture(const char* zero,const char* embedding,const char* rms,unsigned selectedToken=0,unsigned selectedBank=0):ProjectionNativeFixture(zero,embedding,rms,selectedToken,selectedBank){
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer || p->role!=12)continue;
   for(unsigned page=0;page<p->allocated/4096;page++)learned[p->bank*786432+p->offset/4096+page]=1;
  }
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);
   if(p->kind==plan->WeightKind && !p->layer && (p->role==7 || p->role==8)){
    assert(p->allocated==4096);learned[p->bank*786432+p->offset/4096]=1;
   }
  }
#endif
 }
 unsigned PreBindRefusals(){
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned refusals=0,touched=io.touches;uint64 epoch=sQ38EmbeddingModelEpoch;
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer || p->role!=12)continue;
   unsigned last=p->bank*786432+(p->offset+p->allocated-4096)/4096;assert(learned[last]==1);learned[last]=0;
   assert(!BindQ38EmbeddingModel(*c,0,0,0) && !c->q38EmbeddingModel && io.touches==touched && sQ38EmbeddingModelEpoch==epoch);
   learned[last]=1;refusals++;
  }
  for(unsigned position:{1U,8191U,8192U}){assert(!BindQ38EmbeddingModel(*c,0,position,0) && !c->q38EmbeddingModel && io.touches==touched);refusals++;}
  currentTeam=8;assert(!BindQ38EmbeddingModel(*c,0,0,0) && !c->q38EmbeddingModel && io.touches==touched);currentTeam=7;refusals++;
  assert(!c->Unsafe() && !c->dispatchOwnerRetained && !sColdRecoveryRequired && sQ38EmbeddingModelEpoch==epoch);return refusals;
 }
 void ProduceNative(){
  assert(BindQ38EmbeddingModel(*c,token,0,bank));auto* same=c->q38EmbeddingModel;unsigned touched=io.touches;
  assert(!Initialize() && io.touches==touched && !same->historyZero.closed && !same->graph.ConvHistory(same->identity)->InitializationPublished(same->identity));
  assert(InitializeQ38ConvHistoryModel(*c,historyResult) && historyResult.nonce==336 && same->historyZero.closed
   && same->MatchesInitializedHistory(owner.banks,owner.dram));for(auto word:io.historyWords)assert(!word);
  assert(Initialize() && Run() && RunQ38LearnedRmsModel(*c,norm) && c->q38EmbeddingModel==same && same->Closed()
   && same->phase==PioneerQ38EmbeddingNativeState::RmsComplete && !c->Unsafe() && EndFactory(*c));
 }
 unsigned HistoryScopeRefusals(bool live=false){
  auto& m=*c->q38EmbeddingModel;unsigned touched=io.touches,device=transport->Touches(),refusals=0;
  assert(io.ProjectionExecutionSourceScope(live));
  auto refuse=[&](auto& value,auto bad){auto saved=value;value=bad;assert(!io.ProjectionExecutionSourceScope(live));value=saved;refusals++;};
  refuse(currentTeam,8);refuse(cookie.team,8);refuse(failThreadInfo,true);
  refuse(m.historyZero.saved,false);refuse(m.historyZero.closed,false);refuse(m.historyZero.nonce,335U);
  refuse(m.historyZero.ready,true);refuse(m.historyZero.running,true);refuse(m.historyZero.dirty,true);refuse(m.historyZero.unsafe,true);
  refuse(m.historyZero.owner,nullptr);refuse(m.historyZero.banks,nullptr);refuse(m.historyZero.proof,nullptr);
  auto proof=owner.dram;refuse(m.historyZero.proof,&proof);refuse(m.historyZero.identity.modelEpoch,m.historyZero.identity.modelEpoch+1);
  assert(io.ProjectionExecutionSourceScope(live) && io.touches==touched && transport->Touches()==device);return refusals;
 }
 void UnknownNative(){
  auto& m=*c->q38EmbeddingModel;
  assert(m.Unsafe() && !m.Closed() && c->dispatchOwnerRetained && owner.banks.Quarantined() && sColdRecoveryRequired
   && c->lease.recovery_required && bulk.unsafe && transfer.quarantined && wide.quarantined && io.retained && c->q38Clock.quarantined);
  if(c->q38Dispatch)assert(c->q38Dispatch->unsafe);
  unsigned touched=io.touches,device=transport?transport->Touches():0;
  assert(!InitializeQ38ConvHistoryModel(*c,historyResult) && !PrepareQ38ProjectionExecution(*c,projection)
   && !RunQ38ProjectionExecution(*c,10,projection) && !CloseQ38ProjectionExecution(*c,projection)
   && !RunQ38ResidentDecoderModel(*c,decoderResult) && !CloseQ38ResidentDecoderModel(*c,decoderResult)
   && !EndFactory(*c) && io.touches==touched && (!transport || transport->Touches()==device));
 }
};
#ifndef PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_ONLY
int main(int argc,char** argv){
 assert(argc==7 || (argc==8 && (!strcmp(argv[7],"--faults") || !strcmp(argv[7],"--history-faults"))));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 DecoderNativeFixture f(argv[3],argv[4],argv[5]);printf("DECODER_NATIVE_PREBIND_REFUSALS%u no epoch/claim/IO\n",f.PreBindRefusals());
 if(argc==8 && !strcmp(argv[7],"--history-faults")){
  for(unsigned fault=1;fault<=3;fault++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    assert(BindQ38EmbeddingModel(*f.c,0,0,0));auto* m=f.c->q38EmbeddingModel;auto* h=m->graph.ConvHistory(m->identity);assert(h);
    if(fault==1)f.io.packetFault=1;if(fault==2)f.io.dropEntry=f.io.entryWrites+1;if(fault==3)currentTeam=8;
    assert(!InitializeQ38ConvHistoryModel(*f.c,f.historyResult) && !h->InitializationPublished(m->identity)
     && !m->historyZero.closed && (fault==1?f.io.rootStarts==1:!f.io.rootStarts));
    f.UnknownNative();printf("PASS actual native history initializer fault%u retain exact snapshots/ALLhost leases/no later IO\n",fault);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
  puts("PASS3 actual native history initialization unknown packet/lost boot write/team loss,NO fake publication/cleanup/retry;SIMULATED HARDWARE ONLY");return 0;
 }
 f.ProduceNative();puts("DECODER_NATIVE_ACTUAL_SOURCE_HISTORY336X5_ZERO512_QPE1_QRN1");auto* same=f.c->q38EmbeddingModel;
 f.Attach(argv[1],argv[2]);f.transport->DecoderSources(argv[6],same->plan,same->bank,f.io);f.transport->ObserveDecoder(f.decoderResult);
 printf("DECODER_NATIVE_SCOPE_REFUSALS%u no endpoint IO\n",f.ScopeRefusals());
 printf("DECODER_NATIVE_HISTORY_TEAM_SCOPE_REFUSALS%u no endpoint IO\n",f.HistoryScopeRefusals());
 assert(PrepareQ38ProjectionExecution(*f.c,f.projection));unsigned touched=f.io.touches,device=f.transport->Touches();
 assert(!RunQ38ResidentDecoderModel(*f.c,f.decoderResult) && !same->decoder.Claimed() && same->phase==PioneerQ38EmbeddingNativeState::Projection
  && f.io.touches==touched && f.transport->Touches()==device && !same->Unsafe()); // no fabricated public-progress producer
 printf("DECODER_NATIVE_LIVE_SCOPE_REFUSALS%u no endpoint IO\n",f.ScopeRefusals(true));
 printf("DECODER_NATIVE_LIVE_HISTORY_TEAM_SCOPE_REFUSALS%u no endpoint IO\n",f.HistoryScopeRefusals(true));
 for(unsigned role:{10U,11U,5U,6U})assert(RunQ38ProjectionExecution(*f.c,role,f.projection));
 puts("DECODER_NATIVE_ACTUAL_QPS1_ALL4ROLES_SAME_CONTEXT");auto workspace=f.io.wholeWorkspace;
 if(argc==8){
  for(unsigned fault=1;fault<=12;fault++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    if(fault<=6)f.transport->DecoderFault(fault);
    if(fault==7)currentTeam=8;if(fault==8)sRuntimeOwner=nullptr;if(fault==9)same->historyZero.nonce=335;
    bool ran=RunQ38ResidentDecoderModel(*f.c,f.decoderResult);
    if(fault<=4 || (fault>=7 && fault<=9))assert(!ran);
    else if(fault==5 || fault==6)assert(ran && !CloseQ38ResidentDecoderModel(*f.c,f.decoderResult));
    else if(fault==10){assert(ran);currentTeam=8;assert(!CloseQ38ResidentDecoderModel(*f.c,f.decoderResult));}
    else if(fault==11){assert(ran && !EndFactory(*f.c));}
    else{assert(ran);same->historyZero.closed=false;assert(!CloseQ38ResidentDecoderModel(*f.c,f.decoderResult));}
    f.UnknownNative();printf("PASS actual native decoder factory fault%u ALL retained/no later source/status/END/restore IO\n",fault);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
  puts("PASS12 actual native retained-decoder source/team/cookie/history/partial-code/completion/counter/restore/unfinished-END faults;SIMULATED HARDWARE NOT CARD");return 0;
 }
 assert(RunQ38ResidentDecoderModel(*f.c,f.decoderResult) && f.c->q38EmbeddingModel==same && same->phase==PioneerQ38EmbeddingNativeState::DecoderHeld
  && same->decoder.Claimed() && !same->decoder.Closed() && !same->Closed() && f.transport->DecoderStarts()==120
  && f.c->Unsafe() && !f.c->Unsafe(true,false,true) && f.io.ProjectionExecutionSourceScope(true));
 for(unsigned channel=0;channel<10240;channel++){
  uint32 bits=workspace[PioneerQ38ProjectionResultOffset(6)/4+channel];float input;memcpy(&input,&bits,4);
  float weight=float(int((channel*7+3*11)%29)-14)*.03125f,sum=input*weight;double expected=double(sum)/(1+exp(-double(sum)));
  uint32 out=f.io.wholeWorkspace[TT_Q38_CONV_OUTPUT/4+channel];float actual;memcpy(&actual,&out,4);
  assert(isfinite(actual) && fabs(actual-expected)<=3e-6*(1+fabs(expected))
   && !f.io.wholeWorkspace[TT_Q38_CONV_NEXT/4+channel*3] && !f.io.wholeWorkspace[TT_Q38_CONV_NEXT/4+channel*3+1]
   && f.io.wholeWorkspace[TT_Q38_CONV_NEXT/4+channel*3+2]==bits);
 }
 for(unsigned i=0;i<workspace.size();i++)if(!(i>=TT_Q38_CONV_OUTPUT/4 && i<(TT_Q38_CONV_OUTPUT+40960)/4)
  && !(i>=TT_Q38_CONV_NEXT/4 && i<(TT_Q38_CONV_NEXT+122880)/4))assert(f.io.wholeWorkspace[i]==workspace[i]);
 touched=f.io.touches;device=f.transport->Touches();assert(!CloseQ38ProjectionExecution(*f.c,f.projection)
  && !RunQ38ResidentDecoderModel(*f.c,f.decoderResult) && f.io.touches==touched && f.transport->Touches()==device);
 assert(CloseQ38ResidentDecoderModel(*f.c,f.decoderResult) && same->phase==PioneerQ38EmbeddingNativeState::DecoderComplete && same->Closed()
  && same->decoder.MatchesClosedNativeOwner(same->projection,*same->graph.ConvHistory(same->identity))
  && f.decoderResult.memory.restoredWords==8134072 && f.decoderResult.memory.verifiedOriginalWords==8134072 && !f.c->Unsafe());
 assert(EndFactory(*f.c) && f.c->q38Dispatch->closed && same->Closed() && !f.c->dispatchOwnerRetained && !sColdRecoveryRequired);
 puts("PASS SAME ACTUAL native context/factories/source-cookie-team-thread-clock/ALLhosts/history336x5/zero512/QPE1/QRN1/QPS1ALL4->QCB1ALL120/640jobs/10240outputs+30720private next words/full8134072word restore/ACTUAL complete dispatch END;NO CPU-reference in controller;SIMULATED learned bytes/HARDWARE NOT CARD_OR_MODEL_SPEED");
}
#endif
