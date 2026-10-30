/* SPDX-License-Identifier: MIT */
// HOST TEST DATA normalization ONLY. Read/sink callbacks confer no execution,
// source authentication,EOS,STOP,native ownership or cleanup permission.
#include "PioneerQ38ObservedTrace.h" // exact HOST role and late-mixing checks
#if defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)
#error EOS state capture is HOST DATA only,never native
#endif
#ifndef PIONEER_Q38_EOS_STATE_CAPTURE_H
#define PIONEER_Q38_EOS_STATE_CAPTURE_H
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
static inline bool Q38EosCaptureFinite(unsigned kind,const uint32_t* words,unsigned count){
 if(!words || kind>3)return false;
 for(unsigned i=0;i<count;i++)if(kind<2?(words[i]&0x7f800000)==0x7f800000:
  (words[i]&0x7c00)==0x7c00 || (words[i]&0x7c000000)==0x7c000000)return false;
 return true;
}
template<class Plan,class Read,class Sink>
bool CaptureQ38EosStateData(const Plan& plan,Read read,Sink sink){
 if(!plan.Ready() || plan.RecurrentCount()!=672 || !plan.RecurrentRegions())return false;
 // Validate ALL two-slot region geometry before the first byte observation.
 uint64_t coverage[48][2]={};
 for(unsigned i=0;i<672;i++){
  const auto& r=plan.RecurrentRegions()[i];
  if(r.layer>=64 || r.layer%4==3 || r.slot>=2 || r.bank>=7 || !r.heads || r.firstHead>=48 || r.heads>48-r.firstHead
   || uint64_t(r.offset)+uint64_t(r.heads)*65536>Plan::Capacity)return false;
  auto& seen=coverage[r.layer-r.layer/4][r.slot];
  for(unsigned head=r.firstHead;head<r.firstHead+r.heads;head++){
   const uint64_t bit=UINT64_C(1)<<head;if(seen&bit)return false;seen|=bit;
  }
 }
 for(const auto& slots:coverage)for(auto seen:slots)if(seen!=UINT64_C(0xffffffffffff))return false;
 unsigned records=0;
 for(unsigned layer=0;layer<64;layer++){
  if(layer%4==3)continue;
  std::vector<uint32_t> words(786432);
  for(unsigned i=0;i<672;i++){
   const auto& r=plan.RecurrentRegions()[i];if(r.layer!=layer || r.slot!=0)continue; // completed2 -> current slot0
   for(unsigned head=r.firstHead;head<r.firstHead+r.heads;head++)for(unsigned half=0;half<2;half++)
    for(unsigned key=0;key<128;key++)for(unsigned value=0;value<64;value++){
     const unsigned physical=(head-r.firstHead)*16384+half*8192+key*64+value;
     // Device:[head][half][key][value64];CPU:[head][key][value128].
     const unsigned logical=head*16384+key*128+half*64+value;
     if(!read(PioneerQ38ProjectionRoute(r.bank),Plan::Base+r.offset+physical*4,words[logical]))return false;
    }
  }
  if(!Q38EosCaptureFinite(0,words.data(),words.size()) || !plan.Ready() || !sink(layer,0,words.data(),words.size()))return false;
  records++;
 }
 for(unsigned layer=0;layer<64;layer++){
  const bool dense=layer%4==3;
  std::vector<uint32_t> first(dense?1024:30720),second(dense?1024:0);
  std::vector<uint8_t> seen(dense?8:10240);unsigned parts=0;
  for(unsigned i=0;i<plan.Count();i++){
   const auto* p=plan.Part(i);if(!p)return false;
   if(p->layer!=layer || p->kind!=unsigned(dense?Plan::KvKind:Plan::ConvKind))continue;
   if(++parts>7 || p->bank>=7 || !p->rows || p->firstRow>=seen.size() || p->rows>seen.size()-p->firstRow
    || uint64_t(p->offset)+p->allocated>Plan::Capacity)return false;
   if(dense){
    if(p->rowBytes!=8192U*512 || p->bytes!=p->rows*p->rowBytes || p->allocated!=p->bytes)return false;
   }else if(p->rowBytes!=12 || p->bytes!=p->rows*12 || p->allocated!=20480)return false;
   for(unsigned row=0;row<p->rows;row++){
    const unsigned logical=p->firstRow+row;if(seen[logical]++)return false;
    if(!dense){
     for(unsigned tap=0;tap<3;tap++)if(!read(PioneerQ38ProjectionRoute(p->bank),Plan::Base+p->offset+(row*3+tap)*4,first[logical*3+tap]))return false;
    }else{
     auto& output=logical<4?first:second;
     for(unsigned position=0;position<2;position++)for(unsigned word=0;word<128;word++){
      const unsigned address=Plan::Base+p->offset+row*p->rowBytes+position*512+word*4;
      if(!read(PioneerQ38ProjectionRoute(p->bank),address,output[position*512+(logical%4)*128+word]))return false;
     }
    }
   }
  }
  if(parts!=7 || !plan.Ready())return false;for(auto count:seen)if(count!=1)return false;
  if(!Q38EosCaptureFinite(dense?2:1,first.data(),first.size()) || !sink(layer,dense?2:1,first.data(),first.size()))return false;
  records++;
  if(dense){if(!Q38EosCaptureFinite(3,second.data(),second.size()) || !sink(layer,3,second.data(),second.size()))return false;records++;}
 }
 return records==128 && plan.Ready(); // DATA only;no completion marker or execution receipt
}
#endif
