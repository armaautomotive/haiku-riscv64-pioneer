/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native full-flow KV TEST endpoints required
#endif
// Empty physical KV byte endpoint. Geometry reserves addresses but supplies
// NO zero/cache row/product. Only actual device writes create readable words.
struct KvByteSpan {unsigned route,address,bytes;};
std::map<uint64,KvByteSpan> kvSpans;
std::map<uint64,uint32> kvWords;
uint64 kvReadWords=0,kvWriteWords=0;
static uint64 KvKey(unsigned route,unsigned address){return uint64(route)<<32|address;}
void BindKvBytes(const PioneerQ38CanonicalModelPlan& canonical){
 assert(canonical.Ready() && kvSpans.empty() && kvWords.empty());uint64 bytes=0;
 for(unsigned i=0;i<canonical.Count();i++){const auto& p=*canonical.Part(i);if(p.kind!=canonical.KvKind)continue;
  const unsigned route=PioneerQ38ProjectionRoute(p.bank),address=canonical.Base+p.offset;
  auto next=kvSpans.lower_bound(KvKey(route,address));
  if(next!=kvSpans.end() && next->second.route==route)assert(uint64(address)+p.allocated<=next->second.address);
  if(next!=kvSpans.begin()){const auto& previous=std::prev(next)->second;
   if(previous.route==route)assert(uint64(previous.address)+previous.bytes<=address);}
  assert(p.bytes==p.allocated && kvSpans.emplace(KvKey(route,address),KvByteSpan{route,address,p.bytes}).second);bytes+=p.bytes;
 }
 assert(kvSpans.size()==112 && bytes==UINT64_C(536870912) && kvWords.empty());
}
const KvByteSpan* KvSpan(unsigned route,unsigned address,unsigned bytes)const{
 if(!bytes || (address|bytes)&3 || uint64(address)+bytes>UINT64_C(0x100000000))return nullptr;
 auto next=kvSpans.upper_bound(KvKey(route,address));if(next==kvSpans.begin())return nullptr;
 const auto& span=std::prev(next)->second;
 return span.route==route && address>=span.address && bytes<=span.bytes && address-span.address<=span.bytes-bytes?&span:nullptr;
}
bool ReadKvBytes(unsigned route,unsigned address,uint8* out,unsigned bytes){
 if(!out || !KvSpan(route,address,bytes))return false;
 for(unsigned at=0;at<bytes;at+=4)if(kvWords.find(KvKey(route,address+at))==kvWords.end())return false;
 for(unsigned at=0;at<bytes;at+=4){const uint32 word=kvWords.at(KvKey(route,address+at));
  for(unsigned b=0;b<4;b++)out[at+b]=uint8(word>>(b*8));}
 kvReadWords+=bytes/4;return true;
} // a missing word refuses the WHOLE read before changing output--never inserts zero
bool PutKvWord(unsigned route,unsigned address,uint32 value){
 if(!KvSpan(route,address,4))return false;
 if(!kvWords.emplace(KvKey(route,address),value).second)return false;
 kvWriteWords++;return true;
 // Prior/current appended words are immutable,including an identical
 // rewrite. A failed transfer may not be retried or silently recaptured.
}
