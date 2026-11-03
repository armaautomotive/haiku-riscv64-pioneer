/* SPDX-License-Identifier: MIT */
// Actual NEW Native.inc methods against explicit KernelExport/PCI endpoints.
// Scope/ready/closed are fixture setup ONLY,not native prerequisite/completion
// proof. No engine/ARC run;coupled instructions are a separate owner test.
#define PIONEER_DRAM_SWEEP_NATIVE_LIFECYCLE_COMPONENT
#include "pioneer_dram_sweep_native_lifecycle_test.cpp"
#undef PIONEER_DRIVER_BUILD_TAG
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x4452434e4f445256)
#define PIONEER_DRAM_COHORT_NATIVE_CONTEXT 1
#define PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT 1
#define PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY 1
#define TT_PIONEER_DRIVER_RELOAD
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
using bigtime_t=int64_t;
static bigtime_t clockEndpoint=1234567;
static bigtime_t system_time(){Touch();return clockEndpoint;}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDramCohortNative.inc"
struct CohortFixture {
 PioneerProbeCookie fd;
 PioneerDramCohortNativeContext* c;
 CohortFixture(){
  assert(views.empty() && !sDramSweepNativeContext && !sDramCohortNativeContext);
  ioCalls=deletes=dropDelete=unreserves=pciWrites=dropPciWrite=pciReads=badPciRead=0;
  currentThread=10;currentTeam=20;semCount=0;sColdRecoveryRequired=sReloadQuiesced=sBankStorageOnly=false;
  sOpenCookies=1;sReloadGeneration=7;sReloadGuard=&guard;sPCI=&pci;sRuntimeOwner=NULL;clockEndpoint=1234567;
  c=new PioneerDramCohortNativeContext(&fd,thread_info{},16);sDramSweepNativeContext=c;sDramCohortNativeContext=c;
  c->backing=(uint32*)calloc(c->AllocationWords,4);c->exactBacking=c->backing;c->reserved=true;
  c->profile.root_bus=0;c->profile.bar0_host_start=UINT64_C(0x4900000000);
  assert(c->io.Map() && views.size()==17);c->ready=c->cohortReady=true;assert(c->CohortScope());
 }
 void Select(unsigned r){
  for(unsigned i=0;i<3;i++){
   c->io.DramTlbWrite(0,i,BlackholeDramFixedTlbWord(0,PioneerDramCohortX(r),2,false,i));
   c->io.DramTlbWrite(1,i,BlackholeDramFixedTlbWord(0xffa00000,PioneerDramCohortX(r),2,false,i));
  }
 }
 ~CohortFixture(){
  // Fixture teardown only,NEVER production recovery of quarantined resources.
  for(const auto& v:views)free(v.second.address);views.clear();
  delete c;sDramSweepNativeContext=NULL;sDramCohortNativeContext=NULL;sColdRecoveryRequired=false;
 }
};
int main(){
 unsigned good=0,bad=0;
 assert(PioneerDramSweepControl(NULL,UINT32_MAX,NULL,0)==B_NOT_ALLOWED && !ioCalls && !copyCalls);bad++;
 {CohortFixture f;auto& c=*f.c;auto& io=c.cohortIO;
  assert(io.CohortNativeScope(&c.cohort,&c.dram,c.exactBacking,c.exactBacking+BlackholeDramCohortReadSweepImage::CohortSavedWords,
   BlackholeDramCohortReadSweepImage::CohortSavedWords,65536));
  assert(!io.CohortNativeScope(&c.cohort,&c.dram,c.exactBacking,c.exactBacking,493225,65536));bad++;
  for(unsigned r=0;r<8;r++){
   f.Select(r);uint32_t v=0;
   for(unsigned a:{0x6000U,0x64000U,0x6e000U,0x6fffcU,0x74000U,0x7407cU,0x74100U,0x7417cU,0x74200U,0x74220U,0x74240U}){
    assert(io.CohortWrite(r,a,0x12345678) && io.CohortRead(r,a,v) && v==0x12345678);good++;
   }
   for(unsigned a:{0x70000U,0x74080U,0x74180U,0x74204U,0x74480U,0x74880U,0xd0000U}){
    const unsigned before=ioCalls;v=0xfeedabba;
    assert(!io.CohortRead(r,a,v) && !io.CohortWrite(r,a,1) && v==0xfeedabba && ioCalls==before);bad++;
   }
   const unsigned allowed=r?0x90000:0x74500,forbidden=r?0x74500:0x90000;
   assert(io.CohortWrite(r,allowed,r) && io.CohortRead(r,allowed,v) && v==r);good++;
   assert(!io.CohortRead(r,forbidden,v));bad++;
  }
  uint64 now=0;assert(io.CohortHostTime(now) && now==1234567);good++;
  clockEndpoint=-1;assert(!io.CohortHostTime(now));bad++;
 }
 for(unsigned r=0;r<8;r++){
  CohortFixture f;auto& c=*f.c;auto& io=c.cohortIO;f.Select(r);
  c.result.sweep.phase=4;io.CohortResetWrite(r,0x87ffff);assert(c.resetStages[r]==1 && *c.io.resetWrite==0x87ffff);
  c.result.sweep.phase=5;c.result.sweep.launched=1U<<r;io.CohortResetWrite(r,0x46800);
  assert(c.resetStages[r]==2 && *c.io.resetWrite==0x46800);
  assert(!io.CohortWrite(r,0x74000,1));bad++;
  c.result.sweep.phase=6;c.result.sweep.observed=255;io.CohortResetWrite(r,0x47800);
  assert(c.resetStages[r]==3 && *c.io.resetWrite==0x47800);good++;
  io.CohortResetWrite(r,0x47800);assert(c.quarantined && c.result.retained);bad++;
  const unsigned before=ioCalls;
  assert(io.CohortPcRead(r)==UINT32_MAX && !io.CohortWrite(r,0x74000,1) && ioCalls==before);bad++;
 }
 for(unsigned fault=0;fault<10;fault++){
  CohortFixture f;auto& c=*f.c;
  if(fault==0)f.fd.magic=0;
  if(fault==1)currentThread++;
  if(fault==2)currentTeam++;
  if(fault==3)sReloadGeneration++;
  if(fault==4)c.exactBacking++;
  if(fault==5)c.cohortReady=false;
  if(fault==6)c.reply.sweep.started=1;
  if(fault==7)c.resetStage=1;
  if(fault==8)sDramCohortNativeContext=NULL;
  if(fault==9)c.clockCall=true;
  const unsigned before=ioCalls;uint64 now=0;
  assert(!c.cohortIO.CohortHostTime(now) && c.quarantined && ioCalls==before);bad++;
 }
 {CohortFixture f;auto& c=*f.c;c.result.sweep.closed=1;c.transaction.config_attempted=1;
  assert(c.CloseCohortResources() && c.result.viewsClosed && c.reply.viewsClosed
   && c.result.pciClosed && views.empty() && deletes==17 && pciWrites==11);good++;
 }
 for(unsigned fail:{1U,8U,17U}){
  CohortFixture f;auto& c=*f.c;c.result.sweep.closed=1;c.transaction.config_attempted=1;dropDelete=fail;
  assert(!c.CloseCohortResources() && c.quarantined && deletes==fail && !pciWrites);bad++;
  const unsigned before=ioCalls;assert(!c.CloseCohortResources() && ioCalls==before);bad++;
 }
 printf("PASS_COHORT_ACTUAL_NATIVE_ENDPOINTS valid%u refused%u all8bounds1 exactBacking1 realHostClockCall1 oneAttemptResetLedger1 forwardedLeaseClosure1 noIOAfterRetention1;KERNEL_EXPORT_PCI_FIXTURES_NOT_NATIVE_PREREQUISITES_ENGINE_ARC_HARDWARE_BANDWIDTH_OR_WORK\n",good,bad);
}
