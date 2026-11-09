/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_QGW8_TRACE_ASSEMBLER_INSPECTION 1
#define PIONEER_Q38_QGW8_TRACE_PACKET_INSPECTION 1
#include "../PioneerQ38Qgw8LayerTraceHandoff.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
namespace P=PioneerQ38Qgw8LayerTracePacket;namespace G=PioneerQ38Qgw8LayerTraceGeometry;
using H=PioneerQ38Qgw8LayerTraceHandoff;
static constexpr uint64_t Key=UINT64_C(0x123456789abcdef0);
static void Reply(P::Packet& p){
 const auto& q=p.request;auto& r=p.reply;r.version=1;r.ordinal=q.ordinal;r.started=r.ready=1;r.words=q.words;
 const uint32_t details[]={1,1,4,0,0,0,1,q.ordinal,q.layer*5120+q.part*4096,q.words,0,64,1,1,1,q.words*4};
 for(unsigned i=0;i<16;i++)r.detail[i]=details[i];
 for(unsigned i=0;i<q.words;i++)p.words[i]=q.layer*5120+q.part*4096+i;
 if(!q.ordinal)p.words[0]=0x7fc12345; // raw NaN is DATA, not a quality pass
}
int main(){
 constexpr size_t Total=64U*5120U;auto h=std::make_unique<H>(Key,0x04000000);
 auto p=std::make_unique<P::Packet>();std::vector<uint32_t> out(Total,0xdeadbeef);
 assert(!h->Copy(out.data(),out.size()) && out[0]==0xdeadbeef);
 for(unsigned ordinal=0;ordinal<G::Chunks;ordinal++){
  *p={};assert(h->Prepare(*p) && P::RequestShape(*p));Reply(*p);assert(P::ReplyShape(*p));
  assert(h->Accept(*p) && h->Cursor()==ordinal+1);
 }
 assert(!h->Copy(reinterpret_cast<uint32_t*>(h.get()),out.size()));
 assert(h->Copy(out.data(),out.size()) && out[0]==0x7fc12345);
 for(unsigned i=1;i<Total;i++)assert(out[i]==i);
 assert(!h->Copy(out.data(),out.size()));
 unsigned refused=0;
 for(unsigned section=0;section<2;section++)for(unsigned bit=0;bit<(section?sizeof(P::Reply):sizeof(P::Request))*8;bit++){
  auto a=std::make_unique<H>(Key,0x04000000);*p={};assert(a->Prepare(*p));Reply(*p);
  auto* bytes=section?reinterpret_cast<uint8_t*>(&p->reply):reinterpret_cast<uint8_t*>(&p->request);
  bytes[bit/8]^=uint8_t(1U<<(bit%8));assert(!a->Accept(*p) && a->Failed());
  out.assign(Total,0xdeadbeef);assert(!a->Copy(out.data(),out.size()) && out[0]==0xdeadbeef);refused++;
 }
 {
  auto a=std::make_unique<H>(Key,0x04000000);*p={};assert(a->Prepare(*p));Reply(*p);assert(a->Accept(*p));
  *p={};assert(a->Prepare(*p));Reply(*p);p->words[1024]=1;
  assert(!a->Accept(*p) && a->Failed());refused++;
 }
 {
  auto a=std::make_unique<H>(Key,0x04000000);*p={};assert(a->Prepare(*p));auto other=std::make_unique<P::Packet>();
  assert(!a->Prepare(*other) && a->Failed());refused++;
 }
 {
  auto a=std::make_unique<H>(Key,0x04000000);*p={};assert(!a->Accept(*p) && a->Failed());refused++;
 }
 {
  auto a=std::make_unique<H>(Key,0x04000000);*p={};p->words[0]=1;assert(!a->Prepare(*p) && a->Failed());refused++;
 }
 printf("PASS 128 ordered synthetic packets, raw NaN, %u request/reply/tail/lifecycle refusals; NO SOURCE PROVENANCE OR DEVICE\n",refused);
}
