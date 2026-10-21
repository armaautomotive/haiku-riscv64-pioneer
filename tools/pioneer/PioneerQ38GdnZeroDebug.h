/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_ZERO_DEBUG_H
#define PIONEER_Q38_GDN_ZERO_DEBUG_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnZeroProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include <cstdio>
#include <memory>
// Explicit fixed DATA-only qualification,NEVER production Work inference.
// mode1 first owned page,mode2 whole-state initialization. No caller address
// or code;ALL failed calls/replies use CLOSE ONLY,not END/retry/reset.
template<class IO> bool PioneerQ38GdnZeroDebug(IO& io,unsigned mode,bool supportOnly=false)
{
 if (mode!=1 && mode!=2) return false;
 int fd=io.Open(); if (fd<0) return false;
 auto fail=[&]() { io.Close(fd); return false; };
 tt_q38_gdn_zero_support support={};
 if (!io.Call(fd,TT_PCI_PROBE_Q38_GDN_ZERO_SUPPORT,&support,sizeof(support)) || support.version!=1 || support.driver!=55
  || support.zero_only!=1 || support.pages!=73728 || support.max_regions!=672 || support.requires_closed_span!=1
  || support.model_inference || support.pilot_pages!=1) return fail();
 if (supportOnly) { puts("GDN_ZERO_SUPPORT fixed1-page pilot/all73728pages;NOT_COMPUTE_OR_MODEL_CAPABILITY"); return io.Close(fd); }
 tt_q38_span_support span={}; tt_q38_bank_support banks={};
 if (!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,&span,sizeof(span)) || span.driver!=55 || span.version!=1 || span.banks!=7
  || span.page_bytes!=4096 || span.max_bytes!=1048576 || span.before_cohort!=1 || span.explicit_end!=1 || span.reserved
  || !io.Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&banks,sizeof(banks)) || banks.driver!=45 || banks.version!=1 || banks.banks!=7
  || banks.page_bytes!=4096 || banks.max_pages!=4 || banks.storage_only!=1 || banks.bytes_per_bank!=PioneerResidentBanks::Capacity) return fail();
 tt_q38_bank_session session={}; session.version=1;
 if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session)) || session.version!=1 || session.reserved || session.result.version!=1
  || session.result.state!=TT_RESIDENT_SESSION_READY || session.result.active!=1 || session.result.host_ready!=1 || session.result.cold
  || session.result.capabilities || session.result.bytes!=PioneerResidentBanks::Total) return fail();
 // One fixed public DATA page prepares the existing span/bulk/wide owners.
 // The subsequent fresh-state planner MUST exclude this learned-page bit.
 auto upload=std::make_unique<tt_q38_span_upload>(); upload->version=1; upload->bank=0; upload->bytes=4096; upload->offset=0;
 for (unsigned w=0;w<1024;w++) upload->data[w]=0x5147385aU^(w*0x9e3779b9U);
 if (!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,upload.get(),sizeof(*upload))) return fail();
 const auto& p=upload->result;
 if (p.version!=1 || p.state!=TT_RUNTIME_PASSED || p.bytes!=4096 || p.pages!=1 || p.nonce!=1 || p.started!=1 || p.completed!=1
  || p.held!=1 || p.host_exact || p.matched_words || p.recovery_required || p.reserved) return fail();
 tt_q38_span_end end={1,0,0,0};
 if (!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end)) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required) return fail();
 tt_q38_gdn_zero_request request={}; request.version=1; request.mode=mode;
 printf("GDN_ZERO_BEGIN mode%u fixed_pages%u;SPAN_CLOSED_NO_MODEL_COMPUTE\n",mode,mode==1?1U:73728U);
 if (!io.Call(fd,TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE,&request,sizeof(request))) return fail();
 if (request.version!=1 || request.mode!=mode || request.reserved[0] || request.reserved[1] || request.state!=TT_RUNTIME_PASSED
  || request.regions!=(mode==1?1U:672U) || request.jobs!=(mode==1?1U:672U) || request.pages!=(mode==1?1U:73728U)
  || request.closed!=1 || request.initialization_published!=(mode==1?0U:1U) || request.recovery_required) return fail();
 printf("GDN_ZERO mode%u regions%u jobs%u pages%u clean_close%u initialization_published%u;NOT_COMPUTE_OR_MODEL_CAPABILITY\n",
  mode,request.regions,request.jobs,request.pages,request.closed,request.initialization_published);
 session={}; session.version=1;
 if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session)) || session.version!=1 || session.reserved || session.result.version!=1
  || session.result.state!=TT_RESIDENT_SESSION_CLOSED || session.result.active || !session.result.pci_restored || !session.result.host_released
  || session.result.cold || session.result.capabilities) return fail();
 bool closed=io.Close(fd);
 printf("%s_GDN_ZERO mode%u KNOWN_CLEAN_END%u;NO_CPU_NUMERIC_REFERENCE_OR_MODEL_SPEED_CLAIM\n",closed?"PASS":"FAIL",mode,unsigned(closed));
 return closed;
}
#endif
