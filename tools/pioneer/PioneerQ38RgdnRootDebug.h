/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RGDN_ROOT_DEBUG_H
#define PIONEER_Q38_RGDN_ROOT_DEBUG_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnRootProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnZeroProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "PioneerQ38DeltaProbeDebug.h"
#include <memory>
#include <cstdio>
#include <cstring>
// EXPLICIT CPU-reference diagnostic ONLY,NEVER production Work inference.
// State inputs stay on the card;CPU independently validates full GPU GDDR
// writeback before ACCEPT. First mismatch/transport error:CLOSE ONLY.
template<class IO> bool PioneerQ38RgdnRootDebug(IO& io,unsigned trials,bool supportOnly=false)
{
 if (trials!=1 && trials!=16) return false;
 int fd=io.Open(); if (fd<0) return false;
 auto fail=[&]() { io.Close(fd); return false; };
 tt_q38_rgdn_root_support support={};
 if (!io.Call(fd,TT_PCI_PROBE_Q38_RGDN_ROOT_SUPPORT,&support,sizeof(support)) || support.version!=1 || support.driver!=55
  || support.fixed_root || support.fixed_head || support.keys!=128 || support.values!=64 || support.requires_initialization!=1
  || support.slots!=2 || support.accept_required!=1 || support.model_inference) return fail();
 if (supportOnly) { puts("RGDN_ROOT_SUPPORT root0 half0 stateGDDR32768 explicit_ACCEPT;NOT_MODEL_OR_WORK_SPEED"); return io.Close(fd); }
 tt_q38_gdn_zero_support zero={}; tt_q38_span_support span={}; tt_q38_bank_support banks={};
 if (!io.Call(fd,TT_PCI_PROBE_Q38_GDN_ZERO_SUPPORT,&zero,sizeof(zero)) || zero.version!=1 || zero.driver!=55
  || zero.zero_only!=1 || zero.pages!=73728 || zero.max_regions!=672 || zero.requires_closed_span!=1 || zero.model_inference || zero.pilot_pages!=1
  || !io.Call(fd,TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,&span,sizeof(span)) || span.driver!=55 || span.version!=1 || span.banks!=7
  || span.page_bytes!=4096 || span.max_bytes!=1048576 || span.before_cohort!=1 || span.explicit_end!=1 || span.reserved
  || !io.Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&banks,sizeof(banks)) || banks.driver!=45 || banks.version!=1 || banks.banks!=7
  || banks.page_bytes!=4096 || banks.max_pages!=4 || banks.storage_only!=1 || banks.bytes_per_bank!=PioneerResidentBanks::Capacity) return fail();
 tt_q38_bank_session session={}; session.version=1;
 if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session)) || session.version!=1 || session.reserved || session.result.version!=1
  || session.result.state!=TT_RESIDENT_SESSION_READY || session.result.active!=1 || session.result.host_ready!=1 || session.result.cold
  || session.result.capabilities || session.result.bytes!=PioneerResidentBanks::Total) return fail();
 auto upload=std::make_unique<tt_q38_span_upload>(); upload->version=1; upload->bank=0; upload->bytes=4096; upload->offset=0;
 for (unsigned w=0;w<1024;w++) upload->data[w]=0x5147385aU^(w*0x9e3779b9U);
 if (!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,upload.get(),sizeof(*upload))) return fail();
 const auto& p=upload->result;
 if (p.version!=1 || p.state!=TT_RUNTIME_PASSED || p.bytes!=4096 || p.pages!=1 || p.nonce!=1 || p.started!=1 || p.completed!=1
  || p.held!=1 || p.host_exact || p.matched_words || p.recovery_required || p.reserved) return fail();
 tt_q38_span_end spanEnd={1,0,0,0};
 if (!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_END,&spanEnd,sizeof(spanEnd)) || spanEnd.version!=1 || spanEnd.reserved || spanEnd.closed!=1 || spanEnd.recovery_required) return fail();
 tt_q38_gdn_zero_request initialize={}; initialize.version=1; initialize.mode=2;
 puts("RGDN_ROOT_BEGIN full_state_initialization;EXPLICIT_CPU_DEBUG_ONLY_NOT_MODEL");
 if (!io.Call(fd,TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE,&initialize,sizeof(initialize)) || initialize.version!=1 || initialize.mode!=2
  || initialize.reserved[0] || initialize.reserved[1] || initialize.state!=TT_RUNTIME_PASSED || initialize.regions!=672
  || initialize.jobs!=672 || initialize.pages!=73728 || initialize.closed!=1 || initialize.initialization_published!=1 || initialize.recovery_required) return fail();
 auto job=std::make_unique<tt_q38_rgdn_root_request>();
 auto reference=std::make_unique<tt_q38_delta_probe>();
 auto frozen=std::make_unique<tt_q38_rgdn_root_request>();
 uint32_t prior[8192]={}; // user-space debug stack ONLY,never kernel/Work;never uploaded as state
 for (unsigned trial=0;trial<trials;trial++) {
  PioneerQ38DeltaDebugFixture(*reference,4+trial); std::memcpy(reference->prior,prior,sizeof(prior));
  *job={}; job->version=1; job->decay=reference->decay; job->beta=reference->beta;
  std::memcpy(job->query,reference->query,sizeof(job->query)); std::memcpy(job->key,reference->key,sizeof(job->key));
  std::memcpy(job->value,reference->value,sizeof(job->value)); *frozen=*job;
  if (!io.Call(fd,TT_PCI_PROBE_Q38_RGDN_ROOT_RUN,job.get(),sizeof(*job))) return fail();
  const auto& r=job->result;
  if (r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.started!=1 || r.completed!=1 || r.held!=1 || r.recovery_required || r.awaiting!=1
   || r.nonce!=trial+1 || r.position!=trial || r.read_slot!=trial%2 || r.write_slot!=(trial%2^1)
   || r.phase!=0x7200c || r.count!=64 || r.marker!=0x31523851 || (r.pc[0]!=0x72038 && r.pc[0]!=0x7203c)
   || (r.pc[1]!=0x72038 && r.pc[1]!=0x7203c) || std::memcmp(frozen.get(),job.get(),offsetof(tt_q38_rgdn_root_request,result))) return fail();
  std::memcpy(reference->next,job->next,sizeof(job->next)); std::memcpy(reference->output,job->output,sizeof(job->output));
  unsigned errors=PioneerQ38DeltaDebugErrors(*reference);
  printf("RGDN_ROOT trial%u nonce%u position%llu slots%u/%u state%u held%u exact_errors%u words8256;CPU_REFERENCE_DEBUG_ONLY\n",
   trial,r.nonce,(unsigned long long)r.position,r.read_slot,r.write_slot,r.state,r.held,errors);
  if (errors) return fail(); // no ACCEPT/END after numeric mismatch
  tt_q38_rgdn_root_accept accept={1,r.nonce,{0,0}};
  if (!io.Call(fd,TT_PCI_PROBE_Q38_RGDN_ROOT_ACCEPT,&accept,sizeof(accept))) return fail();
  std::memcpy(prior,job->next,sizeof(prior)); // previously verified/accepted result ONLY
 }
 tt_q38_rgdn_root_end end={1,0,0,0};
 if (!io.Call(fd,TT_PCI_PROBE_Q38_RGDN_ROOT_END,&end,sizeof(end)) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required) return fail();
 session={}; session.version=1;
 if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session)) || session.version!=1 || session.reserved || session.result.version!=1
  || session.result.state!=TT_RESIDENT_SESSION_CLOSED || session.result.active || !session.result.pci_restored || !session.result.host_released
  || session.result.cold || session.result.capabilities) return fail();
 bool closed=io.Close(fd);
 printf("%s_RGDN_ROOT trials%u KNOWN_CLEAN_END%u;RESIDENT_HALF_DEBUG_NOT_MODEL_OR_TOKENS_PER_SECOND\n",closed?"PASS":"FAIL",trials,unsigned(closed));
 return closed;
}
#endif
