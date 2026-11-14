/* SPDX-License-Identifier: MIT */
// Extracted real dispatch preparation/locking with simulated device IO only.
#define PIONEER_Q38_DISPATCH_CONTROL_MOCK_ONLY
#define user_memcpy batch_original_user_memcpy
#include "pioneer_q38_dispatch_control_test.cpp"
#undef user_memcpy
static bool batchPartialCopy=false;
static status_t user_memcpy(void* to,const void* from,size_t bytes){
 if(batchPartialCopy && copyCalls+1==failCopy){copyCalls++;memcpy(to,from,bytes/2);return -99;}
 return batch_original_user_memcpy(to,from,bytes);
}
#define PIONEER_Q38_WORK_BATCH_CONTROL_INSPECTION 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkBatchControl.inc"

int main(int argc,char** argv)
{
 assert(argc==2);(void)&Q38DispatchControl;
 for(unsigned scenario=0;scenario<
#ifdef PIONEER_Q38_SPAN_PRELOAD
  8
#else
  7
#endif
  ;scenario++){
  PioneerProbeCookie cookie;Begin(cookie,argv[1]);auto* c=cookie.context;
  auto packet=std::make_unique<PioneerQ38WorkBatchPacket>();
  packet->version=1;packet->count=2;packet->jobs[0]=*Request();packet->jobs[1]=*Request();
  copyCalls=failCopy=0;batchPartialCopy=false;
  if(scenario==1)packet->jobs[1].reserved[0]=1;
  if(scenario==2)failCopy=1; // input copy must not touch the owner
  if(scenario==3)failCopy=2; // successful device execution, lost reply
  if(scenario==4)c->io.rootFault=2; // uncertain first Work dispatch
  if(scenario==5){ // only the second wave references an uninitialized extent
   packet->jobs[1].part[0].offset=0x08000040;
   uint32_t address=0,route=0,index=0;
   assert(c->banks.Page(c->dram,0,0x08000000,address,route,index));
   c->initialized[index]=0;
  }
  if(scenario==6){failCopy=2;batchPartialCopy=true;}
#ifdef PIONEER_Q38_SPAN_PRELOAD
  if(scenario==7)c->q38Span=new PioneerQ38SpanLease; // an unclosed upload cannot run
#endif
  const status_t status=Q38WorkBatchControlInspection(&cookie,packet.get(),sizeof(*packet));
  if(!scenario){
   assert(status==B_OK && packet->jobs[0].result.nonce==1 && packet->jobs[1].result.nonce==2
    && c->q38Dispatch && c->q38Dispatch->rootNonce==2 && !c->Unsafe());
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
   // Prove the real Work dispatch controller reuses the staged activation
   // across the two jobs in one batch (the simulated root reports mode 2).
   assert(c->io.inputMode==2);
#endif
   End(cookie,true);
  }else if(scenario==1 || scenario==2 || scenario==5 || scenario==7){
   assert(status==(scenario==2?-99:B_NOT_ALLOWED) && !c->q38Dispatch && !c->Unsafe());
   End(cookie,true);
  }else{
   assert(status==((scenario==3 || scenario==6)?-99:B_DEV_NOT_READY) && c->Unsafe());
   if(scenario==6)assert(packet->jobs[0].result.version==1 && packet->jobs[1].result.version==0);
   else assert(packet->jobs[0].result.version==0 && packet->jobs[1].result.version==0);
   End(cookie,false);sColdRecoveryRequired=false;
  }
  failCopy=0;
 }
 puts("PASS isolated Work batch control: actual lock, preparation, authorization, dispatch and lost-reply quarantine; SIMULATED IO ONLY");
}
