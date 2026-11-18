/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_Q8_FAILURE_RECEIPT_H
#define PIONEER_Q38_NATIVE_Q8_FAILURE_RECEIPT_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenControlPacket.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenFrontierPacket.h"
#include <string.h>

// Failure DATA from the exact same rejected ioctl. A receipt must never
// authorize a new read, token publication, HOLD, reset, END or fallback.
namespace PioneerQ38NativeQ8FailureReceipt {
enum : uint32_t { Frontier=12, Terminal=9, Qfd1=0x31444651, Qnt1=0x31544e51 };
struct Receipt {
 uint32_t phase=0,check=0,word=0,error=0;
};
static inline bool Read(const PioneerQ38TokenFrontierControl::Packet& p,
 const PioneerQ38TokenFrontierControl::Request& sent,Receipt& out)
{
 namespace P=PioneerQ38TokenFrontierControl;
 const auto& r=p.reply;const auto* d=r.detail;
 if(out.phase || memcmp(&p.request,&sent,sizeof(sent)) || r.version!=P::Version || r.operation!=P::Observe
  || r.status>=0 || r.started!=1 || r.ready || r.unsafe!=1 || r.reserved[0] || r.reserved[1]
  || d[0]!=1 || d[1]!=1 || d[2]!=Frontier || d[3] || d[4]>63 || d[5]!=1 || d[6] || d[7] || d[8] || d[9]
  || d[10]!=Qfd1 || !d[11] || d[11]>11)return false;
 // The QFD1 encoder supplies only fields it had already read; later
 // failures cannot disguise an early error or an unobserved value.
 const unsigned known=d[11]<=2?0:d[11]<=4?1:d[11]<=6?3:7;
 if(d[15]!=known || (!(known&1) && d[12]) || (!(known&2) && d[13])
  || (!(known&4) && d[14]) || bool(d[13])!=(d[11]==5))return false;
 out={Frontier,d[11],d[4],d[13]};return true;
}
static inline bool Read(const PioneerQ38TokenControl::Packet& p,
 const PioneerQ38TokenControl::Request& sent,Receipt& out)
{
 namespace P=PioneerQ38TokenControl;
 const auto& r=p.reply;const auto* d=r.detail;
 if(out.phase || sent.operation!=P::Observe || memcmp(&p.request,&sent,sizeof(sent))
  || r.version!=P::Version || r.operation!=P::Observe || r.status>=0
  || r.started!=1 || r.ready || r.unsafe!=1 || r.complete || r.records || r.stage!=Terminal
  || d[0]!=1 || d[1]!=1 || d[2]!=Terminal || d[3] || d[4]>63 || d[5]!=1 || d[6] || d[7]
  || d[8] || d[9] || d[10] || d[11])return false;
 for(auto v:r.reserved)if(v)return false;
 for(auto v:p.prompt)if(v)return false;
 for(auto v:p.records)if(v)return false;
 if(d[14] && (d[14]<Qnt1+1 || d[14]>Qnt1+9))return false;
 const unsigned check=d[14]?d[14]-Qnt1:0;
 if(bool(d[15])!=(check==5))return false;
 // Zero means a matched failure later than the early ROOT checks.
 out={Terminal,check,d[4],d[15]};return true;
}
}
#endif
