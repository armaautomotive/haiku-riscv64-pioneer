/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWIN_Q8_DIAGNOSTIC_PROTOCOL_H
#define PIONEER_Q38_TWIN_Q8_DIAGNOSTIC_PROTOCOL_H
#include <Drivers.h>
#include "PioneerResidentSession.h"
#include "../../../../../tools/pioneer/PioneerQ38TwinQ8ProjectionSequence.h"

namespace PioneerQ38TwinQ8Diagnostic {
namespace S=PioneerQ38TwinQ8ProjectionSequence;
enum : unsigned { Version=1, RunOperation=B_DEVICE_OP_CODES_END+0x8281 };
constexpr uint64_t Tag=UINT64_C(0x2026111700010222);
enum class Step : uint32_t { New, Begin, Memory, MapRoot, Upload, FleetHeld, PeerStore, Aperture,
 Journal, Image, Start, Sequence, RootClose, ResidentClose, Complete, SelfRoute };
struct Request {
 uint64_t tag=0,generation=0;
 uint32_t version=0,arm=0,variant=0,reserved=0;
 Step step=Step::New;
 uint32_t item=0,cold=0,uploaded=0,uploadState=0,uploadCompleted=0,uploadMismatches=0,uploadRecovery=0;
 uint32_t bootFault=0,bootStatus=0,bootPC=0,stopPC=0;
 uint32_t journalOp=0,journalWord=0,journalAddress=0,journalSeen=0,journalExpected=0;
 S::Result result={};
 tt_resident_session_result closed={};
};
static_assert(sizeof(Request)<1024,"bounded no-pointer diagnostic packet");
static inline bool Closed(const tt_resident_session_result& r){
 return r.version==1 && r.state==TT_RESIDENT_SESSION_CLOSED && !r.active && !r.cold
  && r.pci_restored && r.host_released && !r.capabilities;
}
}
#endif
