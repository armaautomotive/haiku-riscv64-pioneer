/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_CONTROL_PACKET_H
#define PIONEER_Q38_TOKEN_CONTROL_PACKET_H
#include <stddef.h>
#include <stdint.h>
// Fixed DATA grammar shared by the private adapter and bounded validation.
// No ioctl number,
// support/Work advertisement, addresses, code, CPU products or ownership grants.
namespace PioneerQ38TokenControl {
enum {Magic=0x33544351,Version=1,Context=8192,Vocabulary=248320,Batch=64,RecordWords=64};
enum Operation {Start=1,Observe=2,Cancel=3,Hold=4,Read=5,Finish=6};
struct Request {
 uint32_t magic,version,operation,prompts,limit,stopEos,eos0,eos1,records,reserved[7];
}; // limit is TOTAL token positions,including the immutable initial prompt
struct Reply {
 uint32_t version,operation,started,ready,unsafe,complete,stage,records;
 int32_t status;uint32_t reserved[7],detail[16];
}; // ready is operation-specific;ONLY the actual Finish may set complete
struct Packet {Request request;uint32_t prompt[Context];Reply reply;uint32_t records[Batch*RecordWords];};
static_assert(sizeof(Request)==64 && sizeof(Reply)==128,"fixed pointer-free headers");
static_assert(offsetof(Packet,prompt)==64 && offsetof(Packet,reply)==32832
 && offsetof(Packet,records)==32960 && sizeof(Packet)==49344,"fixed HEAP-only transaction");
static inline bool RequestShape(const Packet& p){
 const auto& r=p.request;
 if(r.magic!=Magic || r.version!=Version || r.operation<Start || r.operation>Finish)return false;
 for(auto value:r.reserved)if(value)return false;
 if(r.operation==Start){
  if(!r.prompts || r.prompts>r.limit || !r.limit || r.limit>Context || r.stopEos>1
   || r.eos0>=Vocabulary || r.eos1>=Vocabulary || r.records)return false;
  for(unsigned i=0;i<r.prompts;i++)if(p.prompt[i]>=Vocabulary)return false;
 }else if(r.prompts || r.limit || r.stopEos || r.eos0 || r.eos1
  || (r.operation==Read?r.records>Batch:r.records!=0))return false;
 for(unsigned i=r.operation==Start?r.prompts:0;i<Context;i++)if(p.prompt[i])return false;
 const auto* bytes=reinterpret_cast<const unsigned char*>(&p.reply);
 for(size_t i=0;i<sizeof(p.reply);i++)if(bytes[i])return false;
 for(auto value:p.records)if(value)return false;
 return true;
} // shape ONLY. No source/bank/launch/terminal/drain/cleanup authority.
}
#endif
