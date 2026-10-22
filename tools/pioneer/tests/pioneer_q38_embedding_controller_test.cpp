/* SPDX-License-Identifier: MIT */
// Actual private controller+ACTUAL QPE1 instruction VM in a separate TU.
// Actual frozen zero ROM publishes workspace first. Endpoints simulated.
#define PIONEER_Q38_PROJECTION_ZERO_FIXTURE_ONLY
#include "pioneer_q38_projection_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingController.h"
#include "pioneer_q38_embedding_execution_bridge.h"
template<class T,class=void>struct HasEmbeddingPublish:std::false_type{};
template<class T>struct HasEmbeddingPublish<T,std::void_t<decltype(&T::PublishEmbedding)>>:std::true_type{};
static_assert(!HasEmbeddingPublish<PioneerQ38ProjectionBanks>::value,"no public producer publication");
static_assert(!std::is_copy_constructible<PioneerQ38EmbeddingController>::value && !std::is_move_constructible<PioneerQ38EmbeddingController>::value,"opaque retained execution owner");
struct EmbeddingControllerIO {
 ProjectionZeroFixture& f;EmbeddingExecutionBridge device;
 const char* image;const PioneerQ38EmbeddingController* expected=nullptr;PioneerQ38EmbeddingResult* result=nullptr;
 unsigned touches=0,reads=0,writes=0,starts=0,holds=0,restores=0;
 unsigned summaryFault=UINT32_MAX,descriptorFault=UINT32_MAX,commandFault=UINT32_MAX,counterFault=UINT32_MAX;
 unsigned sourceFault=UINT32_MAX,badPhase=0,badWord=0,badKind=0;
 bool scope=true,others=true,mapping=true,retained=false,noCompletion=false,corruptCode=false;
 bool badPc=false,badCount=false,badMarker=false,backwards=false,frozen=false;
 uint64_t clock=100000;uint32_t pc=0;
 static unsigned Index(uint32_t a){unsigned n=0;for(unsigned i=0;i<6;i++){auto s=PioneerQ38EmbeddingSnapshotSpan(i);
  if(a>=s.address && a-s.address<s.words*4)return n+(a-s.address)/4;n+=s.words;}assert(false);return 0;}
 EmbeddingControllerIO(ProjectionZeroFixture& source,const char* program):f(source),image(program){
  device.l1.resize(73737);device.resident.fill(0xdeadbeef);
  for(unsigned i=0;i<73737;i++)device.l1[i]=i%17?0x9e3779b9U*(i+1):UINT32_MAX;
  for(unsigned i=0;i<16;i++)device.source[i]=i?f.owner.noc.source[0][i]:f.owner.store.config|0x10000;
  for(unsigned i=0;i<10;i++)device.command[i]=0xa5000000+i;
  for(unsigned i=0;i<8;i++)device.counts[i]=0xfffffff8;
 }
 void Touch(){assert(!retained);touches++;}
 bool AtFault()const{return result && result->phase==badPhase && result->word==badWord;}
 bool EmbeddingNativeScope(const PioneerQ38EmbeddingController& controller,const PioneerQ38ProjectionMutableBanks& graph,
  const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38ProjectionIdentity& id,const uint8_t* ledger,unsigned pages){
  return scope && expected==&controller && &graph==f.graph.get() && &plan==f.plan.get() && id==f.id
   && ledger==f.learned.data() && pages==f.learned.size();
 }
 bool EmbeddingOtherCoresHeld(){Touch();return others;}
 bool EmbeddingMemoryMappingStable(){Touch();return mapping;}
 bool SelectRuntimeCore(unsigned w){Touch();return !w;}
 uint32_t ResetRead(){Touch();return f.io.coreReset;}
 uint32_t SourceRead(unsigned i){Touch();return f.io.SourceRead(i);}
 uint32_t HostPciRead(unsigned p,uint16_t a,uint8_t bytes){Touch();return f.io.HostPciRead(p,a,bytes);}
 uint32_t ResidentNocRead(uint32_t a){
  Touch();if(a<40)return device.command[a/4];
  for(unsigned i=0;i<16;i++)if(a==BlackholeDramSourceOffset(i))return device.source[i];
  const unsigned c[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  for(unsigned i=0;i<8;i++)if(a==c[i])return device.counts[i];
  for(unsigned i=0;i<4;i++)if(a==0x40+i*0x800)return 0;
  if(a>=0x240 && a<0x2c0)return 0;assert(false);return 0;
 }
 void ResidentNocWrite(uint32_t a,uint32_t v){Touch();assert(a<40 && !(a&3) && result && result->phase==6);device.command[a/4]=v;}
 bool EmbeddingReadChecked(uint32_t a,uint32_t& value){
  Touch();reads++;assert(PioneerQ38EmbeddingSnapshotContains(a));
  if(AtFault() && badKind==1)return false;
  value=device.l1.at(Index(a));if(AtFault() && badKind==2)value^=1;return true;
 }
 bool EmbeddingWrite(uint32_t a,uint32_t value){
  Touch();writes++;assert(result && result->savedWords==73737 && result->verifiedWords>=73737 && PioneerQ38EmbeddingSnapshotContains(a));
  if(AtFault() && badKind==3)return false;
  if(!(AtFault() && badKind==4))device.l1.at(Index(a))=value;
  if(result->phase==6)restores++;return true;
 }
 void RuntimeStatusWrite(unsigned i,uint32_t value){Touch();assert(i<3);device.l1.at(Index(0x17ffc0+i*4))=value;}
 uint32_t RuntimeStatusRead(unsigned i){Touch();assert(i<3);return device.l1.at(Index(0x17ffc0+i*4));}
 uint32_t RuntimePcRead(){Touch();return badPc?0x64040:pc;}
 void Pause(){Touch();if(!frozen)clock+=100;}
 uint64_t Q38CohortNow(){return starts && backwards?0:clock;}
 void RuntimeResetWrite(uint32_t value){
  Touch();if(value==0x87ffff){if(f.io.coreReset==0x46800)holds++;f.io.coreReset=value;return;}
  if(value==0x47800){f.io.coreReset=value;return;}
  assert(value==0x46800 && f.io.coreReset==0x87ffff);f.io.coreReset=value;starts++;
  if(noCompletion){pc=0x64000;return;}
  ExecuteEmbeddingBridge(image,device);pc=device.pc;
  if(summaryFault<32)device.l1[Index(TT_Q38_FFN_RESIDENT_SUMMARY+summaryFault*4)]^=1;
  if(descriptorFault<64)device.l1[Index(TT_Q38_FFN_RESIDENT_DESCRIPTOR+descriptorFault*4)]^=1;
  if(commandFault<10)device.command[commandFault]^=1;
  if(counterFault<8){device.counts[counterFault]++;if(counterFault==2 || counterFault==6)device.counts[6]=device.counts[2];}
  if(sourceFault<16)device.source[sourceFault]^=1;
  if(corruptCode)device.l1[Index(0x64080)]^=1;
  if(badMarker)device.l1[Index(0x17ffc8)]^=1;
  if(badCount)device.l1[Index(0x17ffc4)]^=1;
 }
 void QuarantineHostPage(){f.io.QuarantineHostPage();}
 void EmbeddingRetainHostOwners(){retained=true;f.transfer.quarantined=true;f.wide.quarantined=true;}
};
struct EmbeddingControllerFixture {
 ProjectionZeroFixture f;EmbeddingControllerIO io;
 std::unique_ptr<PioneerQ38EmbeddingController> controller=std::make_unique<PioneerQ38EmbeddingController>();
 PioneerQ38EmbeddingResult result;
 unsigned token,position,bank;
 EmbeddingControllerFixture(const char* zero,const char* embedding,unsigned selectedToken=0,unsigned destination=0)
  :f(zero,destination),io(f,embedding),token(selectedToken),position(8191),bank(destination){
  assert(f.PrepareZero() && f.Zero(0,256) && f.Zero(256,256) && f.CloseZero() && f.state->InitializationPublished(f.id));
  auto* p=f.plan->ProjectionStripe(64,0,token>=248320?0:token/35520);(void)p;
  if(token<248320)for(unsigned stripe=0;stripe<7;stripe++){
   p=f.plan->ProjectionStripe(64,0,stripe);if(token<p->firstRow || token-p->firstRow>=p->rows)continue;
   unsigned offset=p->offset+(token-p->firstRow)/16*46080,first=p->bank*unsigned(f.plan->Capacity/4096)+offset/4096;
   unsigned pages=((offset%4096)+46080+4095)/4096;for(unsigned i=0;i<pages;i++)f.learned[first+i]=1;
  }
  io.expected=controller.get();io.result=&result;
 }
 bool Run(){return controller->RunOnce(io,f.owner.dram,f.owner.noc,f.owner.banks,*f.graph,*f.plan,f.id,f.learned.data(),unsigned(f.learned.size()),f.owner.store,token,position,bank,result);}
 void Unknown(){
  assert(controller->Unsafe() && !controller->Completed() && !controller->Closed() && f.graph->Unsafe() && f.owner.banks.Quarantined()
   && f.owner.store.recovery_required && io.retained && f.transfer.quarantined && f.wide.quarantined);
  unsigned touches=io.touches;assert(!Run() && io.touches==touches);
 }
};
#ifndef PIONEER_Q38_EMBEDDING_CONTROLLER_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || argc==4);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positive=0,faults=0;
 if(argc==3){
  for(unsigned bank=0;bank<7;bank++){
   EmbeddingControllerFixture f(argv[1],argv[2],PioneerQ38EmbeddingFirst(bank)+PioneerQ38EmbeddingRows(bank)-1,(bank+3)%7);
   auto original=f.io.device.l1;uint32_t command[10];memcpy(command,f.io.device.command,sizeof(command));
   bool dirty=f.f.owner.store.l1_dirty,restored=f.f.owner.store.l1_restored;
   assert(f.Run() && f.controller->Completed() && f.controller->Closed() && !f.controller->Unsafe() && f.result.completed && f.result.held && f.result.closed);
   assert(f.result.savedWords==73737 && f.result.verifiedWords==147474 && f.result.installedWords==10241 && f.result.restoredWords==73737
    && f.io.device.l1==original && !memcmp(command,f.io.device.command,sizeof(command)) && f.io.starts==1 && f.io.holds==1);
   assert(f.f.owner.store.l1_dirty==dirty && f.f.owner.store.l1_restored==restored && f.f.io.coreReset==0x47800);
   unsigned touches=f.io.touches;assert(!f.Run() && f.io.touches==touches);positive++;
   auto forged=std::make_unique<PioneerQ38EmbeddingController>();PioneerQ38EmbeddingResult forgedResult;
   assert(!forged->RunOnce(f.io,f.f.owner.dram,f.f.owner.noc,f.f.owner.banks,*f.f.graph,*f.f.plan,f.f.id,
    f.f.learned.data(),unsigned(f.f.learned.size()),f.f.owner.store,f.token,f.position,f.bank,forgedResult)
    && !forged->Completed() && !forgedResult.started && f.io.touches==touches);
   printf("PASS ACTUAL zero2MiB->QPE1 controller sourcebank%u/crossbank%u/last rowlane15,73737 saved/restored,opaque publish ONLY after known-clean physical hold+restore,NO learned result CPU read\n",bank,(bank+3)%7);
  }
 }else{
  for(unsigned word=0;word<32;word++){
   EmbeddingControllerFixture f(argv[1],argv[2]);f.io.summaryFault=word;assert(!f.Run() && !f.io.holds && !f.io.restores);f.Unknown();faults++;
  }
  for(unsigned kind=0;kind<14;kind++){
   EmbeddingControllerFixture f(argv[1],argv[2]);
   if(kind==0)f.io.descriptorFault=40;if(kind==1)f.io.commandFault=9;if(kind==2)f.io.counterFault=0;
   if(kind==3)f.io.sourceFault=15;if(kind==4)f.io.corruptCode=true;if(kind==5)f.io.badMarker=true;
   if(kind==6)f.io.badCount=true;if(kind==7)f.io.badPc=true;if(kind==8)f.io.backwards=true;
   if(kind==9)f.io.device.failPacket=22;if(kind==10)f.io.scope=false;if(kind==11)f.io.others=false;
   if(kind==12)f.io.noCompletion=true;if(kind==13){f.io.noCompletion=true;f.io.frozen=true;}
   assert(!f.Run() && !f.io.holds && !f.io.restores);f.Unknown();faults++;
  }
  for(unsigned phase:{1U,2U,3U,6U,7U}){
   EmbeddingControllerFixture f(argv[1],argv[2]);f.io.badPhase=phase;f.io.badWord=phase==3?10240:73736;f.io.badKind=phase==3 || phase==6?3:1;
   assert(!f.Run());if(phase<6)assert(!f.io.holds);f.Unknown();faults++;
  }
  {EmbeddingControllerFixture f(argv[1],argv[2]);unsigned before=f.io.touches;
   f.token=248320;assert(!f.Run() && !f.result.started && f.io.touches==before && !f.controller->Unsafe());}
  {EmbeddingControllerFixture f(argv[1],argv[2]);unsigned before=f.io.touches;
   f.f.learned[0]=0;assert(!f.Run() && !f.result.started && f.io.touches==before && !f.controller->Unsafe());}
  {EmbeddingControllerFixture f(argv[1],argv[2]);unsigned before=f.io.touches;
   auto fake=f.f.learned;
   assert(!f.controller->RunOnce(f.io,f.f.owner.dram,f.f.owner.noc,f.f.owner.banks,*f.f.graph,*f.f.plan,f.f.id,
    fake.data(),unsigned(fake.size()),f.f.owner.store,f.token,f.position,f.bank,f.result)
    && !f.result.started && f.io.touches==before && !f.controller->Unsafe());}
 }
 printf("PASS QPE1 execution controller positives%u/faults%u;ACTUAL frozen zero+embedding RV32,complete root snapshot/second pass/physical proof+hold/exact restore BEFORE private publication,UNKNOWN retainsALL ZERO later IO;SIMULATED endpoints,NO_NATIVE_SELECTOR_OR_MODEL_SPEED\n",positive,faults);
}
#endif
