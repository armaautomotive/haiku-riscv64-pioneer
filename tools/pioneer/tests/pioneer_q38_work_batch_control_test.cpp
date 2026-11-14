/* SPDX-License-Identifier: MIT */
// Extracted real dispatch preparation/locking with simulated device IO only.
#define PIONEER_Q38_DISPATCH_CONTROL_MOCK_ONLY
#include "pioneer_q38_dispatch_control_test.cpp"
#define PIONEER_Q38_WORK_BATCH_CONTROL_INSPECTION 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkBatchControl.inc"

int main(int argc,char** argv)
{
 assert(argc==2);(void)&Q38DispatchControl;
 for(unsigned scenario=0;scenario<6;scenario++){
  PioneerProbeCookie cookie;Begin(cookie,argv[1]);auto* c=cookie.context;
  auto packet=std::make_unique<PioneerQ38WorkBatchPacket>();
  packet->version=1;packet->count=2;packet->jobs[0]=*Request();packet->jobs[1]=*Request();
  copyCalls=failCopy=0;
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
  const status_t status=Q38WorkBatchControlInspection(&cookie,packet.get(),sizeof(*packet));
  if(!scenario){
   assert(status==B_OK && packet->jobs[0].result.nonce==1 && packet->jobs[1].result.nonce==2
    && c->q38Dispatch && c->q38Dispatch->rootNonce==2 && !c->Unsafe());
   End(cookie,true);
  }else if(scenario==1 || scenario==2 || scenario==5){
   assert(status==(scenario==2?-99:B_NOT_ALLOWED) && !c->q38Dispatch && !c->Unsafe());
   End(cookie,true);
  }else{
   assert(status==(scenario==3?-99:B_DEV_NOT_READY) && c->Unsafe());
   // This mock fails before copying. A real partial copy must be discarded
   // by the caller on error; the driver quarantines either way.
   assert(packet->jobs[0].result.version==0 && packet->jobs[1].result.version==0);
   End(cookie,false);sColdRecoveryRequired=false;
  }
  failCopy=0;
 }
 puts("PASS isolated Work batch control: actual lock, preparation, authorization, dispatch and lost-reply quarantine; SIMULATED IO ONLY");
}
