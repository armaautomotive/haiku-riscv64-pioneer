/* SPDX-License-Identifier: MIT */
// COMPONENT ONLY: synthetic nonzero raw bytes expose missing/default/alias
// errors. This does NOT initialize or grant a native model owner.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#define PIONEER_Q38_DECODER_RUNTIME 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvHistoryBanks.h"
#define PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE 1
#define PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY 1
#include <memory>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
struct BytesComponent {
 unsigned rootStarts=0,workspaceAddress=0;
 PioneerQ38GdnRegion recurrent[672]={};PioneerQ38ConvHistoryExtent history[336]={};unsigned recurrentFirst[672]={};
 std::vector<uint32> recurrentWords=std::vector<uint32>(73728*1024,0x13579bdf);
 std::vector<uint32> historyWords=std::vector<uint32>(1680*1024,0x2468ace0);
 std::vector<uint32> runtimeDram=std::vector<uint32>(67108864/4,0x89abcdef);
#include "pioneer_q38_token_native_mutable_bytes.inc"
};
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 auto bytes=std::make_unique<BytesComponent>();auto& b=*bytes;unsigned pages=0;
 for(unsigned i=0;i<672;i++){b.recurrent[i]=canonical->RecurrentRegions()[i];b.recurrent[i].offset+=canonical->Base;
  b.recurrentFirst[i]=pages;pages+=b.recurrent[i].heads*16;}
 assert(pages==73728);unsigned count=0;
 for(unsigned i=0;i<canonical->Count();i++){const auto& p=*canonical->Part(i);if(p.kind!=canonical->ConvKind)continue;
  b.history[count++]={p.bank,canonical->Base+p.offset,PioneerQ38ProjectionRoute(p.bank),0,p.allocated/4096,p.allocated};}
 assert(count==336);b.workspaceAddress=canonical->Base+canonical->Workspace(0)->offset;b.BindMutableBytes();
 unsigned aliases=0,refusals=0;
 for(auto& entry:b.mutableBytes){auto& span=entry.second;uint8 raw[64];const uint32 expected=span.kind==0?0x13579bdf:span.kind==1?0x2468ace0:0x89abcdef;
  for(unsigned address:{span.address,span.address+span.bytes-64}){
   assert(b.ReadMutableBytes(span.route,address,raw,64));for(unsigned i=0;i<16;i++)for(unsigned j=0;j<4;j++)assert(raw[i*4+j]==uint8(expected>>(j*8)));aliases++;
  }
  const uint32 changed=expected^span.address^span.route;assert(b.PutMutableWord(span.route,span.address,changed) && span.words[0]==changed);
  assert(b.ReadMutableBytes(span.route,span.address,raw,4));for(unsigned j=0;j<4;j++)assert(raw[j]==uint8(changed>>(j*8)));
  // Deliberate raw endpoint unit input,NOT a model product or actual zero history.
  assert(b.PutMutableWord(span.route,span.address,expected) && span.words[0]==expected);
  for(unsigned fault=0;fault<7;fault++){
   memset(raw,0xcc,sizeof(raw));
   const unsigned route=fault==0?UINT32_MAX:span.route,address=fault==1?span.address+1:fault==2?UINT32_MAX-3:fault==3?span.address+span.bytes-4:span.address;
   const unsigned size=fault==2?8:fault==3?8:fault==4?0:fault==5?UINT32_MAX:4;
   assert(!b.ReadMutableBytes(route,address,fault==6?nullptr:raw,size));for(auto value:raw)assert(value==0xcc);refusals++;
  }
  assert(!b.PutMutableWord(span.route,span.address+1,0));refusals++;
 }
 // No learned/KV/unselected workspace range may gain a mutable/zero fallback.
 for(unsigned i=0;i<canonical->Count();i++){const auto& p=*canonical->Part(i);
  if(p.kind!=canonical->WeightKind && p.kind!=canonical->KvKind && !(p.kind==canonical->WorkspaceKind && p.bank))continue;
  uint8 raw[4]={1,2,3,4};assert(!b.ReadMutableBytes(PioneerQ38ProjectionRoute(p.bank),canonical->Base+p.offset,raw,4)
   && !b.PutMutableWord(PioneerQ38ProjectionRoute(p.bank),canonical->Base+p.offset,0) && raw[0]==1 && raw[3]==4);refusals+=2;
 }
 assert(aliases==2018 && b.mutableWriteWords[0]==1344 && b.mutableWriteWords[1]==672 && b.mutableWriteWords[2]==2);
 printf("PASS native TEST mutable byte views/1009 canonical spans/2018 direct actual-vector aliases/little-endian reads+raw writes/%u unaligned+overflow+cross-span+null+zero+learned+KV+unselected-workspace refusals/nonzero inputs never replaced with zero;COMPONENT ONLY,NOT native init/state advancement/layer/token/physical/Work/TPS\n",refusals);
}
