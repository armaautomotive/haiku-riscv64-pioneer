/* SPDX-License-Identifier: MIT */
#include "PioneerQ38FullOffloadPreparedQ4Role.h"
#ifndef PIONEER_Q38_PREPARED_SOURCE_PROTOCOL_H
#define PIONEER_Q38_PREPARED_SOURCE_PROTOCOL_H
#include <stdint.h>
#define TT_PCI_PROBE_Q38_PREPARED_SOURCE (B_DEVICE_OP_CODES_END+0x8805)
namespace PioneerQ38PreparedSource {
enum {Magic=0x31535051,Version=1,Begin=1,Phase=2,Seal=3,Parts=1344,Phases=192,Words=2400,BeginWords=Parts+16};
static constexpr uint32_t Identity[16]={0x03de00c6U,0x0e8aae00U,0xb8c0a6b3U,0x8b1b56b5U,0xd26bf196U,0xa6c263c8U,0x29de426cU,0x47a791d3U,
 0xd0c7c270U,0xc24f6cedU,0x0f964288U,0xa5e045a4U,0xcaba12afU,0xa331ac2eU,0xc5a66f32U,0xe6eb1b34U};
struct Request {uint32_t magic=Magic,version=Version,operation=0,index=0,words=0,reserved[3]={},data[Words]={};};
struct Reply {uint32_t version=Version,operation=0;int32_t status=0;uint32_t accepted=0,ready=0,parts=0,phases=0,reserved=0;};
struct Packet {Request request;Reply reply;};
static_assert(sizeof(Request)==9632 && sizeof(Reply)==32 && sizeof(Packet)==9664,"bounded pointer-free prepared source metadata");
static inline bool RequestShape(const Packet& p){
 const auto& q=p.request;
 if(q.magic!=Magic || q.version!=Version || q.operation<Begin || q.operation>Seal)return false;
 for(unsigned v:q.reserved)if(v)return false;
 const unsigned words=q.operation==Begin?BeginWords:q.operation==Phase?Words:0;
 if(q.words!=words || (q.operation==Begin?q.index!=0:q.operation==Phase?q.index>=Phases:q.index!=Phases))return false;
 for(unsigned i=words;i<Words;i++)if(q.data[i])return false;
 if(q.operation==Begin){
  for(unsigned i=0;i<16;i++)if(q.data[i]!=Identity[i])return false;
 }
 const Reply pristine;const auto* a=reinterpret_cast<const uint8_t*>(&p.reply);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(Reply);i++)if(a[i]!=b[i])return false;
 return true;
} // integer source descriptors only;no code,products,ready/completion or arbitrary NoC coordinates
}
#endif
