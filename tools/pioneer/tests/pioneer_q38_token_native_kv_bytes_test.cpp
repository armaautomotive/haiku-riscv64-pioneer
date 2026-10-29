/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#define PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE 1
#define PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY 1
#include <memory>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
struct KvComponent {
#include "pioneer_q38_token_native_kv_bytes.inc"
};
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());KvComponent b;b.BindKvBytes(*plan);
 unsigned views=0,refusals=0;uint8 raw[16];
 for(const auto& entry:b.kvSpans){const auto& span=entry.second;
  for(unsigned address:{span.address,span.address+span.bytes-16}){
   memset(raw,0xcc,sizeof(raw));const uint64 reads=b.kvReadWords;const auto size=b.kvWords.size();
   assert(!b.ReadKvBytes(span.route,address,raw,16) && b.kvReadWords==reads && b.kvWords.size()==size);for(auto byte:raw)assert(byte==0xcc);refusals++;
   const uint32 value=0xa53791e5U^address^span.route;
   assert(b.PutKvWord(span.route,address,value));assert(!b.ReadKvBytes(span.route,address,raw,16));for(auto byte:raw)assert(byte==0xcc);refusals++;
   for(unsigned i=1;i<4;i++)assert(b.PutKvWord(span.route,address+i*4,value+i));
   assert(b.ReadKvBytes(span.route,address,raw,16));for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)assert(raw[i*4+j]==uint8((value+i)>>(j*8)));views++;
   const uint64 writes=b.kvWriteWords;
   assert(!b.PutKvWord(span.route,address,value) && !b.PutKvWord(span.route,address,value^1) && b.kvWriteWords==writes);
   assert(b.ReadKvBytes(span.route,address,raw,16));for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)assert(raw[i*4+j]==uint8((value+i)>>(j*8)));refusals+=2;
  }
  for(unsigned fault=0;fault<7;fault++){
   memset(raw,0xcc,sizeof(raw));const uint64 reads=b.kvReadWords,writes=b.kvWriteWords;const auto size=b.kvWords.size();
   const unsigned route=fault==0?UINT32_MAX:span.route,address=fault==1?span.address+1:fault==2?UINT32_MAX-3:fault==3?span.address+span.bytes-4:span.address;
   const unsigned bytes=fault==2?8:fault==3?8:fault==4?0:fault==5?UINT32_MAX:4;
   assert(!b.ReadKvBytes(route,address,fault==6?nullptr:raw,bytes));for(auto byte:raw)assert(byte==0xcc);
   assert(b.kvReadWords==reads && b.kvWords.size()==size && b.kvWriteWords==writes);refusals++;
  }
  assert(!b.PutKvWord(span.route,span.address+1,0));refusals++;
 }
 for(unsigned i=0;i<plan->Count();i++){const auto& p=*plan->Part(i);if(p.kind==plan->KvKind)continue;
  memset(raw,0xcc,sizeof(raw));assert(!b.ReadKvBytes(PioneerQ38ProjectionRoute(p.bank),plan->Base+p.offset,raw,4)
   && !b.PutKvWord(PioneerQ38ProjectionRoute(p.bank),plan->Base+p.offset,0));for(auto byte:raw)assert(byte==0xcc);refusals+=2;
 }
 assert(views==224 && b.kvReadWords==1792 && b.kvWriteWords==896 && b.kvWords.size()==896);
 printf("PASS empty native TEST KV endpoints/112 canonical regions536870912 reserved bytes/ZERO readable initial words/224 first+last raw-write/read views/%u missing+partial+unaligned+overflow+cross-span+null+non-KV+identical-or-different-rewrite refusals/append-only+atomic output/no zero fallback;COMPONENT ONLY,NOT actual KV append/attention/token/native owner/physical/Work/TPS\n",refusals);
}
