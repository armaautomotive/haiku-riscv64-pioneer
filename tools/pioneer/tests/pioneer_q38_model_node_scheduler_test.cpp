/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_MODEL_NODE_PROFILE
#define PIONEER_Q38_MODEL_ROOT_PROFILE
#include "../PioneerQ38ModelNodeProfile.h"
#include <cassert>
#include <cstdio>
#include <cstdint>
struct ggml_tensor{unsigned op,computed=0;};
struct ggml_cgraph{int n_nodes;ggml_tensor** nodes;};
static uint64_t clockValue=0;
static uint64_t NodeNanos(){return clockValue;}
#include "model_node_observer.inc"
using Callback=bool(*)(ggml_tensor*,bool,void*);
struct Scheduler{Callback callback_eval;void* callback_eval_user_data;};
struct Split{ggml_cgraph graph;};
enum ggml_status{GGML_STATUS_SUCCESS,GGML_STATUS_FAILED};
static unsigned computeCalls=0,syncCalls=0,failCall=0;
static bool badClock=false;
static ggml_cgraph ggml_graph_view(ggml_cgraph* g,int first,int last){assert(first>=0&&first<last&&last<=g->n_nodes);return{last-first,g->nodes+first};}
static ggml_status ggml_backend_graph_compute_async(int backend,ggml_cgraph* g){
 assert(backend==17);computeCalls++;if(computeCalls==failCall)return GGML_STATUS_FAILED;
 for(int i=0;i<g->n_nodes;i++){g->nodes[i]->computed++;clockValue+=g->nodes[i]->op+3;}
 if(badClock)clockValue=0;
 return GGML_STATUS_SUCCESS;
}
static void ggml_backend_synchronize(int backend){assert(backend==17);syncCalls++;clockValue++;}
static ggml_status Execute(Scheduler* sched,Split* split){
 const int split_backend=17;
 // EXACT scheduler loop from pristinec060ca974c77,NOT dirty llama source.
#include "model_node_scheduler.inc"
 return GGML_STATUS_SUCCESS;
}
int main(){
 ggml_tensor a[7]={};ggml_tensor* pointers[7];for(unsigned i=0;i<7;i++){a[i].op=i;pointers[i]=&a[i];}
 Split split={{7,pointers}};PioneerQ38ModelNodeProfile p;Scheduler sched={NodeObserver,&p};
 for(unsigned seq=0;seq<3;seq++)for(unsigned pos=0;pos<32;pos++){
  p.Begin(seq,pos);assert(Execute(&sched,&split)==GGML_STATUS_SUCCESS);p.End(true);
 }
 assert(p.PublishAllowed(true,true,false)&&computeCalls==96*7&&syncCalls==computeCalls);
 for(unsigned i=0;i<7;i++)assert(a[i].computed==96&&p.bucket[i].nodes==81&&p.bucket[i].nanos==81*(i+4));
 for(unsigned scenario=0;scenario<4;scenario++){
  PioneerQ38ModelNodeProfile bad;Scheduler probe={NodeObserver,&bad};computeCalls=syncCalls=0;
  failCall=scenario==2?3:0;badClock=scenario==3;clockValue=100;
  if(scenario!=0)bad.Begin(0,0);
  if(scenario==1)bad.Ask(nullptr,0,clockValue); // metrics already invalid
  auto status=Execute(&probe,&split);
  assert(status==(scenario==2?GGML_STATUS_FAILED:GGML_STATUS_SUCCESS));
  assert(computeCalls==(scenario==0?1U:scenario==2?3U:7U));
  assert(syncCalls==(scenario==0?1U:scenario==2?2U:7U));
  if(scenario!=0)bad.End(status==GGML_STATUS_SUCCESS);
  assert(!bad.PublishAllowed(true,true,false));
 }
 puts("PASS ACTUAL pristine scheduler loop+actual node observer:ALL96positions/7operators/81warm exact counts and clock sums;inactive callback batches;invalid metrics/backward clock NEVER cancel/skip ANY node;real graph failure unchanged;NO tensor data reads/native timing or driver IO");
}
