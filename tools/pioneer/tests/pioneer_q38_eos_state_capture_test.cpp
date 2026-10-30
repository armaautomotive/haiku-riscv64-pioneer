/* SPDX-License-Identifier: MIT */
// SYNTHETIC physical-coordinate DATA,not executed model/device state.
#define PIONEER_Q38_TEST_OBSERVED_TRACE 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "pioneer_q38_eos_state_capture.h"
#include <memory>
struct CapturePlanView {
 const PioneerQ38CanonicalModelPlan& base;std::array<PioneerQ38GdnRegion,672> regions;
 bool valid=true;unsigned count=672;
 static constexpr uint64_t Capacity=PioneerQ38CanonicalModelPlan::Capacity;
 static constexpr unsigned Base=PioneerQ38CanonicalModelPlan::Base;
 enum {ConvKind=PioneerQ38CanonicalModelPlan::ConvKind,KvKind=PioneerQ38CanonicalModelPlan::KvKind};
 explicit CapturePlanView(const PioneerQ38CanonicalModelPlan& p):base(p){std::copy(p.RecurrentRegions(),p.RecurrentRegions()+672,regions.begin());}
 bool Ready()const{return valid && base.Ready();}unsigned RecurrentCount()const{return count;}
 const auto* RecurrentRegions()const{return regions.data();}unsigned Count()const{return base.Count();}const auto* Part(unsigned i)const{return base.Part(i);}
};
static uint32_t Pattern(unsigned layer,unsigned kind,unsigned logical){
 if(kind<2)return 0x3f000000U|((layer*271+kind*65537+logical*17)&0x7fffff);
 return (0x3000U|((layer*13+kind*67+logical*7)&1023))|((0x3400U|((layer*19+kind*73+logical*11)&1023))<<16);
}
struct RawPatterns {
 struct Span{unsigned route,base,words,layer,kind,first;bool gdn;};
 std::map<uint64_t,Span> spans;const Span* cached=nullptr;unsigned reads=0,fail=UINT32_MAX;bool nonfinite=false;
 static uint64_t Key(unsigned route,unsigned address){return uint64_t(route)<<32|address;}
 void Add(unsigned bank,unsigned address,unsigned words,unsigned layer,unsigned kind,unsigned first,bool gdn=false){
  const unsigned route=PioneerQ38ProjectionRoute(bank);assert(spans.emplace(Key(route,address),Span{route,address,words,layer,kind,first,gdn}).second);
 }
 bool Read(unsigned route,unsigned address,uint32_t& word){
  const unsigned request=reads++;if(request==fail)return false;
  if(!cached || cached->route!=route || address<cached->base || address-cached->base>=cached->words*4){
   auto found=spans.upper_bound(Key(route,address));assert(found!=spans.begin());cached=&(--found)->second;
  }
  const auto& s=*cached;assert(route==s.route && address>=s.base && address-s.base<s.words*4 && !(address&3));
  const unsigned raw=(address-s.base)/4;
  // Decode the physical byte coordinate independently into logical CPU
  // key/value coordinates. No capture output or common swizzle helper used.
  const unsigned logical=s.gdn?(s.first+raw/16384)*16384+((raw%8192)/64)*128+(raw%16384/8192)*64+raw%64:s.first+raw;
  word=nonfinite && !request?0x7f800000:Pattern(s.layer,s.kind,logical);return true;
 }
};
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());RawPatterns raw;
 for(unsigned i=0;i<672;i++){const auto& r=plan->RecurrentRegions()[i];if(!r.slot)raw.Add(r.bank,plan->Base+r.offset,r.heads*16384,r.layer,0,r.firstHead,true);}
 for(unsigned i=0;i<plan->Count();i++){
  const auto& p=*plan->Part(i);
  if(p.kind==plan->ConvKind)raw.Add(p.bank,plan->Base+p.offset,p.rows*3,p.layer,1,p.firstRow*3);
  else if(p.kind==plan->KvKind)for(unsigned row=0;row<p.rows;row++)for(unsigned pos=0;pos<2;pos++){
   const unsigned logical=p.firstRow+row;raw.Add(p.bank,plan->Base+p.offset+row*p.rowBytes+pos*512,128,p.layer,logical<4?2:3,pos*512+(logical%4)*128);
  }
 }
 unsigned records=0;uint64_t seenWords=0;
 assert(CaptureQ38EosStateData(*plan,[&](unsigned r,unsigned a,uint32_t& w){return raw.Read(r,a,w);},
  [&](unsigned layer,unsigned kind,const uint32_t* words,unsigned count){
   assert(count==(kind==0?786432U:kind==1?30720U:1024U));for(unsigned i=0;i<count;i++)assert(words[i]==Pattern(layer,kind,i));records++;seenWords+=count;return true;
  }));
 assert(records==128 && seenWords==39256064 && raw.reads==seenWords);
 unsigned refused=0;
 for(unsigned fault=0;fault<8;fault++){
  CapturePlanView bad(*plan);if(fault==0)bad.valid=false;if(fault==1)bad.count--;
  if(fault==2)bad.regions[0].layer^=1;if(fault==3)bad.regions[0].slot^=1;if(fault==4)bad.regions[0].firstHead^=1;
  if(fault==5)bad.regions[0].heads^=1;if(fault==6)bad.regions[0].bank=7;if(fault==7)bad.regions[0].offset=UINT32_MAX;
  raw.reads=0;unsigned sinks=0;
  assert(!CaptureQ38EosStateData(bad,[&](unsigned r,unsigned a,uint32_t& w){return raw.Read(r,a,w);},[&](unsigned,unsigned,const uint32_t*,unsigned){sinks++;return true;}) && !raw.reads && !sinks);refused++;
 }
 for(unsigned fail:{0U,8192U}){
  raw.reads=0;raw.fail=fail;unsigned sinks=0;
  assert(!CaptureQ38EosStateData(*plan,[&](unsigned r,unsigned a,uint32_t& w){return raw.Read(r,a,w);},[&](unsigned,unsigned,const uint32_t*,unsigned){sinks++;return true;}) && raw.reads==fail+1 && !sinks);refused++;
 }
 raw.reads=0;raw.fail=UINT32_MAX;unsigned sinks=0;
 assert(!CaptureQ38EosStateData(*plan,[&](unsigned r,unsigned a,uint32_t& w){return raw.Read(r,a,w);},[&](unsigned,unsigned,const uint32_t*,unsigned){sinks++;return false;}) && sinks==1 && raw.reads==786432);refused++;
 raw.reads=0;raw.nonfinite=true;sinks=0;
 assert(!CaptureQ38EosStateData(*plan,[&](unsigned r,unsigned a,uint32_t& w){return raw.Read(r,a,w);},[&](unsigned,unsigned,const uint32_t*,unsigned){sinks++;return true;}) && !sinks && raw.reads==786432);refused++;
 printf("PASS SYNTHETIC EOS state capture/128 records/39256064 physical-coordinate pattern words/key128-value128 vs half-key128-value64 normalization/all48GDN+Conv+16packedKV2positions/%u geometry-before-read,read-failure,sink-failure,nonfinite refusals/NO third position read;NO model,EOS,STOP,native END,Work or TPS proof\n",refused);
}
