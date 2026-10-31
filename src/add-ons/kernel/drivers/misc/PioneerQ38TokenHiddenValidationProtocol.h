/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_PROTOCOL_H
#include "PioneerQ38TokenControlPacket.h"
// Separate tag75 diagnostic DATA grammar. Not a driver/Work activation.
// Neither request contains a bank,address,user pointer,CPU product or ready hint.
#define TT_PCI_PROBE_Q38_TOKEN_HIDDEN_CONTROL (B_DEVICE_OP_CODES_END+0x6604)
#define TT_PCI_PROBE_Q38_TOKEN_HIDDEN_READ (B_DEVICE_OP_CODES_END+0x6605)
namespace PioneerQ38TokenHiddenValidation {
static constexpr uint64_t BuildTag=UINT64_C(0x2026103100010075);
enum {Magic=0x35484351,Version=1,Prompts=2,Positions=3,FirstToken=248319,SecondToken=17,
 Parts=2,Words=5120,ChunkWords=4096};
struct Request {uint32_t magic,version,part,words,reserved[12];};
struct Reply {
 uint32_t version,part,started,ready,unsafe,words;int32_t status;
 uint32_t reserved[9],detail[16];
};
struct Packet {Request request;Reply reply;uint32_t words[ChunkWords];};
static_assert(sizeof(Request)==64 && sizeof(Reply)==128,"fixed pointer-free diagnostic headers");
static_assert(offsetof(Packet,reply)==64 && offsetof(Packet,words)==192 && sizeof(Packet)==16576,
 "fixed HEAP-only diagnostic transaction");
static inline unsigned PartWords(unsigned part){return part<Parts?(part?1024U:4096U):0U;}
static inline bool RequestShape(const Packet& p){
 const auto& r=p.request;
 if(r.magic!=Magic || r.version!=Version || r.part>=Parts || r.words!=PartWords(r.part))return false;
 for(auto value:r.reserved)if(value)return false;
 const auto* bytes=reinterpret_cast<const unsigned char*>(&p.reply);
 for(size_t i=0;i<sizeof(p.reply);i++)if(bytes[i])return false;
 for(auto value:p.words)if(value)return false;
 return true;
} // DATA shape only;the actual owned/HOLD/drain/cursor checks remain mandatory.
static inline bool ControlRequestShape(const PioneerQ38TokenControl::Packet& p){
 namespace P=PioneerQ38TokenControl;
 if(!P::RequestShape(p))return false;
 const auto& r=p.request;
 if(r.operation==P::Start)return r.prompts==Prompts && r.limit==Positions && !r.stopEos
  && !r.eos0 && !r.eos1 && p.prompt[0]==FirstToken && p.prompt[1]==SecondToken;
 return r.operation!=P::Read || r.records<=Positions;
} // Same fixed three-position diagnostic;NO arbitrary generation/Work route.
}
#endif
