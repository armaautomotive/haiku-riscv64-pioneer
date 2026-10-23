/* SPDX-License-Identifier: MIT */
// REAL native state/factories/scope/retention/END,ACTUAL zero/QPE1/QRN1
// instructions. Simulated hardware endpoints,NOT a physical-card result.
#define PIONEER_Q38_EMBEDDING_FACTORY_FIXTURE_ONLY
#include "pioneer_q38_embedding_native_factory_test.cpp"
bool PioneerSessionIO::RmsMode()const{
 return q38MutableContext && q38MutableContext->q38EmbeddingModel && q38MutableContext->q38EmbeddingModel->phase==PioneerQ38EmbeddingNativeState::Rms;
}
void PioneerSessionIO::RmsResetWrite(uint32 value){
 Touch();resets.push_back(value);
 if(value==0x87ffff){if(coreReset==0x46800)rmsStops++;coreReset=value;return;}
 if(value==0x47800){rootHolds++;coreReset=value;return;}
 assert(value==0x46800 && coreReset==0x87ffff && rmsImage);coreReset=value;rmsStarts++;
 LearnedRmsExecutionBridge t;t.l1.resize(73737);
 for(unsigned i=0;i<73737;i++)t.l1[i]=RawSlot(PioneerQ38EmbeddingSnapshotAddress(i));
 for(unsigned i=0;i<16;i++)t.source[i]=i?source[i]:config;
 for(unsigned i=0;i<10;i++)t.command[i]=command[i];
 for(unsigned i=0;i<8;i++)t.counts[i]=PanelIO::ResidentNocRead(PioneerQ38SpanCounter(i));
 for(unsigned i=0;i<5120;i++){t.resident[i]=wholeWorkspace[i];t.backup[i]=wholeWorkspace[TT_Q38_RMS_BACKUP/4+i];}
 t.failPacket=rmsPacketFault;t.badCounter=rmsBadCounter;t.sourceFault=rmsSourceFault;t.descriptorFault=rmsDescriptorFault;
 ExecuteLearnedRmsBridge(rmsImage,t);rmsPc=t.pc;
 for(unsigned i=0;i<73737;i++)RawSlot(PioneerQ38EmbeddingSnapshotAddress(i))=t.l1[i];
 for(unsigned i=0;i<10;i++)command[i]=t.command[i];
 counters[0]=t.counts[0];counters[1]=t.counts[1];accepted=t.counts[2];counters[3]=t.counts[3];
 writeCounters[0]=t.counts[4];writeCounters[1]=t.counts[5];writeCounters[3]=t.counts[7];
 for(unsigned i=1;i<16;i++)source[i]=t.source[i];
 for(unsigned i=0;i<5120;i++){wholeWorkspace[i]=t.resident[i];wholeWorkspace[TT_Q38_RMS_BACKUP/4+i]=t.backup[i];}
 if(rmsSummaryFault<32)RawSlot(TT_Q38_FFN_RESIDENT_SUMMARY+rmsSummaryFault*4)^=1;
 if(rmsPostFault==1)command[9]^=1;
 if(rmsPostFault==2)source[15]^=1;
 if(rmsPostFault==3)coreMapLost=true;
 if(rmsPostFault==4)runtimeMapped=false;
 if(rmsPostFault==5)prefix[10239]^=1;
 if(rmsPostFault==6)counters[0]++;
}
// TEST ONLY friend observes private proof. Never creates or copies one.
#ifndef PIONEER_Q38_RMS_FACTORY_FIXTURE_ONLY
class PioneerQ38ProjectionGraphOwner {
public:
 static bool Raw(const PioneerQ38EmbeddingNativeState& m){return m.embedding.MatchesCompleted(m.graph,m.plan,m.identity,m.token,m.position,m.bank);}
 static bool Normalized(const PioneerQ38EmbeddingNativeState& m){return m.rms.MatchesCompleted(m.graph,m.plan,m.identity,m.token,m.position,m.bank);}
 static unsigned Refusals(const PioneerQ38EmbeddingNativeState& m){
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  auto graph=std::make_unique<PioneerQ38ProjectionMutableBanks>();
  assert(!m.rms.MatchesCompleted(m.graph,*plan,m.identity,m.token,m.position,m.bank)
   && !m.rms.MatchesCompleted(*graph,m.plan,m.identity,m.token,m.position,m.bank)
   && !m.rms.MatchesCompleted(m.graph,m.plan,m.identity,m.token^1,m.position,m.bank)
   && !m.rms.MatchesCompleted(m.graph,m.plan,m.identity,m.token,m.position^1,m.bank)
   && !m.rms.MatchesCompleted(m.graph,m.plan,m.identity,m.token,m.position,(m.bank+1)%7));
  for(unsigned part=0;part<4;part++){auto id=m.identity;if(!part)id.session++;if(part==1)id.sequence++;if(part==2)id.modelEpoch++;if(part==3)id.driverGeneration++;
   assert(!m.rms.MatchesCompleted(m.graph,m.plan,id,m.token,m.position,m.bank));}
  return 9;
 }
};
struct RmsFactoryFixture:NativeFactoryFixture {
 PioneerQ38LearnedRmsResult norm;
 unsigned learnedFirst=0;
 RmsFactoryFixture(const char* zero,const char* embedding,const char* rms,unsigned token=0,unsigned bank=0):NativeFactoryFixture(zero,embedding,token,bank){
  io.rmsImage=rms;
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind==plan->WeightKind && !p->layer && !p->role){
   assert(!learnedFirst && p->bytes==20480);learnedFirst=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
  }}
  assert(learnedFirst);for(unsigned i=0;i<5;i++)learned[learnedFirst+i]=1;
 }
 bool Normalize(){return RunQ38LearnedRmsModel(*c,norm);}
 unsigned ScopeRefusals(){
  auto& m=*c->q38EmbeddingModel;m.phase=PioneerQ38EmbeddingNativeState::Rms;
  assert(io.LearnedRmsSourceScope(false) && !io.LearnedRmsSourceScope(true) && !io.EmbeddingSourceScope(false)
   && c->Unsafe() && !c->Unsafe(true,false,true));
  unsigned n=io.touches,refusals=0;
  auto refuse=[&](auto& value,auto bad){auto old=value;value=bad;assert(!io.LearnedRmsSourceScope(false));value=old;refusals++;};
  refuse(sColdRecoveryRequired,true);refuse(sReloadQuiesced,true);refuse(sReloadGeneration,sReloadGeneration+1);
  refuse(sRuntimeOwner,nullptr);refuse(cookie.magic,0U);refuse(cookie.context,nullptr);refuse(sBankStorageOnly,false);
  refuse(c->bankStorageOnly,false);refuse(c->bytes,c->bytes-4096);refuse(c->runtimeCoreMode,-1);
  refuse(io.runtimeCore,1U);refuse(io.fleetEnabled,true);refuse(io.runtimeMapped,false);refuse(io.q38SpanAllowed,true);
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
  refuse(m.phase,PioneerQ38EmbeddingNativeState::Complete);
  assert(io.touches==n && io.LearnedRmsSourceScope(false));m.phase=PioneerQ38EmbeddingNativeState::Complete;return refusals;
 }
 void Unknown(){
  auto* m=c->q38EmbeddingModel;assert(m && m->rms.Unsafe() && !m->rms.Completed() && !PioneerQ38ProjectionGraphOwner::Raw(*m)
   && !PioneerQ38ProjectionGraphOwner::Normalized(*m));
  NativeFactoryFixture::Unknown();unsigned n=io.touches;assert(!Normalize() && io.touches==n);
 }
};
int main(int argc,char** argv){
 assert(argc==4 || argc==5);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positives=0,faults=0,refusals=0;
 if(argc==4){
  for(unsigned source=0;source<7;source++){
   RmsFactoryFixture f(argv[1],argv[2],argv[3],PioneerQ38EmbeddingFirst(source)+PioneerQ38EmbeddingRows(source)-1,(source+3)%7);
   unsigned n=f.io.touches;
   for(unsigned p=0;p<5;p++){f.learned[f.learnedFirst+p]=0;assert(!f.Bind() && !f.c->q38EmbeddingModel && f.io.touches==n);f.learned[f.learnedFirst+p]=1;refusals++;}
   assert(f.Bind() && f.Initialize() && f.Run());auto& m=*f.c->q38EmbeddingModel;
   assert(m.Closed() && PioneerQ38ProjectionGraphOwner::Raw(m) && !PioneerQ38ProjectionGraphOwner::Normalized(m));
   assert(!f.io.LearnedRmsSourceScope(false) && !f.io.EmbeddingSourceScope(false));refusals+=2;
   refusals+=f.ScopeRefusals();
   std::vector<uint32> saved(73737);for(unsigned i=0;i<73737;i++)saved[i]=f.io.RawSlot(PioneerQ38EmbeddingSnapshotAddress(i));
   auto raw=f.io.wholeWorkspace;auto ledger=f.learned;
   assert(f.Normalize() && m.Closed() && !m.Unsafe() && m.phase==PioneerQ38EmbeddingNativeState::RmsComplete
    && m.rms.Completed() && !PioneerQ38ProjectionGraphOwner::Raw(m) && PioneerQ38ProjectionGraphOwner::Normalized(m)
    && f.io.rmsStarts==1 && f.io.rmsStops==1 && EndFactory(*f.c));
   refusals+=PioneerQ38ProjectionGraphOwner::Refusals(m);
   assert(f.norm.savedWords==73737 && f.norm.verifiedWords==147474 && f.norm.installedWords==10241 && f.norm.restoredWords==73737);
   for(unsigned i=0;i<73737;i++)assert(saved[i]==f.io.RawSlot(PioneerQ38EmbeddingSnapshotAddress(i)));
   bool changed=false;for(unsigned i=0;i<5120;i++){assert(f.io.wholeWorkspace[TT_Q38_RMS_BACKUP/4+i]==raw[i]);changed|=f.io.wholeWorkspace[i]!=raw[i];}assert(changed);
   for(unsigned i=5120;i<raw.size();i++)if(i<TT_Q38_RMS_BACKUP/4 || i>=TT_Q38_RMS_BACKUP/4+5120)assert(f.io.wholeWorkspace[i]==raw[i]);
   assert(f.learned==ledger && !f.io.pciWrites && !f.io.HostReadMock::bmeWrites && !f.io.wideReleased);
   n=f.io.touches;assert(!f.Normalize() && !f.Run() && f.io.touches==n && !f.io.LearnedRmsSourceScope(true));refusals+=3;
   printf("PASS ACTUAL native unclaimed->canonical bind/2MiB zero/QPE1->private QRN1->restore/END source%u/workspace%u,raw proof consumed/normalized sealed,raw residual preserved,zero host learned products\n",source,(source+3)%7);positives++;
  }
 }else{
  for(unsigned kind=0;kind<14;kind++)for(unsigned slot=0;slot<(kind<4?8U:kind==4?32U:kind==5?6U:kind==6?5U:kind==7?3U:kind==10?4U:kind==11?4U:kind==12?3U:kind==13?2U:1U);slot++){
   RmsFactoryFixture f(argv[1],argv[2],argv[3]);assert(f.Bind() && f.Initialize() && f.Run());
   if(kind==0)f.io.rmsPacketFault=slot+1;
   if(kind==1)f.io.rmsBadCounter=slot+1;
   if(kind==2)f.io.rmsSourceFault=slot+1;
   if(kind==3)f.io.rmsDescriptorFault=slot+1;
   if(kind==4)f.io.rmsSummaryFault=slot;
   if(kind==5)f.io.rmsPostFault=slot+1;
   if(kind==6)f.learned[f.learnedFirst+slot]=0;
   if(kind==7){if(slot==0)f.io.runtimeMapped=false;if(slot==1)f.c->q38GdnCallThread=-1;if(slot==2)f.c->q38Clock.value.restored=1;}
   if(kind==8)f.io.rmsReadFault=10240+1; // first prefix word of second ORIGINAL pass,before any image write
   if(kind==9){unsigned n=f.io.touches;assert(!EndFactory(*f.c) && f.io.touches==n);f.Unknown();faults++;continue;}
   if(kind==10){if(!slot)f.norm.version=2;if(slot==1)f.norm.before[0]=1;if(slot==2)f.norm.summary[31]=1;if(slot==3)f.norm.pc[1]=1;}
   if(kind==11){auto& m=*f.c->q38EmbeddingModel;if(!slot)m.token^=1;if(slot==1)m.position^=1;if(slot==2)m.bank=(m.bank+1)%7;if(slot==3)m.identity.modelEpoch++;}
   if(kind==12)f.io.wholeWorkspace[5119]=slot==0?0x7fc00000:slot==1?0x47800000:0x267fffff;
   if(kind==13)f.io.rmsReadFault=slot?71681:61441; // restore readback OR independent final original pass
   assert(!f.Normalize());if(kind>=6 && kind<12)assert(!f.io.rmsStarts);f.Unknown();faults++;
  }
 }
 printf("PASS ACTUAL native learned RMS FACTORY positives%u/refusals%u/faults%u;real source/state/scope/mapping/retention/END + emitted zero/QPE1/QRN1,simulated endpoints NOT CARD OR MODEL SPEED\n",positives,refusals,faults);
}
#endif
