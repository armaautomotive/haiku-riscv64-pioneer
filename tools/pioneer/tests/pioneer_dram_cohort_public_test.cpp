/* SPDX-License-Identifier: MIT */
// Actual public/native refusal edges with KernelExport/PCI fixtures. Successful
// receipt grammar below is SYNTHETIC DATA,not engine/prerequisite/ARC execution.
#define PIONEER_DRAM_SWEEP_NATIVE_LIFECYCLE_COMPONENT
#include "pioneer_dram_sweep_native_lifecycle_test.cpp"
#undef PIONEER_DRIVER_BUILD_TAG
#ifdef COHORT_DIRECT_ENDPOINT_TEST
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026110300020077)
#define PIONEER_DRAM_COHORT_DIRECT_TLB 1
#else
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026110300010077)
#endif
#define PIONEER_DRAM_COHORT_NATIVE_CONTEXT 1
#define PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT 1
#define PIONEER_DRAM_COHORT_DIAGNOSTIC 1
#define TT_PIONEER_DRIVER_RELOAD
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
using bigtime_t=int64_t;
static bigtime_t system_time(){Touch();return 1234567;}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDramCohortNative.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDramCohortControl.inc"
static void Reset(){
 assert(views.empty() && !sDramSweepNativeContext && !sDramCohortNativeContext);
 ioCalls=deletes=dropDelete=unreserves=pciWrites=dropPciWrite=pciReads=badPciRead=0;
 effectiveUid=copyCalls=failCopy=0;failProfile=failUnreserve=false;
 currentThread=10;currentTeam=20;semCount=0;sColdRecoveryRequired=sReloadQuiesced=sBankStorageOnly=false;
 sOpenCookies=1;sReloadGeneration=7;sReloadGuard=&guard;sPCI=&pci;sRuntimeOwner=NULL;
}
static tt_pioneer_dram_cohort Request(){
 tt_pioneer_dram_cohort r={};r.version=r.sweeps=r.queue_depth=r.nonce=1;
 r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=7;return r;
}
static tt_pioneer_dram_cohort RunReceipt(){
 auto r=Request();auto& p=r.reply;p.version=6;p.phase=3;p.complete=1;p.busy_mhz=1350;p.idle_mhz=800;
 p.views_closed=p.pci_closed=p.reservation_closed=1;
 auto* s=p.sweep;s[0]=6;s[1]=s[7]=1;s[2]=10;s[4]=s[5]=s[6]=255;
 s[8]=s[9]=493457;s[10]=7*4096;
 const uint32 routes[7]={18|(12<<6),18|(15<<6),18|(18<<6),17|(12<<6),17|(15<<6),17|(18<<6),17|(21<<6)};
 for(unsigned role=0;role<8;role++){
  auto* v=s+11+role*32;v[0]=0x36435244;v[1]=6;v[2]=role;v[3]=r.nonce;v[4]=v[5]=1;
  v[6]=8388608*(role?1:7);v[8]=100;v[10]=1000100;v[12]=1000000;
  v[14]=role?routes[role-1]:28;v[15]=512*(role?1:7);
  if(!role){v[16]=7;v[17]=42;for(unsigned k=1;k<=7;k++)v[16+k*2]=100*k;}
 }
 auto& t=p.timing;t.version=t.state=1;t.start_lower_us=10;t.start_upper_us=20;
 t.end_lower_us=1000;t.end_upper_us=1010;t.elapsed_min_us=980;t.elapsed_max_us=1000;
 t.cycles=1000000;t.bytes=7*8388608;return r;
}
int main(){
 unsigned good=0,bad=0;PioneerProbeCookie fd;
 assert(PioneerDramSweepControl(NULL,UINT32_MAX,NULL,0)==B_NOT_ALLOWED);bad++;
 {Reset();auto r=Request();assert(PioneerDramCohortControl(&fd,TT_PCI_PROBE_DRAM_COHORT_SUPPORT,&r,sizeof(r))==B_OK
  && copyCalls==2 && !ioCalls && PioneerDramCohortReply(r,TT_PCI_PROBE_DRAM_COHORT_SUPPORT));good++;}
 for(unsigned byte=0;byte<sizeof(tt_pioneer_dram_cohort_reply);byte++){
  Reset();auto r=Request();reinterpret_cast<uint8*>(&r.reply)[byte]=1;
  assert(PioneerDramCohortControl(&fd,TT_PCI_PROBE_DRAM_COHORT_RUN,&r,sizeof(r))==B_BAD_VALUE
   && copyCalls==1 && !ioCalls && !unreserves);bad++;
 }
 for(unsigned word=0;word<12;word++){
  Reset();auto r=Request();reinterpret_cast<uint32*>(&r)[word]^=1;
  assert(PioneerDramCohortControl(&fd,TT_PCI_PROBE_DRAM_COHORT_PREFLIGHT,&r,sizeof(r))!=B_OK
   && !ioCalls && !unreserves && !sColdRecoveryRequired);bad++;
 }
 for(unsigned fault=0;fault<10;fault++){
  Reset();auto r=Request();fd.magic=0x50313030;
  if(!fault)effectiveUid=1;
  if(fault==1)fd.magic=0;
  if(fault==2)currentTeam++;
  if(fault==3)failCopy=1;
  if(fault==4)failCopy=2;
  const unsigned op=fault==5?UINT32_MAX:TT_PCI_PROBE_DRAM_COHORT_SUPPORT;
  const size_t size=fault==6?sizeof(r)-1:fault==7?sizeof(r)+1:sizeof(r);
  void* data=fault==8?NULL:&r;
  if(fault==9)r.queue_depth=3;
  assert(PioneerDramCohortControl(&fd,op,data,size)!=B_OK && !ioCalls && !unreserves);bad++;
 }
 fd.magic=0x50313030;
 for(uint64 generation:{UINT64_C(0),UINT64_C(6),UINT64_C(8),UINT64_MAX}){
  Reset();PioneerDramCohortNativeReply p;
  assert(RunPioneerDramCohortNative(&fd,1,1,1,p,false,generation)==B_BAD_VALUE && !ioCalls && !unreserves
   && !sDramCohortNativeContext && !sDramSweepNativeContext);bad++;
 }
 for(bool fail:{false,true}){
  Reset();failProfile=true;failUnreserve=fail;auto r=Request();
  const status_t status=PioneerDramCohortControl(&fd,TT_PCI_PROBE_DRAM_COHORT_RUN,&r,sizeof(r));
  assert(copyCalls==2 && unreserves==1 && !r.reply.complete && !r.reply.preflight && !pciWrites && views.empty());
  if(fail){
   assert(status==B_DEV_NOT_READY && r.reply.retained && sDramCohortNativeContext && sColdRecoveryRequired);
   const unsigned before=ioCalls;auto again=Request();
   assert(PioneerDramCohortControl(&fd,TT_PCI_PROBE_DRAM_COHORT_SUPPORT,&again,sizeof(again))!=B_OK && ioCalls==before);
   // Fixture-only disposal;NEVER production recovery of retained resources.
   auto* retained=sDramCohortNativeContext;sDramCohortNativeContext=NULL;sDramSweepNativeContext=NULL;
   delete retained;sColdRecoveryRequired=false;bad++;
  }else{assert(status==B_NOT_ALLOWED && r.reply.reservation_closed && !r.reply.retained);good++;}
 }
 for(unsigned fault=0;fault<7;fault++){
  Reset();auto* c=new PioneerDramCohortNativeContext(&fd,thread_info{},16);
  sDramCohortNativeContext=c;sDramSweepNativeContext=c;c->reserved=true;
  c->backing=(uint32*)calloc(c->AllocationWords,4);c->exactBacking=c->backing;
  c->profile.root_bus=0;c->profile.bar0_host_start=UINT64_C(0x4900000000);
  assert(c->io.Map() && views.size()==17);c->ready=c->cohortReady=true;
  c->transaction.config_attempted=1;c->cohortPreflightClosed=true;c->result.preflight=1;
  if(fault==0)c->cohortPreflightClosed=false;
  if(fault==1)c->result.preflight=0;
  if(fault==3)c->result.sweep.started=1;
  if(fault==4)c->result.sweep.unsafe=1;
  if(fault==5)c->clockEntered=true;
  if(fault==6)dropDelete=8;
  const unsigned before=ioCalls;const bool closed=c->CloseCohortResources();
  if(fault==2){assert(closed && c->result.viewsClosed && c->reply.viewsClosed && c->result.pciClosed
   && views.empty() && deletes==17 && pciWrites==11);good++;}
  else if(fault==6){
   assert(!closed && c->quarantined && deletes==8 && !pciWrites);
   const unsigned stopped=ioCalls;assert(!c->CloseCohortResources() && ioCalls==stopped);bad++;
  }else{
   // Scope revalidates17 OS area descriptors before rejecting the receipt.
   // Those read-only endpoint queries are NOT card IO or resource teardown.
   assert(!closed && ioCalls==before+17 && !deletes && !pciWrites && !pciReads);bad++;
  }
  // Fixture-only setup/disposal of flags/OS views,NOT actual native preflight
  // or production recovery after retention. Generic preflight runs separately.
  for(const auto& v:views)free(v.second.address);views.clear();
  delete c;sDramCohortNativeContext=NULL;sDramSweepNativeContext=NULL;sColdRecoveryRequired=false;
 }
 {Reset();auto r=Request();r.reply.version=6;r.reply.sweep[0]=6;r.reply.timing.version=1;
  assert(PioneerDramCohortReply(r,TT_PCI_PROBE_DRAM_COHORT_SUPPORT));good++;
  r.reply.phase=3;r.reply.views_closed=r.reply.pci_closed=r.reply.reservation_closed=r.reply.preflight=1;
  assert(PioneerDramCohortReply(r,TT_PCI_PROBE_DRAM_COHORT_PREFLIGHT));good++;
 }
 {auto r=RunReceipt();assert(PioneerDramCohortReply(r,TT_PCI_PROBE_DRAM_COHORT_RUN));good++;
  for(unsigned i=0;i<sizeof(r.reply)/4;i++){
   auto changed=r;reinterpret_cast<uint32*>(&changed.reply)[i]^=1;
   bool variable=i==4; // busy MHz is bounded DATA,not a fixed expected clock
   for(unsigned role=0;role<8;role++){
    const unsigned begin=10+11+role*32;
    variable|=role && (i==begin+16 || i==begin+17); // arbitrary final payload samples
    for(unsigned k=1;k<=7;k++)variable|=!role && i==begin+16+k*2; // ordered release observations
   }
   const bool accepted=PioneerDramCohortReply(changed,TT_PCI_PROBE_DRAM_COHORT_RUN);
   if(variable){assert(accepted);good++;}else{assert(!accepted);bad++;}
  }
  r.reply.timing.state=2;r.reply.timing.elapsed_min_us=r.reply.timing.elapsed_max_us=0;
  assert(PioneerDramCohortReply(r,TT_PCI_PROBE_DRAM_COHORT_RUN));good++;
  r.reply.timing.elapsed_min_us=1;assert(!PioneerDramCohortReply(r,TT_PCI_PROBE_DRAM_COHORT_RUN));bad++;
 }
 printf("PASS_COHORT_PUBLIC_NATIVE_ENDPOINTS valid%u refused%u allReplyBytesPristine1 sameLockGeneration1 actualFactoryRefusalRetention1 replyGrammar1;SYNTHETIC_SUCCESS_DATA_NOT_NATIVE_PREREQUISITES_ARC_ENGINE_HARDWARE_BANDWIDTH_OR_WORK\n",good,bad);
 return 0;
}
