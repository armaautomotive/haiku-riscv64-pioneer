/* SPDX-License-Identifier: MIT */
#include "PioneerDramSweepProtocol.h"
static_assert(sizeof(PioneerDramSweepNativeReply)==sizeof(tt_pioneer_dram_sweep_reply),"private/public DATA sizes");
static_assert(offsetof(PioneerDramSweepNativeReply,sweep)==offsetof(tt_pioneer_dram_sweep_reply,sweep),"inner DATA offset");
static_assert(offsetof(PioneerDramSweepNativeReply,preflight)==offsetof(tt_pioneer_dram_sweep_reply,preflight),"preflight offset");
static status_t PioneerDramSweepControl(PioneerProbeCookie* fd,uint32 operation,void* data,size_t length){
 if(geteuid()!=0 || !fd || fd->magic!=0x50313030)return B_NOT_ALLOWED;
 if(!data || length!=sizeof(tt_pioneer_dram_sweep)
  || (operation!=TT_PCI_PROBE_DRAM_SWEEP_SUPPORT && operation!=TT_PCI_PROBE_DRAM_SWEEP_PREFLIGHT
   && operation!=TT_PCI_PROBE_DRAM_SWEEP_RUN))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=fd->team)return B_NOT_ALLOWED;
 tt_pioneer_dram_sweep request={};status=user_memcpy(&request,data,sizeof(request));
 if(status!=B_OK)return status;
 if(!PioneerDramSweepRequest(request))return B_BAD_VALUE;
 PioneerDramSweepNativeReply reply;
 if(operation==TT_PCI_PROBE_DRAM_SWEEP_SUPPORT){
  status=AcquireProbeLock();if(status!=B_OK)return status;
  status=request.expected_tag==PIONEER_DRIVER_BUILD_TAG && request.expected_generation==sReloadGeneration
   && !sDramSweepNativeContext && !fd->context && sReloadGuard && sReloadGuard->version==1?B_OK:B_DEV_NOT_READY;
  release_sem(sLock);
 }else status=RunPioneerDramReadSweepNative(fd,request.bank,request.chunk_shift,request.sweeps,
  request.nonce,reply,operation==TT_PCI_PROBE_DRAM_SWEEP_PREFLIGHT,request.expected_generation);
 // Always deliver the already captured result,including retention. No extra
 // card observation or cleanup after a failure;failed copy cannot restart it.
 memcpy(&request.reply,&reply,sizeof(reply));
 const status_t copied=user_memcpy(data,&request,sizeof(request));
 return copied!=B_OK?copied:status;
}
