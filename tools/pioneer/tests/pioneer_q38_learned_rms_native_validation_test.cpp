/* SPDX-License-Identifier: MIT */
// Actual native state/source/controller/factory + emitted zero/QPE1/QRN1.
// Only hardware endpoints are simulated; numerical vectors are never inputs
// to the native controller. This is NOT physical-card qualification.
#define PIONEER_Q38_RMS_FACTORY_FIXTURE_ONLY
#include "pioneer_q38_learned_rms_native_factory_test.cpp"
struct RmsValidationFixture:NativeFactoryFixture {
 PioneerQ38LearnedRmsResult norm;
 std::array<uint32,5120> normalized={},residual={};
 RmsValidationFixture(const char* zero,const char* embedding,const char* rms,unsigned token=0,unsigned bank=0)
  :NativeFactoryFixture(zero,embedding,token,bank){
  io.rmsImage=rms;auto p=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(p->Build());bool found=false;
  for(unsigned i=0;i<p->Count();i++){
   const auto* part=p->Part(i);
   if(part->kind!=p->WeightKind || part->layer || part->role)continue;
   assert(!found && part->bytes==20480);found=true;
   unsigned first=part->bank*unsigned(p->Capacity/4096)+part->offset/4096;
   for(unsigned page=0;page<5;page++)learned[first+page]=1;
  }assert(found);
 }
 bool Normalize(){return RunQ38LearnedRmsModel(*c,norm,normalized.data(),residual.data());}
 void Unknown(){
  auto* m=c->q38EmbeddingModel;assert(m && m->rms.Unsafe() && !m->rms.Completed());
  NativeFactoryFixture::Unknown();unsigned n=io.touches;
  assert(!Normalize() && !EndFactory(*c) && io.touches==n);
 }
};
#ifndef PIONEER_Q38_RMS_VALIDATION_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positives=0,faults=0,refusals=0;
 for(unsigned source=0;source<7;source++){
  RmsValidationFixture f(argv[1],argv[2],argv[3],PioneerQ38EmbeddingFirst(source)+PioneerQ38EmbeddingRows(source)-1,(source+3)%7);
  assert(f.Bind() && f.Initialize() && f.Run());auto raw=f.io.wholeWorkspace;
  std::vector<uint32> original(73737);for(unsigned i=0;i<73737;i++)original[i]=f.io.RawSlot(PioneerQ38EmbeddingSnapshotAddress(i));
  assert(f.Normalize() && f.c->q38EmbeddingModel->Closed() && f.norm.validationPages==10 && f.norm.validationWords==10240
   && f.io.validationReads==10 && f.io.embeddingStarts==1 && f.io.rmsStarts==1 && f.io.rmsStops==1
   && f.norm.savedWords==73737 && f.norm.restoredWords==73737 && f.norm.verifiedWords==147474);
  for(unsigned i=0;i<5120;i++){
   assert(f.normalized[i]==f.io.wholeWorkspace[i] && f.residual[i]==raw[i]
    && f.residual[i]==f.io.wholeWorkspace[TT_Q38_RMS_BACKUP/4+i]);
  }
  for(unsigned i=0;i<73737;i++)assert(original[i]==f.io.RawSlot(PioneerQ38EmbeddingSnapshotAddress(i)));
  assert(!f.io.pciWrites && !f.io.HostReadMock::bmeWrites && EndFactory(*f.c));
  unsigned n=f.io.touches;assert(!f.Normalize() && f.io.touches==n);refusals++;
  printf("PASS actual native QRN1 DEBUG fixed normalized+raw readback source%u/workspace%u,10poisoned pages/10240words,full ROOT restore/END\n",source,(source+3)%7);positives++;
 }
 // Invalid internal output storage must be refused BEFORE claim/card I/O.
 for(unsigned kind=0;kind<7;kind++){
  RmsValidationFixture f(argv[1],argv[2],argv[3]);assert(f.Bind() && f.Initialize() && f.Run());auto& m=*f.c->q38EmbeddingModel;
  auto* a=f.normalized.data();auto* b=f.residual.data();
  if(kind==0)a=nullptr;if(kind==1)b=nullptr;if(kind==2)b=a;if(kind==3)b=a+1;
  if(kind==4)a=reinterpret_cast<uint32*>(uintptr_t(1));
  if(kind==5)a=reinterpret_cast<uint32*>(UINTPTR_MAX-3);
  if(kind==6)f.norm.validationWords=1;
  unsigned n=f.io.touches;
  assert(!m.rms.RunOnce(f.io,f.owner.dram,f.owner.noc,f.owner.banks,m.graph,m.plan,m.embedding,m.identity,
   f.learned.data(),PioneerResidentBanks::Pages,f.owner.store,m.token,m.position,m.bank,f.norm,a,b)
   && !m.rms.Claimed() && f.io.touches==n);refusals++;
 }
 for(unsigned kind=0;kind<4;kind++)for(unsigned page=1;page<=10;page++){
  RmsValidationFixture f(argv[1],argv[2],argv[3]);assert(f.Bind() && f.Initialize() && f.Run());
  if(kind==0)f.io.validationFailRead=page;if(kind==1)f.io.validationPoisonRead=page;
  if(kind==2)f.io.validationBadCounter=page;if(kind==3)f.io.validationSourceFault=page;
  assert(!f.Normalize() && f.io.validationReads==page && f.norm.unsafe && !f.norm.closed && !f.norm.completed
   && !f.norm.restoredWords);f.Unknown();faults++;
 }
 printf("PASS actual native QRN1 DEBUG readback positives%u/refusals%u/faults%u;exact same zero/QPE1/QRN1 owner/scope/physical hold,10fixed poisoned pages,first unknown MEMORY-ONLY retention/no subsequent IO/END;SIMULATED_HARDWARE_NOT_CARD_OR_SPEED\n",positives,refusals,faults);
}
#endif
