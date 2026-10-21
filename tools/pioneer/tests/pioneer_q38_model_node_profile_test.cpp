/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_MODEL_NODE_PROFILE
#define PIONEER_Q38_MODEL_ROOT_PROFILE
#include "../PioneerQ38ModelNodeProfile.h"
#include <cassert>
#include <cstdio>
#include <cstring>
struct ggml_tensor{unsigned op;};
static uint64_t clockValue=0;
static uint64_t NodeNanos(){return clockValue;}
#include "model_node_observer.inc"
static void Position(PioneerQ38ModelNodeProfile& p,unsigned seq,unsigned pos){
 p.Begin(seq,pos);for(unsigned op=0;op<7;op++){
  ggml_tensor node={op};clockValue=100+op*17;assert(NodeObserver(&node,true,&p));clockValue+=op+3;assert(NodeObserver(&node,false,&p));
 }p.End(true);
}
static void Prefix(PioneerQ38ModelNodeProfile& p){for(unsigned pos=0;pos<5;pos++)Position(p,0,pos);p.Begin(0,5);}
int main(){
 PioneerQ38ModelNodeProfile p;ggml_tensor node={0};
 assert(!NodeObserver(&node,true,&p)&&NodeObserver(&node,false,&p)&&!NodeObserver(&node,true,nullptr)&&NodeObserver(&node,false,nullptr));
 for(unsigned seq=0;seq<3;seq++)for(unsigned pos=0;pos<32;pos++){
  assert(!p.PublishAllowed(true,true,false));Position(p,seq,pos);
 }
 assert(p.PublishAllowed(true,true,false));
 for(unsigned op=0;op<256;op++)assert(p.bucket[op].nodes==(op<7?81U:0U)&&p.bucket[op].nanos==(op<7?81U*(op+3):0U));
 assert(!p.PublishAllowed(false,true,false)&&!p.PublishAllowed(true,false,false)&&!p.PublishAllowed(true,true,true));
 unsigned refused=0;
 for(unsigned fault=0;fault<17;fault++){
  PioneerQ38ModelNodeProfile bad;Prefix(bad);ggml_tensor a={0},b={0};clockValue=100;
  switch(fault){
   case 0:assert(NodeObserver(nullptr,true,&bad));break;
   case 1:a.op=256;assert(NodeObserver(&a,true,&bad));break;
   case 2:clockValue=UINT64_MAX;assert(NodeObserver(&a,true,&bad));break;
   case 3:assert(NodeObserver(&a,false,&bad));break;
   case 4:assert(NodeObserver(&a,true,&bad)&&NodeObserver(&b,false,&bad));break;
   case 5:assert(NodeObserver(&a,true,&bad));clockValue=99;assert(NodeObserver(&a,false,&bad));break;
   case 6:assert(NodeObserver(&a,true,&bad));clockValue=UINT64_MAX;assert(NodeObserver(&a,false,&bad));break;
   case 7:assert(NodeObserver(&a,true,&bad)&&NodeObserver(&b,true,&bad));break;
   case 8:assert(NodeObserver(&a,true,&bad));break;
   case 9:bad.Begin(0,5);break;
   case 10:bad.Begin(1,0);break;
   case 11:bad.Begin(0,6);break;
   case 12:bad.Begin(3,0);break;
   case 13:bad.Begin(0,32);break;
   case 14:bad.End(false);break;
   case 15:bad.End(true);bad.End(true);break;
   case 16:bad.End(true);break; // zero observed nodes
  }
  // A failed metric NEVER changes callback graph-success decisions.
  assert(NodeObserver(&a,false,&bad));bad.End(true);
  auto old=bad.bucket[0];bad.Begin(0,0);clockValue=200;assert(NodeObserver(&a,true,&bad));clockValue=201;assert(NodeObserver(&a,false,&bad));bad.End(true);
  assert(!memcmp(&old,&bad.bucket[0],sizeof(old))&&!bad.PublishAllowed(true,true,false));refused++;
 }
 for(unsigned fault=0;fault<2;fault++){
  PioneerQ38ModelNodeProfile bad;Prefix(bad);auto& bucket=bad.bucket[0];
  if(fault)bucket.nanos=UINT64_MAX;else bucket.nodes=UINT64_MAX;
  auto old=bucket;clockValue=100;assert(NodeObserver(&node,true,&bad));clockValue=101;assert(NodeObserver(&node,false,&bad));bad.End(true);
  assert(!memcmp(&old,&bucket,sizeof(old))&&!bad.PublishAllowed(true,true,false));refused++;
 }
 printf("PASS actual node observer/CPU metrics:96positions exactly81warm,7operator buckets/excluded startup,publication ONLY after complete clean THREE32,%u sticky atomic-overflow/identity/clock/transition cases;ALL malformed metric callbacks still allow graph computation;NO tensor data/device IO/cleanup authority\n",refused);
}
