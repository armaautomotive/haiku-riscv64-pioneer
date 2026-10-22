/* SPDX-License-Identifier: MIT */
// ACTUAL new DEBUG controller/control/factory/scope plus actual zero+QPE1.
// Numerical result copied from real simulated GDDR into POISONED local L1;
// no second compute,CPU reference or original local value can supply it.
#define PIONEER_Q38_EMBEDDING_CONTROL_FIXTURE_ONLY
#include "pioneer_q38_embedding_native_control_test.cpp"
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positives=0,faults=0,refusals=0;
 for(unsigned bank=0;bank<7;bank++){
  unsigned token=PioneerQ38EmbeddingFirst(bank)+PioneerQ38EmbeddingRows(bank)-1;
  NativeFactoryFixture f(argv[1],argv[2],token,0);ResetScope(f);
  tt_q38_embedding_support support={};unsigned touches=f.io.touches;
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT,&support,sizeof(support))==B_OK
   && support.model_namespace==11 && !support.no_cpu_reference && !support.model_inference && f.io.touches==touches);
  auto reply=std::make_unique<tt_q38_embedding_validation>();reply->metadata=Request(token);
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE,reply.get(),sizeof(*reply))==B_OK);
  const auto& r=reply->metadata;
  assert(r.state==TT_RUNTIME_PASSED && r.closed && r.input_ready && !r.recovery_required && r.session
   && r.driver_generation==sReloadGeneration && r.saved_words==73737 && r.restored_words==73737
   && f.io.validationReads==5 && f.c->q38EmbeddingModel->Closed() && !locked && locks==releases && f.c->q38GdnCallThread==-1
   && !f.io.pciWrites && !f.io.HostReadMock::bmeWrites && EndFactory(*f.c));
  for(unsigned i=0;i<5120;i++)assert(reply->values[i]==f.io.wholeWorkspace[i] && reply->values[i]==f.io.computedEmbedding[i]);
  for(unsigned i=0;i<73737;i++)assert(f.io.embeddingOriginal[i]==f.io.RawSlot(PioneerQ38EmbeddingSnapshotAddress(i)));
  positives++;
 }
 {NativeFactoryFixture f(argv[1],argv[2]);ResetScope(f);unsigned touches=f.io.touches;
  for(unsigned word:{0U,1023U,1024U,4095U,5119U}){
   auto reply=std::make_unique<tt_q38_embedding_validation>();reply->metadata=Request();reply->values[word]=1;
   assert(Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE,reply.get(),sizeof(*reply))==B_BAD_VALUE);refusals++;
  }
  for(unsigned op:{TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT,TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,
    TT_PCI_PROBE_Q38_WORK_RUN,TT_PCI_PROBE_Q38_DISPATCH_RUN}){
   auto r=Request();assert(Route(&f.cookie,op,&r,sizeof(r))==B_NOT_ALLOWED);refusals++;
  }
  assert(!f.c->q38EmbeddingModel && f.io.touches==touches);
 }
 for(unsigned fault=0;fault<7;fault++){
  NativeFactoryFixture f(argv[1],argv[2]);ResetScope(f);
  if(fault<2)f.io.validationFailRead=fault?5:1;
  if(fault==2)f.io.validationPoisonRead=5;if(fault==3)f.io.validationBadCounter=5;
  if(fault==4)f.io.validationSourceFault=5;if(fault==5)f.io.embeddingPacketFault=22;if(fault==6)copyFailure=2;
  auto reply=std::make_unique<tt_q38_embedding_validation>();reply->metadata=Request();
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE,reply.get(),sizeof(*reply))==(fault==6?B_BAD_VALUE:B_OK));
  assert(sColdRecoveryRequired && f.c->dispatchOwnerRetained && f.io.retained && !locked && f.c->q38GdnCallThread==-1);
  if(fault!=6){assert(reply->metadata.recovery_required && !reply->metadata.closed);for(auto value:reply->values)assert(!value);}
  unsigned touches=f.io.touches;auto again=std::make_unique<tt_q38_embedding_validation>();again->metadata=Request();
  assert(Route(&f.cookie,TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE,again.get(),sizeof(*again))!=B_OK && !EndFactory(*f.c) && f.io.touches==touches);faults++;
 }
 printf("PASS ACTUAL DEBUG native GDDR embedding readback:positive%u/refusals%u/faults%u,actual zero+QPE1 instructions,5fixed poisoned pages/5120bits,ALL73737 original root words restored,no BME/host DMA,full real scope/lock/retention/newENDgate,first loss NO further IO;SIMULATED_HARDWARE_NOT_CARD\n",positives,refusals,faults);
}
