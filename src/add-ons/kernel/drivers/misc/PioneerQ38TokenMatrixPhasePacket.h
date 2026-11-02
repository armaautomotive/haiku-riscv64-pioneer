/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_MATRIX_PHASE_PACKET_H
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_PACKET_H
#include <stdint.h>
#include <stddef.h>
#include "../../../../../tools/pioneer/PioneerQ38TokenMatrixPhaseData.h"
// Pure fixed DATA grammar,not an enabled ioctl or native/USER/Work selector.
// The only input is worker1..119. The STOP nonce must come from the native
// owner's independently sealed terminal journal,NEVER from metric nonce+1.
#define TT_PCI_PROBE_Q38_TOKEN_MATRIX_PHASE_READ (B_DEVICE_OP_CODES_END+0x660a)
namespace PioneerQ38TokenMatrixPhaseControl {
enum {Version=1,Read=1,DataWords=PioneerQ38TokenMatrixPhaseData::Words};
struct Request {uint32_t version=Version,operation=Read,worker=0,reserved[5]={};};
struct Reply {
 uint32_t version=0,operation=0;int32_t status=0;
 uint32_t started=0,ready=0,unsafe=0,worker=0,stopNonce=0,detail[16]={};
};
struct Packet {Request request;Reply reply;uint32_t words[DataWords]={};};
static_assert(sizeof(Request)==32 && sizeof(Reply)==96 && sizeof(Packet)==192
 && offsetof(Packet,reply)==32 && offsetof(Packet,words)==128,"fixed pointer-free peer metric DATA");
static inline bool RequestFields(const Request& r){
 if(r.version!=Version || r.operation!=Read || !r.worker || r.worker>=120)return false;
 for(auto word:r.reserved)if(word)return false;
 return true;
}
static inline bool RequestShape(const Packet& p){
 if(!RequestFields(p.request))return false;
 const auto* bytes=reinterpret_cast<const uint8_t*>(&p.reply);
 for(unsigned i=0;i<sizeof(p.reply);i++)if(bytes[i])return false;
 for(auto word:p.words)if(word)return false;
 return true;
}
static inline bool ReplyShape(const Packet& p){
 namespace Q=PioneerQ38TokenMatrixPhaseData;
 const auto& r=p.reply;const auto* d=r.detail;
 if(!RequestFields(p.request) || r.version!=Version || r.operation!=Read || r.worker!=p.request.worker
  || r.started>1 || r.ready>1 || r.unsafe>1 || d[0]!=1 || d[1]!=r.started
  || d[2]>4 || d[3]>=120 || d[4]>=64 || d[5]!=r.unsafe || d[6]!=r.ready || d[7]>1
  || d[12] || d[13] || d[14] || d[15])return false;
 if(r.ready){
  if(r.status || !r.started || r.unsafe || d[2]!=4 || d[3]!=r.worker || d[4]!=63
   || !r.stopNonce || r.stopNonce>=0x7fffffff)return false;
  const unsigned state=Q::Classify(p.words,r.worker,r.stopNonce);
  return d[8]==state && d[7]==unsigned(state==Q::Intervals)
   && d[9]==p.words[2] && d[10]==p.words[5] && d[11]==p.words[6];
 }
 // A refused request publishes no timing DATA or STOP nonce. An attempted
 // failed native read is unsafe;stable invalid DATA instead uses ready=1.
 if(!r.status || r.stopNonce || r.started!=r.unsafe || d[7] || d[8] || d[9] || d[10] || d[11])return false;
 if(r.started?(d[2]<1 || d[2]>3):(d[2] || d[3] || d[4]))return false;
 for(auto word:p.words)if(word)return false;
 return true;
} // Consistency only. Cannot prove freshness,code/source,HOLD,ownership or an actual STOP nonce.
}
#endif
