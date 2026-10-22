/* SPDX-License-Identifier: MIT */
// Actual owner algorithm;simulated PCI/L1 endpoints,NO native mapping/run.
#ifdef PIONEER_Q38_TEST_QUALIFIED_LEGACY
// Exercise the SAME frozen legacy image family and Work safety features
// as the native tag32 build,in addition to the historical basic fixture.
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR
#define TT_PIONEER_Q38_WORK_MODE
#define PIONEER_Q38_FFN_CODE_DIAGNOSTIC 1
#define PIONEER_Q38_COMPACT_ACTIVATIONS
#define PIONEER_Q38_REUSE_ACTIVATIONS
#define PIONEER_Q38_REUSE_CROSS_QUANT
#define PIONEER_Q38_INT_SFPU
#define PIONEER_Q38_PIPELINED_MATRIX
#define PIONEER_Q38_STATIC_VECTOR
#define PIONEER_Q38_CACHED_AFFINE
#define PIONEER_Q38_REPLAY_OPERAND
#define PIONEER_Q38_WORK_SKIP_DOT_CHECK
#define PIONEER_Q38_WORK_COHORT_CACHE
#define PIONEER_Q38_WORK_POSTED_STAGE
#define PIONEER_Q38_WORK_OUTPUT_DMA
#define PIONEER_Q38_SPAN_FAST
#define PIONEER_Q38_QUEUED_MATRIX_ENTRY
#define PIONEER_Q38_WORK_INPUT_SUM_CACHE
#define PIONEER_Q38_WORK_ZERO_MINIMUM
#define PIONEER_Q38_NATIVE_OPERAND_STORE
#define PIONEER_Q38_WORK_DEFER_AFFINE
#define PIONEER_Q38_WORK_SPARSE_EXPORT
#define PIONEER_Q38_WORK_COMPACT_OUTPUT
#define PIONEER_Q38_ROOT_CYCLE_PROFILE
#define PIONEER_Q38_INLINE_CONFIG
#define PIONEER_Q38_BLOCK_SCALE_REUSE
#define PIONEER_Q38_SCALE_REUSE_QUALIFICATION
#define PIONEER_Q38_BLOCK_AFFINE_DST
#define PIONEER_Q38_BLOCK_AFFINE_QUALIFICATION
#define PIONEER_Q38_AFFINE_RAWDOT_QUALIFICATION
#define PIONEER_Q38_DIRECT_SRCB_QUALIFICATION
#define PIONEER_Q38_SENTINEL_POLL_QUALIFICATION
#define PIONEER_Q38_Q4_TWO_PHASE
#endif
#define PIONEER_Q38_DISPATCH_BOOTSTRAP_MOCK_ONLY
#include "pioneer_q38_dispatch_bootstrap_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphCodeOwner.h"
struct CodeIO:BootstrapIO {
 bool track=false;unsigned touches=0,observed=0,writes=0,reads=0;
 unsigned fault=0,phase=0,worker=0,index=1,aliasReads=0;bool dropped=false;
 const PioneerQ38FfnGraphCodeResult* state=nullptr;
 std::vector<uint32_t> prefix=std::vector<uint32_t>(375808);
 CodeIO(){for(unsigned i=0;i<prefix.size();i++)prefix[i]=0x9e3779b9U*(i+1)^0xa56b4e37U;}
 void Touch(){if(track){assert(!quarantined);touches++;}}
 bool At()const{return track && state && state->phase==phase && current==worker;}
 bool Word()const{return At() && state->word==index;}
 unsigned PrefixIndex(unsigned w,uint32_t a)const{return (w?10240+(w-1)*3072:0)+(a-0x64000)/4;}
 uint32_t RawRead(unsigned w,uint32_t a){assert(w==current && PioneerQ38FfnGraphProgramAddress(w,a));
  return a==0x6000?BootstrapIO::RuntimeCodeRead(0):a<0x70000?prefix.at(PrefixIndex(w,a)):BootstrapIO::Q38PanelRead(a);}
 void RawWrite(unsigned w,uint32_t a,uint32_t v){assert(w==current && PioneerQ38FfnGraphProgramAddress(w,a));
  if(a==0x6000)BootstrapIO::RuntimeCodeWrite(0,v);else if(a<0x70000)prefix.at(PrefixIndex(w,a))=v;else BootstrapIO::Q38PanelWrite(a,v);}
 uint32_t Bad(uint32_t v){observed=touches;return v^1;}
 bool SelectRuntimeCore(unsigned w){Touch();aliasReads=0;return BootstrapIO::SelectRuntimeCore(w);}
 bool FfnGraphCodeMappingStable(unsigned w){Touch();assert(w==current);if(fault==4 && At()){observed=touches;return false;}return true;}
 uint32_t FfnGraphProgramRead(unsigned w,uint32_t a){Touch();reads++;uint32_t v=RawRead(w,a);
  if(dropped){dropped=false;observed=touches;return v;}
  return fault==1 && Word()?Bad(v):v;}
 bool FfnGraphProgramWrite(unsigned w,uint32_t a,uint32_t v){Touch();writes++;
  assert(state && state->savedWords==867448 && (state->phase==2 || state->phase==4));
  if(fault==2 && Word()){observed=touches;return false;}
  if(fault==3 && Word()){assert(RawRead(w,a)!=v);dropped=true;return true;}
  RawWrite(w,a,v);return true;}
 uint32_t Q38PanelRead(uint32_t a){Touch();return BootstrapIO::Q38PanelRead(a);}
 bool Q38PanelMappingStable(){Touch();return BootstrapIO::Q38PanelMappingStable();}
 uint32_t RuntimeCodeRead(unsigned i){Touch();uint32_t v=BootstrapIO::RuntimeCodeRead(i);return fault==5 && At()?Bad(v):v;}
 uint32_t ResetRead(){Touch();uint32_t v=BootstrapIO::ResetRead();return fault==6 && At()?Bad(v):v;}
 uint32_t SourceRead(unsigned i){Touch();uint32_t v=BootstrapIO::SourceRead(i);return fault==7 && At()?Bad(v):v;}
 uint32_t ResidentNocRead(uint32_t o){Touch();uint32_t v=BootstrapIO::ResidentNocRead(o);
  if(At() && ((fault==8 && o==0x100) || (fault==10 && o==0x208) || (fault==12 && !o) || (fault==11 && o==0x210 && ++aliasReads==2)))return Bad(v);
  return v;}
 uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t b){Touch();uint32_t v=BootstrapIO::HostPciRead(p,o,b);return fault==9 && At() && o==4?Bad(v):v;}
 void ResidentNocWrite(uint32_t o,uint32_t v){assert(!track && "NO NoC commands allowed in code owner");BootstrapIO::ResidentNocWrite(o,v);}
 void RuntimeResetWrite(uint32_t v){assert(!track && "NO reset/start/hold in code owner");BootstrapIO::RuntimeResetWrite(v);}
 void QuarantineHostPage(){quarantined=true;}
};
struct CodeFixture {
 CodeIO io;std::unique_ptr<PioneerQ38DispatchLease> cohort{new PioneerQ38DispatchLease};
 std::unique_ptr<PioneerQ38FfnGraphCodeOwner> owner{new PioneerQ38FfnGraphCodeOwner};
 std::vector<std::unique_ptr<PanelOwners>> owners;tt_dram_samples_result dram;PioneerResidentBanks banks;
 PioneerQ38FfnGraphCodeResult result;
 CodeFixture(){dram=BankProof(*io.core[0]);assert(banks.Bind(dram));
  for(unsigned w=0;w<120;w++){owners.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&owners[w]->store;
   n.noc=owners[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*cohort,w));}
  io.state=&result;io.track=true;
 }
 bool Run(){return PioneerQ38FfnGraphCodeRoundTrip(io,dram,banks,*cohort,*owner,result);}
 void Unknown(){assert(result.started && result.unsafe && owner->Unsafe() && !owner->Closed()
  && cohort->unsafe && banks.Quarantined() && io.quarantined && !io.armed && !io.released);
  assert(io.observed && io.observed==io.touches);unsigned touches=io.touches,writes=io.writes;
  assert(!Run() && touches==io.touches && writes==io.writes);
  for(auto& n:cohort->worker)assert(n.image.unsafe && n.Store().recovery_required);
 }
};
#ifndef PIONEER_Q38_FFN_CODE_OWNER_MOCK_ONLY
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned geometry=0,faults=0;
 for(unsigned w=0;w<120;w++){
  unsigned words=PioneerQ38FfnGraphCodeWords(w),prefix=w?3072:10240;
  assert(words==prefix+4097 && PioneerQ38FfnGraphCodeBase(w)+words<=PioneerQ38FfnGraphCodeOwner::Words);
  for(unsigned i=0;i<words;i++){uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i);
   assert(PioneerQ38FfnGraphProgramAddress(w,a) && a==(i?i<=prefix?0x64000+(i-1)*4:0x70000+(i-prefix-1)*4:0x6000));geometry++;}
  assert(PioneerQ38FfnGraphCodeAddress(w,words)==UINT32_MAX && PioneerQ38FfnGraphCodeAddress(w,UINT32_MAX)==UINT32_MAX);
 }
 assert(geometry==PioneerQ38FfnGraphCodeOwner::Words && !PioneerQ38FfnGraphCodeWords(120)
  && PioneerQ38FfnGraphCodeBase(120)==UINT32_MAX && PioneerQ38FfnGraphCodeAddress(120,0)==UINT32_MAX);
 for(bool wrap:{false,true}){CodeFixture f;auto before=f.io.prefix;std::vector<uint32_t> program;
  if(wrap)for(unsigned w=0;w<120;w++){auto& c=*f.io.core[w];c.counters[0]=0xfffffffeU-w;
   c.counters[1]=0xfffff001U+w;c.counters[3]=17+w;c.accepted=0xffffff00U+w;}
  f.io.track=false;for(unsigned w=0;w<120;w++){assert(f.io.SelectRuntimeCore(w));for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++)program.push_back(f.io.RawRead(w,PioneerQ38FfnGraphCodeAddress(w,i)));}
  f.io.track=true;assert(f.Run() && f.result.passed && f.owner->Closed() && !f.owner->Unsafe());
  assert(f.result.savedWords==geometry && f.result.installedWords==geometry && f.result.restoredWords==geometry && f.io.writes==2*geometry);
  assert(f.io.prefix==before && !f.io.armed && !f.io.released);
  unsigned touches=f.io.touches;assert(!f.Run() && touches==f.io.touches);
  f.io.track=false;for(unsigned w=0;w<120;w++){assert(f.io.SelectRuntimeCore(w));for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++)assert(f.io.RawRead(w,PioneerQ38FfnGraphCodeAddress(w,i))==program[PioneerQ38FfnGraphCodeBase(w)+i]);}
  assert(PioneerQ38DispatchBootstrap(f.io,f.dram,f.banks,*f.cohort) && f.io.armed==119 && f.io.released==119);
 }
 for(unsigned phase:{2U,3U,4U,5U})for(unsigned worker:{0U,60U,119U})for(unsigned point:{0U,1U,2U}){
  CodeFixture f;f.io.fault=1;f.io.phase=phase;f.io.worker=worker;
  f.io.index=point==0?0:point==1?PioneerQ38FfnGraphPrefixBytes(worker)/4:PioneerQ38FfnGraphCodeWords(worker)-1;
  assert(!f.Run());f.Unknown();faults++;
 }
 for(unsigned phase:{2U,4U})for(unsigned kind:{2U,3U})for(unsigned worker:{0U,60U,119U}){
  CodeFixture f;f.io.fault=kind;f.io.phase=phase;f.io.worker=worker;f.io.index=1;
  assert(!f.Run());f.Unknown();faults++;
 }
 for(unsigned phase:{1U,2U,3U,4U,5U})for(unsigned kind:{4U,6U,7U,8U,9U}){
  CodeFixture f;f.io.fault=kind;f.io.phase=phase;f.io.worker=60;
  assert(!f.Run());f.Unknown();faults++;
 }
 {CodeFixture f;f.io.fault=5;f.io.phase=1;f.io.worker=60;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned phase:{2U,3U,4U,5U})for(unsigned kind:{10U,12U})for(unsigned worker:{0U,60U,119U}){
  CodeFixture f;f.io.fault=kind;f.io.phase=phase;f.io.worker=worker;assert(!f.Run());f.Unknown();faults++;
 }
 for(unsigned worker:{0U,60U,119U}){CodeFixture f;f.io.fault=11;f.io.phase=1;f.io.worker=worker;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned bad=0;bad<17;bad++){CodeFixture f;
  if(bad==0)f.cohort->booted=true;if(bad==1)f.cohort->bootAttempted=true;if(bad==2)f.cohort->unsafe=true;
  if(bad==3)f.cohort->rootRunning=true;if(bad==4)f.cohort->prepared=119;if(bad==5)f.cohort->closed=true;
  if(bad==6)f.banks.Quarantine();if(bad==7)f.result.version=2;
  if(bad==8)f.result.phase=1;if(bad==9)f.result.worker=1;if(bad==10)f.result.word=1;
  if(bad==11)f.result.savedWords=1;if(bad==12)f.result.installedWords=1;if(bad==13)f.result.restoredWords=1;
  if(bad==14)f.result.started=1;if(bad==15)f.result.passed=1;if(bad==16)f.result.unsafe=1;
  auto result=f.result;assert(!f.Run() && !f.io.touches && !f.io.writes && !memcmp(&result,&f.result,sizeof(result)));faults++;
 }
 printf("PASS OFFLINE complete FFN code owner:positive2/ALL120/words%u/exact install+restore/zero+nonzero+wrapped master baselines/legacy bootstrap after restore/faults%u/first observed mismatch no further IO/whole-owner quarantine/no NoC/start/reset/stack touches;SIMULATED_ENDPOINTS_NOT_NATIVE_GRANT_OR_SPEED\n",geometry,faults);
}
#endif
