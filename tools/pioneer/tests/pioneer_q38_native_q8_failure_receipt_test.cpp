/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38NativeQ8FailureReceipt.h"
#include <cassert>
#include <cstdio>

namespace D=PioneerQ38NativeQ8FailureReceipt;
namespace F=PioneerQ38TokenFrontierControl;
namespace T=PioneerQ38TokenControl;
static F::Packet Frontier(unsigned check){
 F::Packet p;
 auto& r=p.reply;auto* v=r.detail;
 r.version=1;r.operation=F::Observe;r.status=-8;r.started=1;r.unsafe=1;
 v[0]=1;v[1]=1;v[2]=12;v[4]=check<6?0:63;v[5]=1;v[10]=D::Qfd1;v[11]=check;
 v[12]=check>=3?0x514f4b31:0;v[13]=check==5?0x51465431:0;
 v[14]=check>=7?12:0;v[15]=check<3?0:check<5?1:check<7?3:7;
 return p;
}
static T::Packet Terminal(unsigned check){
 T::Packet p={};p.request.magic=T::Magic;p.request.version=1;p.request.operation=T::Observe;
 auto& r=p.reply;auto* v=r.detail;
 r.version=1;r.operation=T::Observe;r.status=-8;r.started=1;r.unsafe=1;r.stage=9;
 v[0]=1;v[1]=1;v[2]=9;v[4]=check==9?30:0;v[5]=1;
 if(check)v[14]=D::Qnt1+check;
 v[15]=check==5?0x51465431:0;
 return p;
}
int main(){
 for(unsigned c=1;c<=11;c++){
  const auto p=Frontier(c);D::Receipt got;
  assert(!F::ReplyShape(p) && D::Read(p,p.request,got) && got.phase==12 && got.check==c);
  assert(got.word==p.reply.detail[4] && got.error==p.reply.detail[13]);
  assert(!D::Read(p,p.request,got)); // never replace the first failure
 }
 for(unsigned c=0;c<=9;c++){
  const auto p=Terminal(c);D::Receipt got;
  assert(D::Read(p,p.request,got) && got.phase==9 && got.check==c);
  assert(got.word==p.reply.detail[4] && got.error==p.reply.detail[15]);
 }
 for(unsigned i=0;i<13;i++){
  auto p=Frontier(5);const auto sent=p.request;D::Receipt got;
  switch(i){
   case 0:p.request.reserved[0]=1;break;
   case 1:p.reply.version=2;break;
   case 2:p.reply.ready=1;break;
   case 3:p.reply.status=0;break;
   case 4:p.reply.unsafe=0;break;
   case 5:p.reply.detail[3]=1;break;
   case 6:p.reply.detail[4]=64;break;
   case 7:p.reply.detail[7]=1;break;
   case 8:p.reply.detail[13]=0;break;
   case 9:p.reply.detail[14]=1;break;
   case 10:p.reply.detail[15]=7;break;
   case 11:p.reply.detail[10]=0;break;
   case 12:p.reply.detail[11]=12;break;
  }
  assert(!D::Read(p,sent,got) && !got.phase);
 }
 for(unsigned i=0;i<10;i++){
  auto p=Terminal(5);auto sent=p.request;D::Receipt got;
  switch(i){
   case 0:p.prompt[8191]=1;break;
   case 1:p.records[4095]=1;break;
   case 2:p.reply.stage=8;break;
   case 3:p.reply.ready=1;break;
   case 4:p.reply.complete=1;break;
   case 5:p.reply.detail[4]=64;break;
   case 6:p.reply.detail[14]=D::Qnt1+10;break;
   case 7:p.reply.detail[15]=0;break;
   case 8:p.reply.detail[8]=1;break;
   case 9:sent.operation=p.request.operation=T::Hold;break;
  }
  assert(!D::Read(p,sent,got) && !got.phase);
 }
 puts("PASS exact failure-only Q8 metadata: 21 matched replies, 23 refusals, no accelerator");
}
