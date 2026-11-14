/* SPDX-License-Identifier: MIT */
#include "PioneerQ38FullOffloadResidentRole.h"
#ifndef PIONEER_Q38_FULL_OFFLOAD_RESIDENT_PROTOCOL_H
#define PIONEER_Q38_FULL_OFFLOAD_RESIDENT_PROTOCOL_H
#include <stdint.h>
#include "PioneerQ38FullOffloadProtocol.h"
static_assert(PioneerQ38FullOffload::BuildTag==UINT64_C(0x2026110400010091),"resident metadata cannot promote a previously included one-shot protocol");
#define TT_PCI_PROBE_Q38_FULL_RESIDENT_SUPPORT (B_DEVICE_OP_CODES_END+0x8803)
#define TT_PCI_PROBE_Q38_FULL_RESIDENT_CONTROL (B_DEVICE_OP_CODES_END+0x8804)
namespace PioneerQ38FullResident {
enum {Version=1,RequestClose=1,NextRequest=2,ModelEnd=3};
struct Support {uint32_t driver=0,version=0,requestClosure=0,modelReservation=0,freshRequest=0,explicitModelEnd=0,reserved[2]={};};
struct Request {uint32_t version=Version,operation=0,reserved[6]={};};
struct Reply {uint32_t version=Version,operation=0;int32_t status=0;
 uint32_t started=0,ready=0,unsafe=0,stage=0,reservationRetained=0,
  hostsReleased=0,oldContextRetired=0,modelEnded=0,reserved[5]={};};
struct Packet {Request request;Reply reply;};
static_assert(sizeof(Support)==32 && sizeof(Request)==32 && sizeof(Reply)==64 && sizeof(Packet)==96,"fixed pointer-free resident lifecycle metadata");
static inline bool SupportRequest(const Support& s){
 return !s.driver && !s.version && !s.requestClosure && !s.modelReservation && !s.freshRequest && !s.explicitModelEnd
  && !s.reserved[0] && !s.reserved[1];
}
static inline bool Supported(const Support& s){return s.driver==55 && s.version==Version && s.requestClosure==1
 && s.modelReservation==1 && s.freshRequest==1 && s.explicitModelEnd==1 && !s.reserved[0] && !s.reserved[1];}
static inline bool RequestShape(const Packet& p){
 if(p.request.version!=Version || p.request.operation<RequestClose || p.request.operation>ModelEnd)return false;
 for(auto n:p.request.reserved)if(n)return false;
 const Reply pristine;const auto* a=reinterpret_cast<const uint8_t*>(&p.reply);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned n=0;n<sizeof(Reply);n++)if(a[n]!=b[n])return false;
 return true;
} // no prompt/product/address/phase/ready/closed/nonce input;actual owner supplies all lifetime evidence
// Data validation only: caller must also verify transport success, exact
// request preservation, and that the same exclusive FD remains held.
static inline bool SuccessfulReply(const Reply& r,unsigned operation){
 if(operation<RequestClose || operation>ModelEnd || r.version!=Version || r.operation!=operation
  || r.status || r.started!=1 || r.ready!=1 || r.unsafe || r.hostsReleased!=3)return false;
 for(auto n:r.reserved)if(n)return false;
 if(operation==RequestClose)return r.stage==10 && r.reservationRetained==1 && !r.oldContextRetired && !r.modelEnded;
 if(operation==NextRequest)return r.stage==11 && r.reservationRetained==1 && r.oldContextRetired==1 && !r.modelEnded;
 return r.stage==12 && !r.reservationRetained && !r.oldContextRetired && r.modelEnded==1;
}
}
#endif
