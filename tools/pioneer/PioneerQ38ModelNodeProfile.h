/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MODEL_NODE_PROFILE_H
#define PIONEER_Q38_MODEL_NODE_PROFILE_H
#if !defined(PIONEER_Q38_MODEL_NODE_PROFILE) || !defined(PIONEER_Q38_MODEL_ROOT_PROFILE)
#error Separate fixed-public THREE32 node+returned-root CPU diagnostic ONLY
#endif
#include <stdint.h>
// CPU METRICS ONLY,no tensor data reads/device IO/allocator/source/cleanup
// authority. Scheduler observation deliberately splits/synchronizes nodes:
// measured wall time INCLUDES this diagnostic overhead,NOT production speed.
// A bad metric is sticky-invalid but NEVER cancels or alters graph success.
class PioneerQ38ModelNodeProfile {
 unsigned sequence=0,position=0,nodes=0,warm=0,currentOp=0;
 const void* current=nullptr;uint64_t began=0;
 bool valid=true,active=false,pending=false;
public:
 enum { Operators=256 };
 struct Bucket{uint64_t nodes=0,nanos=0;};
 Bucket bucket[Operators];
 bool Active()const{return active;}
 void Begin(unsigned seq,unsigned pos){
  if(active||seq!=sequence||pos!=position||seq>=3||pos>=32)valid=false;
  active=true;nodes=0;pending=false;current=nullptr;
 }
 void Ask(const void* node,unsigned op,uint64_t now){
  if(!active||!valid)return;
  if(pending||!node||op>=Operators||now==UINT64_MAX){valid=false;return;}
  current=node;currentOp=op;began=now;pending=true;
 }
 void After(const void* node,uint64_t now){
  if(!active||!valid)return;
  if(!pending||node!=current||now==UINT64_MAX||now<began||nodes==UINT32_MAX){valid=false;return;}
  pending=false;current=nullptr;nodes++;
  if(position<5)return;
  auto& b=bucket[currentOp];uint64_t elapsed=now-began;
  if(b.nodes==UINT64_MAX||elapsed>UINT64_MAX-b.nanos){valid=false;return;}
  b.nodes++;b.nanos+=elapsed;
 }
 void End(bool success){
  if(!active||!success||pending||!nodes)valid=false;
  active=false;pending=false;current=nullptr;
  if(!valid)return;
  if(position>=5)warm++;
  if(++position==32){position=0;sequence++;}
 }
 bool PublishAllowed(bool modelOkay,bool cleanEnd,bool stopped)const{
  return valid&&!active&&!pending&&sequence==3&&!position&&warm==81&&modelOkay&&cleanEnd&&!stopped;
 }
};
#endif
