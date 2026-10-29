/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native full-layer TEST endpoints required
#endif
// Members of the SAME native TEST session. Views refer directly to the
// arrays populated by its actual zero ROMs,not a second zero/default fabric.
struct MutableByteSpan {unsigned route,address,bytes,kind;uint32* words;};
std::map<uint64,MutableByteSpan> mutableBytes;
uint64 mutableReadWords[3]={},mutableWriteWords[3]={};
static uint64 ByteKey(unsigned route,unsigned address){return uint64(route)<<32|address;}
void AddMutableBytes(unsigned route,unsigned address,unsigned bytes,unsigned kind,uint32* words){
 assert(bytes && !((address|bytes)&3) && uint64(address)+bytes<=UINT64_C(0x100000000) && kind<3 && words);
 const uint64 key=ByteKey(route,address);auto next=mutableBytes.lower_bound(key);
 if(next!=mutableBytes.end() && next->second.route==route)assert(uint64(address)+bytes<=next->second.address);
 if(next!=mutableBytes.begin()){const auto& previous=std::prev(next)->second;
  if(previous.route==route)assert(uint64(previous.address)+previous.bytes<=address);}
 assert(mutableBytes.emplace(key,MutableByteSpan{route,address,bytes,kind,words}).second);
}
void BindMutableBytes(){
 assert(mutableBytes.empty() && !rootStarts);
 for(unsigned i=0;i<672;i++){const auto& r=recurrent[i];AddMutableBytes(PioneerQ38ProjectionRoute(r.bank),r.offset,
  r.heads*65536,0,recurrentWords.data()+recurrentFirst[i]*1024);}
 for(unsigned i=0;i<336;i++){const auto& h=history[i];AddMutableBytes(PioneerQ38ProjectionRoute(h.bank),h.address,h.bytes,1,historyWords.data()+i*5120);}
 AddMutableBytes(PioneerQ38ProjectionRoute(0),workspaceAddress,67108864,2,runtimeDram.data());
 assert(mutableBytes.size()==1009);
}
MutableByteSpan* MutableSpan(unsigned route,unsigned address,unsigned bytes){
 if(!bytes || (address|bytes)&3 || uint64(address)+bytes>UINT64_C(0x100000000))return nullptr;
 auto next=mutableBytes.upper_bound(ByteKey(route,address));if(next==mutableBytes.begin())return nullptr;
 auto& span=std::prev(next)->second;
 return span.route==route && address>=span.address && bytes<=span.bytes && address-span.address<=span.bytes-bytes?&span:nullptr;
}
bool ReadMutableBytes(unsigned route,unsigned address,uint8* out,unsigned bytes){
 auto* span=MutableSpan(route,address,bytes);if(!span || !out)return false;
 const auto* words=span->words+(address-span->address)/4;
 for(unsigned i=0;i<bytes/4;i++)for(unsigned b=0;b<4;b++)out[i*4+b]=uint8(words[i]>>(b*8));
 mutableReadWords[span->kind]+=bytes/4;return true;
}
bool PutMutableWord(unsigned route,unsigned address,uint32 value){
 auto* span=MutableSpan(route,address,4);if(!span)return false;
 span->words[(address-span->address)/4]=value;mutableWriteWords[span->kind]++;return true;
} // only actual device packet callbacks call these writes;NO CPU model products
