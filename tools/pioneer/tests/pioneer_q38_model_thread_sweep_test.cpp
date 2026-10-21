/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#define B_DEVICE_OP_CODES_END 10000
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR
#define PIONEER_Q38_MODEL_THREAD_SWEEP
#include "../PioneerQ38ModelThreadSweep.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h"
static constexpr uint64_t ExpectedTag=
#ifdef PIONEER_Q38_MODEL_THREAD_RAW_CACHE
 UINT64_C(0x2026102100010022);
#else
 UINT64_C(0x2026102100010012);
#endif
struct llama_context{};
struct Pool{unsigned threads;};using ggml_threadpool_t=Pool*;
static llama_context context;static Pool large{PioneerQ38ModelWideThreads},small{PioneerQ38ModelNarrowThreads};
static std::vector<unsigned> events;
static void llama_synchronize(llama_context* c){assert(c==&context);events.push_back(1);}
static void llama_set_n_threads(llama_context* c,unsigned a,unsigned b){assert(c==&context&&a==b&&(a==PioneerQ38ModelNarrowThreads||a==PioneerQ38ModelWideThreads)&&events.back()==1);events.push_back(a);}
static void llama_attach_threadpool(llama_context* c,ggml_threadpool_t a,ggml_threadpool_t b){assert(c==&context&&a==b&&a->threads==events.back());events.push_back(2);}
#include "model_thread_switch.inc"
struct Mode{
#include "model_thread_support.inc"
};
struct IO{
 unsigned calls=0,fault=0;
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==17);calls++;
  if(calls==1){
   assert(op==TT_PCI_PROBE_RELOAD_STATUS&&bytes==sizeof(tt_pioneer_driver_reload));auto& r=*static_cast<tt_pioneer_driver_reload*>(data);assert(r.version==1);
   r={};r.version=1;r.driver=55;r.build_tag=ExpectedTag;r.guard_version=1;r.generation=53;r.open_count=1;
   if(fault==1)return false;
   if(fault==2)r.version=2;if(fault==3)r.driver=54;if(fault==4)r.build_tag++;if(fault==5)r.guard_version=0;
   if(fault==6)r.generation=0;if(fault==7)r.quiesced=1;if(fault==8)r.cold=1;if(fault==9)r.active_owner=1;if(fault==10)r.open_count=2;
   return true;
  }
  assert(calls==2&&op==TT_PCI_PROBE_Q38_WORK_SUPPORT&&bytes==sizeof(tt_q38_dispatch_support));auto& s=*static_cast<tt_q38_dispatch_support*>(data);
  s={};s.driver=TT_Q38_DISPATCH_DRIVER;s.version=TT_Q38_WORK_SUPPORT_VERSION;s.peers=119;s.rows=256;s.max_inputs=17408;s.banks=7;s.offset_alignment=64;s.persistent=1;
  if(fault==11)return false;
  if(fault==12)s.version=3;if(fault==13)s.peers=120;if(fault==14)s.rows=0;if(fault==15)s.max_inputs=5120;
  if(fault==16)s.banks=8;if(fault==17)s.offset_alignment=32;if(fault==18)s.persistent=0;if(fault==19)s.model_inference=1;
  if(fault==20)s.version=4;if(fault==21)s.driver=54;
  return true;
 }
};
static int Arguments(unsigned sequences,unsigned limit,bool graph){bool repeatedRun=sequences==3;
#include "model_thread_arguments.inc"
 return 0;
}
int main(){
 assert(PioneerQ38ModelSequenceThreads(0)==PioneerQ38ModelWideThreads&&PioneerQ38ModelSequenceThreads(1)==PioneerQ38ModelNarrowThreads&&PioneerQ38ModelSequenceThreads(2)==PioneerQ38ModelWideThreads);
 for(unsigned i=0;i<3;i++)assert(SetModelThreadSequence(&context,&large,&small,i));
 assert((events==std::vector<unsigned>{1,PioneerQ38ModelWideThreads,2,1,PioneerQ38ModelNarrowThreads,2,1,PioneerQ38ModelWideThreads,2}));
 for(unsigned fault=0;fault<4;fault++){events.clear();assert(!SetModelThreadSequence(fault==0?nullptr:&context,fault==1?nullptr:&large,fault==2?nullptr:&small,fault==3?3:0));assert(events.empty());}
 for(unsigned fault=0;fault<22;fault++){IO io;io.fault=fault;Mode m;assert(m.Support(io,17)==!fault);assert(io.calls==(fault&&fault<=10?1U:2U));}
 unsigned tags=0;for(unsigned bit=0;bit<64;bit++){tt_pioneer_driver_reload r={};r.version=1;r.driver=55;r.build_tag=ExpectedTag^(UINT64_C(1)<<bit);r.guard_version=1;r.generation=53;r.open_count=1;assert(!PioneerQ38ModelThreadSweepTag(r));tags++;}
 for(uint64_t tag:{UINT64_C(0),UINT64_C(0x2026102100010012),UINT64_C(0x2026102100010017),UINT64_C(0x2026102100010021),UINT64_C(0x2026102100010022),UINT64_C(0x2026102100010024),UINT64_C(0x2026102100010025)}){
  tt_pioneer_driver_reload r={};r.version=1;r.driver=55;r.build_tag=tag;r.guard_version=1;r.generation=53;r.open_count=1;
  assert(PioneerQ38ModelThreadSweepTag(r)==(tag==ExpectedTag));
 }
 unsigned args=0;for(unsigned n=0;n<5;n++)for(unsigned limit:{0U,1U,4U,32U,33U})for(bool graph:{false,true}){assert((Arguments(n,limit,graph)==0)==(n==3&&limit==32&&!graph));args++;}
 printf("PASS ACTUAL synchronized%u/%u/%u pool switch;4 invalid calls NO effects;22 extracted Work support/owner/ABI failures;64 wrong-tag bits+7known-role tags;50 extracted fixed-public argument guards;CPU mocks ONLY NO device/thread creation\n",unsigned(PioneerQ38ModelWideThreads),unsigned(PioneerQ38ModelNarrowThreads),unsigned(PioneerQ38ModelWideThreads));
 assert(tags==64&&args==50);
}
