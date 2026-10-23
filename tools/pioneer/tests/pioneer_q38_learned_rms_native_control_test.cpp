/* SPDX-License-Identifier: MIT */
// ACTUAL tag41 public control/routing/global lock and native factory/END.
// Actual emitted zero/QPE1/QRN1;simulated OS/hardware,NOT a card result.
#define PIONEER_Q38_EMBEDDING_CONTROL_FIXTURE_ONLY
#include "pioneer_q38_embedding_native_control_test.cpp"
struct RmsControlFixture:NativeFactoryFixture {
 RmsControlFixture(const char* zero,const char* embedding,const char* rms,unsigned token=0)
  :NativeFactoryFixture(zero,embedding,token,0){
  io.rmsImage=rms;auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());bool found=false;
  for(unsigned i=0;i<plan->Count();i++){
   const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer || p->role)continue;
   assert(!found && p->bytes==20480);found=true;
   unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
   for(unsigned page=0;page<5;page++)learned[first+page]=1;
  }assert(found);ResetScope(*this);
 }
};
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positives=0,refusals=0,faults=0;
 for(unsigned bank=0;bank<7;bank++){
  RmsControlFixture f(argv[1],argv[2],argv[3],PioneerQ38EmbeddingFirst(bank)+PioneerQ38EmbeddingRows(bank)-1);
  tt_q38_embedding_support support={};unsigned touches=f.io.touches;
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT,&support,sizeof(support))==B_OK
   && support.version==1 && support.driver==55 && support.model_namespace==12 && !support.no_cpu_reference
   && !support.model_inference && f.io.touches==touches);
  auto reply=std::make_unique<tt_q38_learned_rms_validation>();reply->metadata=Request(f.token);
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_RMS_VALIDATE,reply.get(),sizeof(*reply))==B_OK);
  const auto& r=reply->metadata;
  assert(r.state==TT_RUNTIME_PASSED && r.closed && r.input_ready && !r.recovery_required && r.session
   && r.driver_generation==sReloadGeneration && r.source_pages==17 && r.workspace_pages==512
   && r.saved_words==147474 && r.restored_words==147474 && f.io.validationReads==10
   && f.c->q38EmbeddingModel->rms.Completed() && !locked && locks==releases && f.c->q38GdnCallThread==-1
   && !f.io.pciWrites && !f.io.HostReadMock::bmeWrites && EndFactory(*f.c));
  for(unsigned i=0;i<5120;i++)assert(reply->normalized[i]==f.io.wholeWorkspace[i]
   && reply->residual[i]==f.io.computedEmbedding[i] && reply->residual[i]==f.io.wholeWorkspace[TT_Q38_RMS_BACKUP/4+i]);
  for(unsigned i=0;i<73737;i++)assert(f.io.embeddingOriginal[i]==f.io.RawSlot(PioneerQ38EmbeddingSnapshotAddress(i)));
  positives++;
 }
 {RmsControlFixture f(argv[1],argv[2],argv[3]);unsigned touches=f.io.touches;
  f.cookie.context=nullptr;tt_q38_embedding_support support={};
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT,&support,sizeof(support))==B_OK);f.cookie.context=f.c.get();
  for(unsigned plane=0;plane<2;plane++)for(unsigned word:{0U,1023U,1024U,4095U,5119U}){
   auto r=std::make_unique<tt_q38_learned_rms_validation>();r->metadata=Request();
   (plane?r->residual:r->normalized)[word]=1;
   assert(Route(&f.cookie,TT_PCI_PROBE_Q38_RMS_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refusals++;
  }
  for(unsigned field=0;field<18;field++){
   if(field==1 || field==2)continue;
   auto r=std::make_unique<tt_q38_learned_rms_validation>();r->metadata=Request();
   reinterpret_cast<uint32*>(&r->metadata)[field]=field?1:2;
   assert(Route(&f.cookie,TT_PCI_PROBE_Q38_RMS_VALIDATE,r.get(),sizeof(*r))==B_BAD_VALUE);refusals++;
  }
  for(unsigned op:{TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,
   TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT,TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE,
   TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT,TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE,TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT,
   TT_PCI_PROBE_Q38_FFN_GRAPH_RUN,TT_PCI_PROBE_Q38_WORK_SUPPORT,TT_PCI_PROBE_Q38_WORK_RUN,
   TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,TT_PCI_PROBE_Q38_DISPATCH_RUN}){
   auto r=Request();assert(Route(&f.cookie,op,&r,sizeof(r))==B_NOT_ALLOWED);refusals++;
  }
  for(unsigned kind=0;kind<7;kind++){
   auto r=std::make_unique<tt_q38_learned_rms_validation>();r->metadata=Request();auto* cookie=&f.cookie;size_t bytes=sizeof(*r);
   if(kind==0)r->metadata.token=248320;if(kind==1)r->metadata.position=8192;
   if(kind==2)bytes--;if(kind==3)bytes++;if(kind==4)cookie=nullptr;
   if(kind==5)callerTeam=8;if(kind==6)lockStatus=B_NOT_ALLOWED;
   assert(Route(cookie,TT_PCI_PROBE_Q38_RMS_VALIDATE,r.get(),bytes)!=B_OK);callerTeam=7;lockStatus=B_OK;refusals++;
  }
  assert(!f.c->q38EmbeddingModel && f.io.touches==touches && !locked && f.c->q38GdnCallThread==-1);
 }
 for(unsigned kind=0;kind<10;kind++){
  RmsControlFixture f(argv[1],argv[2],argv[3]);
  if(kind<2)f.io.zeroFailRun=kind+1;if(kind==2)f.io.failMap=true;if(kind==3)f.io.embeddingPacketFault=22;
  if(kind==4)f.io.rmsPacketFault=8;if(kind==5)f.io.validationFailRead=1;if(kind==6)f.io.validationPoisonRead=10;
  if(kind==7)f.io.validationBadCounter=10;if(kind==8)f.io.validationSourceFault=10;if(kind==9)copyFailure=2;
  auto r=std::make_unique<tt_q38_learned_rms_validation>();r->metadata=Request();
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_RMS_VALIDATE,r.get(),sizeof(*r))==(kind==9?B_BAD_VALUE:B_OK)
   && sColdRecoveryRequired && f.c->dispatchOwnerRetained && f.io.retained && !locked && locks==releases && f.c->q38GdnCallThread==-1);
  if(kind!=9){assert(r->metadata.recovery_required && !r->metadata.closed);for(auto x:r->normalized)assert(!x);for(auto x:r->residual)assert(!x);}
  unsigned touches=f.io.touches;auto again=std::make_unique<tt_q38_learned_rms_validation>();again->metadata=Request();
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_RMS_VALIDATE,again.get(),sizeof(*again))!=B_OK
   && !EndFactory(*f.c) && f.io.touches==touches);faults++;
 }
 printf("PASS ACTUAL native RMS DEBUG control/routing/scope:positives%u/refusals%u/faults%u,actual zero/QPE1/QRN1,10poisoned pages/10240words,no Work/old namespace promotion,full restore/END,user-copy failure/first unknown retainALL/NO_LATER_IO;SIMULATED_NOT_CARD_OR_SPEED\n",positives,refusals,faults);
}
