/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_WORK_BATCH_H
#define PIONEER_Q38_WORK_BATCH_H
#if !defined(TT_PIONEER_Q38_WORK_MODE) || TT_PIONEER_Q38_WORK_MODE != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Work batch is separate from CPU validation and reference modes
#endif
#include "PioneerQ38DispatchProtocol.h"
#include <string.h>

// Bounded batch primitive for future private Work ioctls. The caller owns
// both kernel-resident requests, the session lock and their user-copy. This
// header does not register an operation, acquire an owner, or execute a card
// by itself. No CPU reference or fallback is performed.
namespace PioneerQ38WorkBatch {
enum Result { Refused, Complete, Unknown };
enum { Count = 2 };

// authorize() must check the current session and all initialized extents
// without device writes; priorNonce must come from the locked live owner;
// run() must invoke the ordinary Work dispatch and stop on uncertainty;
// quarantine() must retain the complete owner without further device IO.
// Requests remain private and MUST NOT be copied to the caller unless the
// result is Complete. A post-launch failure clears both partial outputs.
template<class Authorize,class Run,class Quarantine>
Result Execute(tt_q38_dispatch_probe* jobs,unsigned count,uint32_t priorNonce,
 Authorize authorize,Run run,Quarantine quarantine)
{
 if(!jobs || count!=Count || priorNonce>0x7ffffffcU)return Refused;
 for(unsigned i=0;i<Count;i++)if(!PioneerQ38DispatchValid(jobs[i]) || !authorize(jobs[i]))return Refused;
 for(unsigned i=0;i<Count;i++){
  memset(jobs[i].output,0,sizeof(jobs[i].output));memset(&jobs[i].result,0,sizeof(jobs[i].result));
 }
 for(unsigned i=0;i<Count;i++){
  auto& job=jobs[i];
  const unsigned peers=job.peers;
  const bool okay=run(job);const auto& r=job.result;
  if(!okay || r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.recovery_required
   || job.peers!=peers || r.nonce!=priorNonce+i+1 || r.peers!=peers || r.root_started!=1
   || r.root_completed!=1 || r.root_held!=1 || r.root_marker!=0x314d3851 || r.root_count!=peers){
   quarantine();
   for(unsigned j=0;j<Count;j++){memset(jobs[j].output,0,sizeof(jobs[j].output));memset(&jobs[j].result,0,sizeof(jobs[j].result));}
   return Unknown;
  }
 }
 return Complete;
}
}
#endif
