/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_PHASE_PROFILE_PACKET_H
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_PACKET_H
#include <stdint.h>
#include <stddef.h>
#include "../../../../../tools/pioneer/PioneerQ38TokenPhaseProfileData.h"
// Distinct fixed held-read DATA. No address/count/generation or action input.
#define TT_PCI_PROBE_Q38_TOKEN_PHASE_PROFILE_READ (B_DEVICE_OP_CODES_END+0x6609)
namespace PioneerQ38TokenPhaseProfileControl {
enum {Version=1,Read=1,DataWords=60,PacketWords=64};
struct Request {uint32_t version=Version,operation=Read,reserved[6]={};};
struct Reply {
 uint32_t version=0,operation=0;int32_t status=0;
 uint32_t started=0,ready=0,unsafe=0,reserved[2]={},detail[16]={};
};
struct Packet {Request request;Reply reply;uint32_t words[PacketWords]={};};
static_assert(sizeof(Request)==32 && sizeof(Reply)==96 && sizeof(Packet)==384
 && offsetof(Packet,words)==128,"separate fixed packet,not token reserved fields");
static inline bool RequestShape(const Packet& p){
 if(p.request.version!=Version || p.request.operation!=Read)return false;
 for(auto value:p.request.reserved)if(value)return false;
 const auto* bytes=reinterpret_cast<const uint8_t*>(&p.reply);
 for(unsigned i=0;i<sizeof(p.reply);i++)if(bytes[i])return false;
 for(auto value:p.words)if(value)return false;
 return true;
}
static inline bool ReplyShape(const Packet& p){
 const auto& r=p.reply;const auto* d=r.detail;
 if(p.request.version!=Version || p.request.operation!=Read || r.version!=Version || r.operation!=Read
  || r.started>1 || r.ready>1 || r.unsafe>1 || r.reserved[0] || r.reserved[1]
  || d[0]!=1 || d[1]!=r.started || d[5]!=r.unsafe || d[6]!=r.ready || d[7]>1 || d[3]>119
  || d[4]>63 || d[14] || d[15])return false;
 for(auto value:p.request.reserved)if(value)return false;
 for(unsigned i=DataWords;i<PacketWords;i++)if(p.words[i])return false;
 if(r.ready){
  namespace Q=PioneerQ38TokenPhaseProfileData;
  const bool valid=Q::Closed(p.words) && p.words[Q::Committed]==d[8];
  return !r.status && r.started && !r.unsafe && !d[3] && d[2]==4 && d[4]==59 && d[8]<=8192 && d[9]==DataWords
   && d[10]==p.words[Q::State] && d[11]==p.words[Q::Error] && d[7]==unsigned(valid) && !d[12] && !d[13];
 }
 if(!r.status || d[7] || d[8] || d[9] || d[10] || d[11])return false;
 for(auto value:p.words)if(value)return false;
 return true;
} // consistency ONLY;invalid timing can still accompany a clean model END.
}
#endif
