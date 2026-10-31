/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_FRONTIER_PACKET_H
#define PIONEER_Q38_TOKEN_FRONTIER_PACKET_H
#include <stddef.h>
#include <stdint.h>
// Public DATA shape only. Not routed by any current physical driver.
#define TT_PCI_PROBE_Q38_TOKEN_FRONTIER_OBSERVE (B_DEVICE_OP_CODES_END+0x6608)
namespace PioneerQ38TokenFrontierControl {
enum {Version=1,Observe=1};
struct Request {uint32_t version=Version,operation=Observe,reserved[6]={};};
struct Reply {
 uint32_t version=0,operation=0;int32_t status=0;
 uint32_t started=0,ready=0,unsafe=0,reserved[2]={},detail[16]={};
};
struct Packet {Request request;Reply reply;};
static_assert(sizeof(Request)==32 && sizeof(Reply)==96 && sizeof(Packet)==128
 && offsetof(Packet,reply)==32,"fixed integer observation DATA,not the token-record ABI");
static inline bool RequestShape(const Packet& p){
 if(p.request.version!=Version || p.request.operation!=Observe)return false;
 for(auto value:p.request.reserved)if(value)return false;
 const auto* bytes=reinterpret_cast<const uint8_t*>(&p.reply);
 for(unsigned i=0;i<sizeof(p.reply);i++)if(bytes[i])return false;
 return true;
}
static inline bool ReplyShape(const Packet& p){
 const auto& r=p.reply;const auto* d=r.detail;
 if(p.request.version!=Version || p.request.operation!=Observe || r.version!=Version || r.operation!=Observe
  || r.started>1 || r.ready>1 || r.unsafe>1 || r.reserved[0] || r.reserved[1]
  || d[0]!=1 || d[1]!=r.started || d[5]!=r.unsafe || d[6]!=r.ready || d[3] || d[4]>63)return false;
 for(auto value:p.request.reserved)if(value)return false;
 for(unsigned i=10;i<16;i++)if(d[i])return false;
 if(r.ready)return !r.status && r.started && !r.unsafe && d[2]==12 && d[4]==63
  && d[8] && d[8]<=8192 && d[9] && d[9]<=d[8] && d[7]<=d[8];
 return r.status && !d[7] && !d[8] && !d[9] && (!d[2] || d[2]==12);
} // consistency only;NO source,session,terminal,HOLD,END or timing authority
}
#endif
