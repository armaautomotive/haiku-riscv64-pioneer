/* SPDX-License-Identifier: MIT */
#include "PioneerQ38FrontierFailureReportRole.h"
#ifndef PIONEER_Q38_FRONTIER_FAILURE_REPORT_H
#define PIONEER_Q38_FRONTIER_FAILURE_REPORT_H
#if !defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT) || PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT != 1
#error Failed-reply formatter is not an old/default USER helper
#endif
#include <cstdio>
#include <cstring>
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenFrontierPacket.h"
// Copy ONLY the caller's already-returned fixed packet. No ioctl,Open,
// retry,Cancel,HOLD,END,source mutation or state transition exists here.
// The caller initializes the complete packet BEFORE its original ioctl;
// an early driver rejection may leave that pristine packet unchanged.
static __attribute__((noinline)) bool PioneerQ38WriteFrontierFailedReply(
 FILE* sink,uint32_t operation,const void* data,size_t bytes,int status,int error){
 namespace F=PioneerQ38TokenFrontierControl;
 if(!sink || !status || operation!=TT_PCI_PROBE_Q38_TOKEN_FRONTIER_OBSERVE
  || !data || bytes!=sizeof(F::Packet))return false;
 F::Packet snapshot;std::memcpy(&snapshot,data,sizeof(snapshot));
 uint32_t words[sizeof(snapshot)/4];std::memcpy(words,&snapshot,sizeof(words));
 static_assert(sizeof(words)==128,"fixed caller-owned integer packet only");
 const auto& r=snapshot.reply;const auto* d=r.detail;
 std::fprintf(sink,"Q38_FRONTIER_FAILED_REPLY_RAW transport_status%d transport_errno%d consistent_shape%u;ALREADY_RETURNED_DATA_NOT_PROVENANCE_ROOT_CAUSE_OR_CLEAN_END\n",
  status,error,unsigned(F::ReplyShape(snapshot)));
 for(unsigned row=0;row<4;row++){
  std::fprintf(sink,"Q38_FRONTIER_FAILED_PACKET word%02u",row*8);
  for(unsigned column=0;column<8;column++)std::fprintf(sink," %08x",unsigned(words[row*8+column]));
  std::fputc('\n',sink);
 }
 std::fprintf(sink,"Q38_FRONTIER_FAILED_FIELDS raw_started%u raw_phase%u raw_worker%u raw_guard_cursor%u raw_unsafe%u raw_ready%u raw_committed%u raw_limit%u raw_prompts%u;NOT_COMPLETED_LAYERS_RETIRED_PC_FAULT_CODE_OR_RESTORATION_AUTHORITY\n",
  unsigned(d[1]),unsigned(d[2]),unsigned(d[3]),unsigned(d[4]),unsigned(d[5]),unsigned(d[6]),
  unsigned(d[7]),unsigned(d[8]),unsigned(d[9]));
 return true; // logging attempted ONLY;NEVER turns a failed ioctl into success
}
#endif
