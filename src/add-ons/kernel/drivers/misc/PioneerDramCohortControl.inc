/* SPDX-License-Identifier: MIT */
#include "PioneerDramCohortProtocol.h"
static_assert(sizeof(PioneerDramCohortNativeReply)==sizeof(tt_pioneer_dram_cohort_reply),"native/public DATA size");
static_assert(offsetof(PioneerDramCohortNativeReply,sweep)==offsetof(tt_pioneer_dram_cohort_reply,sweep),"native/public sweep offset");
static_assert(offsetof(PioneerDramCohortNativeReply,preflight)==offsetof(tt_pioneer_dram_cohort_reply,preflight),"native/public preflight offset");
static_assert(offsetof(PioneerDramCohortReadSweepResult,timing)==268*4,"native/public timing offset");
static status_t PioneerDramCohortControl(PioneerProbeCookie* fd,uint32 operation,void* data,size_t length){
 if(geteuid()!=0 || !fd || fd->magic!=0x50313030)return B_NOT_ALLOWED;
 if(!data || length!=sizeof(tt_pioneer_dram_cohort)
  || (operation!=TT_PCI_PROBE_DRAM_COHORT_SUPPORT && operation!=TT_PCI_PROBE_DRAM_COHORT_PREFLIGHT
   && operation!=TT_PCI_PROBE_DRAM_COHORT_RUN))return B_BAD_VALUE;
 thread_info t={};status_t status=get_thread_info(find_thread(NULL),&t);
 if(status!=B_OK || t.team!=fd->team)return B_NOT_ALLOWED;
 // Large fixed receipts are heap DATA,not two nested1.2KiB kernel frames.
 auto* request=new(std::nothrow) tt_pioneer_dram_cohort{};
 auto* reply=new(std::nothrow) PioneerDramCohortNativeReply;
 if(!request || !reply){delete request;delete reply;return B_NO_MEMORY;}
 status=user_memcpy(request,data,sizeof(*request));
 if(status!=B_OK || !PioneerDramCohortRequest(*request)){
  delete request;delete reply;return status!=B_OK?status:B_BAD_VALUE;
 }
 if(operation==TT_PCI_PROBE_DRAM_COHORT_SUPPORT){
  status=AcquireProbeLock();
  if(status==B_OK){
   status=request->expected_tag==PIONEER_DRIVER_BUILD_TAG && request->expected_generation==sReloadGeneration
    && !sDramSweepNativeContext && !sDramCohortNativeContext && !fd->context
    && sReloadGuard && sReloadGuard->version==1?B_OK:B_DEV_NOT_READY;
   release_sem(sLock);
  }
 }else status=RunPioneerDramCohortNative(fd,request->sweeps,request->queue_depth,request->nonce,*reply,
  operation==TT_PCI_PROBE_DRAM_COHORT_PREFLIGHT,request->expected_generation);
 // Captured DATA only after execution,including retention. Copy failure or FD
 // close never retries,resets,cleans up or silently executes legacy model END.
 memcpy(&request->reply,reply,sizeof(*reply));
 const status_t copied=user_memcpy(data,request,sizeof(*request));
 delete reply;delete request;
 return copied!=B_OK?copied:status;
}
