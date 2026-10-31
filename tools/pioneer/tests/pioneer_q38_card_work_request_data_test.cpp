/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38CardWorkRequestData.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenValidationProtocol.h"
#include "../../../src/apps/assistant/Qwen27Protocol.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
namespace D=PioneerQ38CardWorkRequestData;
namespace P=PioneerQ38TokenControl;
static_assert(unsigned(D::MaxPrompts)==unsigned(Assistant27::MaxPromptTokens)
 && unsigned(D::MaxOutputs)==unsigned(Assistant27::MaxTokens),"match actual Assistant request bounds");

// Independent literal DATA grammar;not a native/session/completion oracle.
static bool Reference(const P::Packet& p){
 const auto& r=p.request;
 if(r.magic!=0x33544351 || r.version!=1 || r.operation!=1 || r.prompts<1 || r.prompts>96
  || r.limit<r.prompts || r.limit>127 || r.limit-r.prompts>31 || r.stopEos!=1
  || r.eos0>=248320 || r.eos1>=248320 || r.records)return false;
 for(auto v:r.reserved)if(v)return false;
 for(unsigned i=0;i<8192;i++)if(i<r.prompts?p.prompt[i]>=248320:p.prompt[i]!=0)return false;
 const auto* reply=reinterpret_cast<const unsigned char*>(&p.reply);
 for(unsigned i=0;i<128;i++)if(reply[i])return false;
 for(auto v:p.records)if(v)return false;
 return true;
}
int main(){
 auto packet=std::make_unique<P::Packet>();auto previous=std::make_unique<P::Packet>();
 std::array<uint32_t,96> input={};unsigned positives=0,refusals=0,mutations=0,mappings=0;
 auto clear=[&](){std::memset(packet.get(),0,sizeof(*packet));};
 auto refuse=[&](const uint32_t* prompt,unsigned m,unsigned n,unsigned a=0,unsigned b=248319){
  std::memcpy(previous.get(),packet.get(),sizeof(*packet));
  assert(!D::Build(*packet,prompt,m,n,a,b));
  assert(!std::memcmp(previous.get(),packet.get(),sizeof(*packet)));refusals++;
 };
 for(unsigned m=1;m<=96;m++)for(unsigned n=1;n<=32;n++){
  clear();for(unsigned i=0;i<96;i++)input[i]=(i*7919+m*n)%248320;
  input[0]=0;input[m-1]=248319;
  const unsigned eos1=(m+n)%2?248319:0; // a model with one EOS may repeat it
  assert(D::Build(*packet,input.data(),m,n,0,eos1));
  assert(Reference(*packet) && D::Shape(*packet) && P::RequestShape(*packet));
  assert(!PioneerQ38TokenValidation::RequestShape(*packet)); // never alias tag74
  assert(packet->request.limit==m+n-1 && packet->request.limit-m==n-1);
  for(unsigned i=0;i<m;i++)assert(packet->prompt[i]==input[i]);
  input.fill(123); // subsequent caller changes cannot change the copied prompt
  assert(packet->prompt[m-1]==248319);
  unsigned found=0;
  for(unsigned position=0;position<=128;position++){
   unsigned ordinal=0xdeadbeef;
   const bool got=D::OutputOrdinalData(packet->request,position,ordinal);
   const bool expected=position>=m-1 && position<m+n-1;
   assert(got==expected);
   if(got){assert(ordinal==found);found++;}else assert(ordinal==0xdeadbeef);
   mappings++;
  }
  assert(found==n);positives++;
 }
 clear();input.fill(0);
 refuse(nullptr,1,1);
 for(unsigned m:{0U,97U,UINT32_MAX})refuse(input.data(),m,1);
 for(unsigned n:{0U,33U,UINT32_MAX})refuse(input.data(),1,n);
 for(unsigned bad:{248320U,UINT32_MAX}){
  refuse(input.data(),1,1,bad,0);refuse(input.data(),1,1,0,bad);
  for(unsigned m:{1U,2U,96U}){input[m-1]=bad;refuse(input.data(),m,32);input[m-1]=0;}
 }
 refuse(packet->prompt,1,1);refuse(packet->records,1,1);
 refuse(reinterpret_cast<const uint32_t*>(&packet->reply),1,1);
 refuse(reinterpret_cast<const uint32_t*>(reinterpret_cast<const unsigned char*>(input.data())+1),1,1);
 refuse(reinterpret_cast<const uint32_t*>(UINTPTR_MAX&~uintptr_t(3)),96,1);
 // Every output word,including unused prompt/reply/record storage,must be
 // pristine. Reject before touching it;preserve a caller's nonzero bytes.
 auto* bytes=reinterpret_cast<unsigned char*>(packet.get());
 for(unsigned i=0;i<sizeof(*packet)/4;i++){
  bytes[i*4]=1;refuse(input.data(),1,1);bytes[i*4]=0;
 }
 clear();input.fill(17);assert(D::Build(*packet,input.data(),96,32,248318,248319));
 auto flip=[](void* object,unsigned word,unsigned bit){
  auto* raw=reinterpret_cast<unsigned char*>(object)+word*4;uint32_t value=0;
  std::memcpy(&value,raw,4);value^=UINT32_C(1)<<bit;std::memcpy(raw,&value,4);
 };
 for(unsigned i=0;i<16;i++)for(unsigned bit=0;bit<32;bit++){
  flip(&packet->request,i,bit);assert(D::Shape(*packet)==Reference(*packet));
  unsigned ordinal=0xabcdef01;
  if(!D::Header(packet->request))assert(!D::OutputOrdinalData(packet->request,95,ordinal) && ordinal==0xabcdef01);
  flip(&packet->request,i,bit);mutations++;
 }
 for(unsigned i:{0U,1U,95U,96U,8191U})for(unsigned bit=0;bit<32;bit++){
  packet->prompt[i]^=UINT32_C(1)<<bit;assert(D::Shape(*packet)==Reference(*packet));
  packet->prompt[i]^=UINT32_C(1)<<bit;mutations++;
 }
 for(unsigned i=0;i<32;i++)for(unsigned bit=0;bit<32;bit++){
  flip(&packet->reply,i,bit);assert(!D::Shape(*packet) && !Reference(*packet));
  flip(&packet->reply,i,bit);mutations++;
 }
 for(unsigned i:{0U,1U,2048U,4095U})for(unsigned bit=0;bit<32;bit++){
  packet->records[i]^=UINT32_C(1)<<bit;assert(!D::Shape(*packet) && !Reference(*packet));
  packet->records[i]^=UINT32_C(1)<<bit;mutations++;
 }
 // Conversely the exact old diagnostic cannot satisfy the new Work DATA
 // grammar. There is no route activation in either grammar comparison.
 clear();auto& r=packet->request;r.magic=P::Magic;r.version=1;r.operation=P::Start;
 r.prompts=2;r.limit=3;packet->prompt[0]=248319;packet->prompt[1]=17;
 assert(PioneerQ38TokenValidation::RequestShape(*packet) && !D::Shape(*packet));
 unsigned ordinal=123;assert(!D::OutputOrdinalData(r,1,ordinal) && ordinal==123);
 printf("PASS %u bounded request DATA builds/%u refusal-preserves-output cases/%u independent grammar mutations/%u position mappings;tag74 disjoint/both directions;NO ioctl,driver,execution,record-delivery,Work activation,numerics or TPS qualification\n",positives,refusals,mutations,mappings);
}
