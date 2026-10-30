/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_full_end_role.h"
#if !defined(PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE != 1 \
 || !defined(PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE) || PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE != 1
#error Explicit native state snapshot TEST required
#endif
#include "pioneer_q38_native_state_test_bridge.h"
static const char* tokenNativeStateSnapshotDirectory=nullptr;
// Read the SAME physical TEST arrays populated by actual ROM/instruction
// writes. Normalize bank stripes into logical order. No endpoint calls,
// writes,counters,NoC,model math,CPU reference or expected products here.
static bool CaptureNativeStateSnapshots(const PioneerSessionIO& io,const PioneerQ38CanonicalModelPlan& plan,
 Q38NativeStateTest& sink,const char* directory){
 if(!plan.Ready() || plan.RecurrentCount()!=672 || io.recurrentWords.size()!=73728U*1024
  || io.historyWords.size()!=1680U*1024 || io.kvSpans.size()!=112 || io.kvWords.size()!=49152)return false;
 unsigned pages=0;
 for(unsigned i=0;i<672;i++){
  const auto& actual=io.recurrent[i];const auto& expected=plan.RecurrentRegions()[i];
  if(actual.layer!=expected.layer || actual.slot!=expected.slot || actual.firstHead!=expected.firstHead
   || actual.heads!=expected.heads || actual.bank!=expected.bank || actual.offset!=plan.Base+expected.offset
   || io.recurrentFirst[i]!=pages || !actual.heads || actual.firstHead>=48 || actual.heads>48-actual.firstHead)return false;
  pages+=actual.heads*16;
 }if(pages!=73728)return false;
 for(unsigned completed=2;completed<=3;completed++)for(unsigned layer=0;layer<64;layer++){
  if(layer%4==3)continue;
  std::vector<uint32> words(786432);uint64 seen=0;
  for(unsigned i=0;i<672;i++){
   const auto& r=io.recurrent[i];if(r.layer!=layer || r.slot!=(completed&1))continue;
   for(unsigned head=r.firstHead;head<r.firstHead+r.heads;head++){
    const uint64 bit=UINT64_C(1)<<head;if(seen&bit)return false;seen|=bit;
    const auto* begin=io.recurrentWords.data()+io.recurrentFirst[i]*1024+(head-r.firstHead)*16384;
    // Physical half-heads are [half][key][value64]. The independent CPU
    // reference retains [key][value128],NOT a linear pair of half arrays.
    for(unsigned half=0;half<2;half++)for(unsigned key=0;key<128;key++)for(unsigned value=0;value<64;value++)
     words[head*16384+key*128+half*64+value]=begin[half*8192+key*64+value];
   }
  }
  if(seen!=UINT64_C(0xffffffffffff) || !sink.Record(directory,completed,layer,0,words.data(),words.size()))return false;
 }
 for(unsigned layer=0;layer<64;layer++){
  const bool dense=layer%4==3;std::vector<uint32> first(dense?1536:30720),second(dense?1536:0);
  std::vector<uint8> seen(dense?8:10240);unsigned history=0,parts=0;
  for(unsigned i=0;i<plan.Count();i++){
   const auto& p=*plan.Part(i);const bool conv=p.kind==plan.ConvKind;
   const unsigned ordinal=history;if(conv)history++;
   if(p.layer!=layer || p.kind!=unsigned(dense?plan.KvKind:plan.ConvKind))continue;parts++;
   if(p.firstRow>=seen.size() || p.rows>seen.size()-p.firstRow)return false;
   if(!dense){
    if(ordinal>=336 || p.rowBytes!=12 || p.allocated!=20480 || p.bytes!=p.rows*12)return false;
    const auto& h=io.history[ordinal];if(h.bank!=p.bank || h.address!=plan.Base+p.offset || h.bytes!=p.allocated)return false;
    const auto* raw=io.historyWords.data()+ordinal*5120;
    for(unsigned row=0;row<p.rows;row++){
     if(seen[p.firstRow+row]++)return false;
     for(unsigned tap=0;tap<3;tap++)first[(p.firstRow+row)*3+tap]=raw[row*3+tap];
    }
    for(unsigned word=p.rows*3;word<5120;word++)if(raw[word])return false;
   }else{
    if(p.rowBytes!=8192U*512 || p.bytes!=p.rows*p.rowBytes || p.allocated!=p.bytes)return false;
    for(unsigned row=0;row<p.rows;row++){
     const unsigned logical=p.firstRow+row;if(seen[logical]++)return false;auto& out=logical<4?first:second;
     for(unsigned position=0;position<3;position++)for(unsigned word=0;word<128;word++){
      const unsigned address=plan.Base+p.offset+row*p.rowBytes+position*512+word*4;
      const auto found=io.kvWords.find(uint64(PioneerQ38ProjectionRoute(p.bank))<<32|address);
      if(found==io.kvWords.end())return false;out[position*512+(logical%4)*128+word]=found->second;
     }
    }
   }
  }
  if(history!=336 || parts!=7)return false;for(auto count:seen)if(count!=1)return false;
  if(!sink.Record(directory,3,layer,dense?2:1,first.data(),first.size()))return false;
  if(dense && !sink.Record(directory,3,layer,3,second.data(),second.size()))return false;
 }
 return true; // DATA captured,not a full-completion marker;Finish follows genuine END
}
