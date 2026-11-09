/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Qgw8TraceControlInspectionRole.h"
#if defined(PIONEER_Q38_QGW8_TRACE_NATIVE_131)
#include "PioneerQ38Qgw8TraceNativeProgramRole.h"
#elif defined(PIONEER_Q38_QGW8_TRACE_CONTROL_INSPECTION)
// The separately isolated fake owner may check the same DATA shape.
#elif !defined(PIONEER_Q38_QGW8_TRACE_PACKET_INSPECTION) || PIONEER_Q38_QGW8_TRACE_PACKET_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION) \
 || defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) || defined(PIONEER_Q38_QGW8_HIDDEN_NATIVE_129) \
 || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) || defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129) \
 || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_QGW8_LAYER_TRACE_IMAGE) || defined(PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION) \
 || defined(PIONEER_Q38_QGW8_TRACE_READ_INSPECTION) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Trace packets are separate offline DATA inspection only
#endif
#ifndef PIONEER_Q38_QGW8_LAYER_TRACE_PACKET_H
#define PIONEER_Q38_QGW8_LAYER_TRACE_PACKET_H
#include "PioneerQ38Qgw8LayerTraceGeometry.h"
#include <cstddef>
#include <cstdint>

// Fixed DATA proposal only: no ioctl number, driver, source authentication,
// terminal proof, held ownership, FINISH or hardware-access authority.
namespace PioneerQ38Qgw8LayerTracePacket {
namespace G=PioneerQ38Qgw8LayerTraceGeometry;
enum : uint32_t {Magic=0x41525451,Version=1,ChunkWords=4096};
struct Request {uint32_t magic,version,ordinal,layer,part,words,keyLow,keyHigh,reserved[8];};
struct Reply {uint32_t version,ordinal,started,ready,unsafe,words;int32_t status;uint32_t reserved[9],detail[16];};
struct Packet {Request request;Reply reply;uint32_t words[ChunkWords];};
static_assert(sizeof(Request)==64 && sizeof(Reply)==128 && offsetof(Packet,words)==192 && sizeof(Packet)==16576,
 "fixed pointer-free trace DATA proposal");
inline bool RequestFields(const Request& q){
 if(q.magic!=Magic || q.version!=Version || q.ordinal>=G::Chunks || q.layer!=q.ordinal/G::Parts
  || q.part!=q.ordinal%G::Parts || q.words!=(q.part?1024U:4096U) || !(q.keyLow||q.keyHigh))return false;
 for(auto v:q.reserved)if(v)return false;return true;
}
inline bool RequestShape(const Packet& p){
 if(!RequestFields(p.request))return false;
 const auto* bytes=reinterpret_cast<const uint8_t*>(&p.reply);
 for(size_t i=0;i<sizeof(p.reply);i++)if(bytes[i])return false;
 for(auto word:p.words)if(word)return false;return true;
}
inline bool ReplyShape(const Packet& p){
 if(!RequestFields(p.request))return false;const auto& q=p.request;const auto& r=p.reply;
 if(r.version!=Version || r.ordinal!=q.ordinal || r.started!=1 || r.ready!=1 || r.unsafe || r.status || r.words!=q.words)return false;
 for(auto value:r.reserved)if(value)return false;
 const uint32_t expected[]={1,1,4,0,0,0,1,q.ordinal,q.layer*G::Words+q.part*ChunkWords,q.words,0,64,1,1,1,q.words*4};
 for(unsigned i=0;i<16;i++)if(r.detail[i]!=expected[i])return false;
 for(unsigned i=q.words;i<ChunkWords;i++)if(p.words[i])return false;
 return true; // raw bits may include non-finite DATA; no quality claim
}
}
#endif
