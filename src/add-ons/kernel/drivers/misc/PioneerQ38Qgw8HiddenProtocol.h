/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGW8_HIDDEN_PROTOCOL_H
#define PIONEER_Q38_QGW8_HIDDEN_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>
#include "PioneerQ38TokenControlPacket.h"

// Isolated tag129 diagnostic DATA only. No arbitrary address, bank, CPU
// product, completion hint, accelerator ownership, or Work-mode grant.
#define TT_PCI_PROBE_Q38_QGW8_HIDDEN_READ (B_DEVICE_OP_CODES_END+0x8832)
namespace PioneerQ38Qgw8Hidden {
static constexpr uint64_t BuildTag=UINT64_C(0x2026110700010129);
enum : uint32_t {Magic=0x39484351,Version=1,Input=248319,Layers=64,Position=0,
 Committed=1,TerminalKind=1,Parts=2,Words=5120,ChunkWords=4096};
struct Request {uint32_t magic,version,part,words,reserved[12];};
struct Reply {uint32_t version,part,started,ready,unsafe,words;int32_t status;uint32_t reserved[9],detail[16];};
struct Packet {Request request;Reply reply;uint32_t words[ChunkWords];};
static_assert(sizeof(Request)==64 && sizeof(Reply)==128 && offsetof(Packet,words)==192 && sizeof(Packet)==16576,
 "fixed pointer-free HEAP-only diagnostic transaction");
inline unsigned PartWords(unsigned part){return part<Parts?(part?1024U:4096U):0U;}
inline bool ControlRequestShape(const PioneerQ38TokenControl::Packet& p){
 namespace C=PioneerQ38TokenControl;
 if(!C::RequestShape(p))return false;
 const auto& r=p.request;
 if(r.operation==C::Start)return r.prompts==1 && r.limit==1 && p.prompt[0]==Input;
 if(r.operation==C::Cancel)return false;
 return r.operation!=C::Read || r.records==Committed;
}
inline bool RequestFields(const Request& r){
 if(r.magic!=Magic || r.version!=Version || r.part>=Parts || r.words!=PartWords(r.part))return false;
 for(auto value:r.reserved)if(value)return false;return true;
}
inline bool RequestShape(const Packet& p){
 if(!RequestFields(p.request))return false;
 const auto* bytes=reinterpret_cast<const uint8_t*>(&p.reply);
 for(size_t i=0;i<sizeof(p.reply);i++)if(bytes[i])return false;
 for(auto word:p.words)if(word)return false;return true;
}
// This checks returned DATA shape only; the driver must prove its real
// session, terminal, cohort hold, drained record, cursor and owned mappings.
inline bool ReplyShape(const Packet& p){
 if(!RequestFields(p.request))return false;const auto& r=p.reply;
 if(r.version!=Version || r.part!=p.request.part || r.started!=1 || r.ready!=1
  || r.unsafe || r.status || r.words!=p.request.words)return false;
 for(auto word:r.reserved)if(word)return false;
 const uint32_t expected[]={1,1,4,0,0,0,1,p.request.part,p.request.part*ChunkWords,p.request.words,
  Position,Layers,TerminalKind,Committed,1,p.request.words*4};
 for(unsigned i=0;i<16;i++)if(r.detail[i]!=expected[i])return false;
 for(unsigned i=p.request.words;i<ChunkWords;i++)if(p.words[i])return false;
 return true; // raw IEEE bits, including nonfinite DATA, are not a quality pass
}
}
#endif
