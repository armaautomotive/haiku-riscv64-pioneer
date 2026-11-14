/* SPDX-License-Identifier: MIT */
// Actual Work dispatch through the batch primitive with simulated device IO.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main pioneer_work_run_unused_main
#include "pioneer_q38_work_run_test.cpp"
#undef main
#pragma GCC diagnostic pop
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkBatch.h"

int main(int argc,char** argv)
{
 assert(argc==2);(void)&pioneer_work_run_unused_main;
 for(unsigned scenario=0;scenario<11;scenario++){
  RunFixture f(argv[1],7);auto jobs=std::make_unique<tt_q38_dispatch_probe[]>(2);
  jobs[0]=*f.job;jobs[1]=*f.job;
  for(unsigned i=0;i<2;i++){jobs[i].output[0][0]=0xdeadbeef;jobs[i].result.version=99;}
  if(scenario==1)jobs[1].reserved[0]=1;
  unsigned authorized=0,runs=0,quarantines=0;
  auto authorize=[&](const tt_q38_dispatch_probe& job){
   authorized++;
   if(scenario==2 && authorized==2)return false;
   for(unsigned w=0;w<job.peers;w++){
    PioneerQ38CohortExtent extent;
    if(!PioneerQ38CohortAuthorizePart(f.banks,f.dram,f.initialized.data(),unsigned(f.initialized.size()),
      job.type,job.inputs,job.part[w],w+1,extent))return false;
   }
   return true;
  };
  auto run=[&](tt_q38_dispatch_probe& job){
   runs++;
   assert(job.output[0][0]==0 && job.result.version==0);
   if((scenario==3 && runs==2) || (scenario==6 && runs==1))return false; // uncertain completion
   *f.job=job;const bool okay=Work(f);job=*f.job;
   if(scenario==4 && runs==2)job.result.nonce++;
   if(scenario==7 && runs==2)job.result.recovery_required=1;
   if(scenario==8 && runs==2)job.result.root_marker^=1;
   if(scenario==9 && runs==2)job.peers++;
   return okay;
  };
  auto quarantine=[&](){quarantines++;PioneerQ38DispatchQuarantine(*f.lease,f.banks);};
  const auto result=PioneerQ38WorkBatch::Execute(jobs.get(),scenario==5?1:2,
   scenario==10?0x7ffffffdU:0,authorize,run,quarantine);
  if(scenario==0){
   assert(result==PioneerQ38WorkBatch::Complete && authorized==2 && runs==2 && !quarantines);
   assert(jobs[0].result.nonce==1 && jobs[1].result.nonce==2 && f.lease->rootNonce==2);
  }else if(scenario==3 || scenario==4 || scenario==6 || scenario==7 || scenario==8 || scenario==9){
   assert(result==PioneerQ38WorkBatch::Unknown && authorized==2 && runs==(scenario==6?1U:2U) && quarantines==1);
   for(unsigned i=0;i<2;i++){
    assert(jobs[i].result.version==0);
    for(const auto& row:jobs[i].output)for(auto value:row)assert(value==0);
   }
  }else assert(result==PioneerQ38WorkBatch::Refused && !runs && !quarantines);
 }
 puts("PASS two-wave Work batch primitive with actual dispatch and synthetic IO; NO DEVICE OR SPEED CLAIM");
}
