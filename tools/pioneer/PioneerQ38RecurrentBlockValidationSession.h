/* SPDX-License-Identifier: MIT */
// Recheck after every include,including late/defined0 selections.
#if !defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT) || PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT != 1 \
 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1
#error Exact separate recurrent DEBUG71 CPU validation client required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_NATIVE) || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RECURRENT_BLOCK_EMIT_CONTEXT_BODY) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE) || defined(PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST) \
 || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK) \
 || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_DECODER_RUNTIME) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE) \
 || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) \
 || defined(PIONEER_Q38_DENSE_PREPARE_IMAGE) || defined(PIONEER_Q38_DENSE_ATTENTION_IMAGE) \
 || defined(PIONEER_Q38_DENSE_PROJECTION_IMAGE) || defined(PIONEER_Q38_DENSE_PARTITION_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_CONTINUE_IMAGE) || defined(PIONEER_Q38_RESIDENT_ADVANCE_IMAGE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || defined(PIONEER_Q38_GDN_MUTABLE_STATE)
#error Recurrent DEBUG71 client is not a native,image,old dense/kernel or Work role,including defined0
#endif
#ifndef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_SESSION_H
#define PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_SESSION_H
#include "PioneerQ38RecurrentBlockOriginalSource.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockValidationProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <algorithm>
#include <cstring>
#include <vector>
// Only authenticated immutable RAW bytes cross into upload requests. All
// independent CPU arithmetic is prepared/sealed BEFORE Open. The captured
// FINAL comparison must pass BEFORE explicit END;unknowns CLOSE ONLY.
template<class IO,class Source=PioneerQ38RecurrentBlockOriginalSource>class PioneerQ38RecurrentBlockValidationSession {
 static constexpr uint32_t Base=0x04000000;
 static constexpr uint64_t Capacity=UINT64_C(3)*1024*1024*1024,Total=7*Capacity;
 IO& io;Source& source;std::vector<PioneerRecurrentOriginalExtent> spans;uint64_t fingerprint=0,generation=0;
 int fd=-1;bool attempted=false,failed=false,active=false,finished=false,validated=false;
 unsigned expectedSpans=0;
 bool Current()const{return source.Ready() && fingerprint && source.ReferenceFingerprint()==fingerprint
  && source.ReferenceReads()==3853 && source.Spans()==57 && source.Pages()==68603 && source.Bytes()==280971136;}
 bool Stop(const char* why){
  if(!failure)failure=why;
  failed=true;active=validated=false;finished=true;
  if(fd>=0){int owned=fd;fd=-1;io.Close(owned);}return false;
 }
 template<class T>bool Call(unsigned op,T& value){
  operation=op;
  return !failed && !finished && fd>=0 && Current() && io.Call(fd,op,&value,sizeof(value)) && Current();
 }
 bool Add(const PioneerRecurrentOriginalExtent& extent){
  const auto& e=extent;static constexpr uint32_t routes[]={786,978,1170,785,977,1169,1361};
  if(e.bank>=7 || e.route!=routes[e.bank] || !e.bytes || !e.pages || e.pages!=(uint64_t(e.bytes)+4095)/4096 || (e.address&4095) || e.address<Base
   || uint64_t(e.address)-Base+uint64_t(e.pages)*4096>Capacity || !source.Contains(e.route,e.address,e.bytes))return false;
  for(const auto& s:spans)if(s.bank==e.bank && uint64_t(e.address)<uint64_t(s.address)+uint64_t(s.pages)*4096
   && uint64_t(s.address)<uint64_t(e.address)+uint64_t(e.pages)*4096)return false;
  spans.push_back(e);expectedSpans+=(e.pages+255)/256;return true;
 }
public:
 const char* failure=nullptr;unsigned operation=0,uploaded_pages=0,upload_spans=0;
 tt_q38_embedding_produce result={};PioneerRecurrentOriginalComparison comparison;
 uint32_t reply_header[80]={};bool reply_captured=false;
 PioneerQ38RecurrentBlockValidationSession(IO& transport,Source& original):io(transport),source(original){}
 ~PioneerQ38RecurrentBlockValidationSession(){if(fd>=0)io.Close(fd);} // NEVER implicit END/reset/retry
 PioneerQ38RecurrentBlockValidationSession(const PioneerQ38RecurrentBlockValidationSession&)=delete;
 PioneerQ38RecurrentBlockValidationSession& operator=(const PioneerQ38RecurrentBlockValidationSession&)=delete;
 bool Validate(unsigned token=0,unsigned position=0){
  if(attempted || failed || finished || token || position)return false;
  attempted=true;
  if(!source.Prepare() || !source.SealBeforeNativeContext() || !source.Ready())return Stop("source_preparation");
  fingerprint=source.ReferenceFingerprint();if(!Current())return Stop("source_identity");
  uint64_t rawBytes=0;unsigned pages=0,embedding=0;
  for(unsigned i=0;i<57;i++){
   PioneerRecurrentOriginalExtent extent;
   if(!Current() || !source.Extent(i,extent) || !Add(extent) || !Current())return Stop("recurrent_geometry");
   if(!extent.bank && extent.address==Base && extent.bytes==46080 && extent.pages==12)embedding++;
   rawBytes+=extent.bytes;pages+=extent.pages;
  }
  if(spans.size()!=57 || embedding!=1 || rawBytes!=280971136 || pages!=68603 || !Current())return Stop("source_geometry");
  fd=io.Open();if(fd<0)return Stop("open");
  tt_pioneer_driver_reload reload={};reload.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,reload) || !PioneerDriverReloadRequest(reload) || reload.expected_tag || reload.expected_generation
   || reload.driver!=55 || reload.guard_version!=1
   || reload.build_tag!=UINT64_C(0x2026102700010071) || !reload.generation || reload.quiesced || reload.cold
   || reload.active_owner || reload.open_count!=1)return Stop("driver_identity");
  generation=reload.generation;tt_q38_embedding_support support={};
  if(!Call(TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATION_SUPPORT,support) || support.version!=1 || support.driver!=55
   || support.model_namespace!=22 || support.inputs!=5120 || support.vocabulary!=248320 || support.runtime_pages!=512
   || support.no_cpu_reference || support.model_inference)return Stop("diagnostic_identity");
  tt_q38_bank_support storage={};tt_q38_span_support span={};
  if(!Call(TT_PCI_PROBE_Q38_BANK_SUPPORT,storage) || storage.driver!=45 || storage.version!=1 || storage.banks!=7
   || storage.page_bytes!=4096 || storage.max_pages!=4 || storage.storage_only!=1 || storage.bytes_per_bank!=Capacity
   || !Call(TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,span) || span.driver!=55 || span.version!=1 || span.banks!=7
   || span.page_bytes!=4096 || span.max_bytes!=1048576 || span.before_cohort!=1 || span.explicit_end!=1 || span.reserved)return Stop("storage_identity");
  tt_q38_bank_session begin={};begin.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_BEGIN,begin) || begin.result.version!=1 || begin.result.state!=TT_RESIDENT_SESSION_READY
   || begin.result.active!=1 || begin.result.host_ready!=1 || begin.result.cold || begin.result.capabilities
   || begin.result.bytes!=Total)return Stop("begin");
  active=true;auto upload=std::make_unique<tt_q38_span_upload>();
  for(const auto& selected:spans)for(unsigned at=0;at<selected.pages;){
   const unsigned count=std::min(256U,selected.pages-at);std::memset(upload.get(),0,sizeof(*upload));
   upload->version=1;upload->bank=selected.bank;upload->bytes=count*4096;
   upload->offset=uint64_t(selected.address)-Base+uint64_t(at)*4096;
   for(unsigned page=0;page<count;page++){
    const unsigned offset=(at+page)*4096;
    if(offset>=selected.bytes)return Stop("source_geometry");
    const unsigned bytes=std::min(4096U,selected.bytes-offset);
    auto* data=reinterpret_cast<uint8_t*>(upload->data+page*1024);
    if(!Current() || !source.Read(selected.route,selected.address+offset,data,bytes)
     || !Current())return Stop("source_changed");
    // The zero-initialized tail MUST NOT leak adjacent original tensor or
    // embedding-tile bytes. Only the selected raw extent may be requested.
   }
   if(!Call(TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,*upload))return Stop("upload");
   const auto& r=upload->result;
   if(r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.bytes!=count*4096 || r.pages!=count || r.nonce!=upload_spans+1
    || r.started!=1 || r.completed!=1 || r.held!=1 || r.host_exact || r.matched_words || r.recovery_required || r.reserved)return Stop("upload_reply");
   at+=count;uploaded_pages+=count;upload_spans++;
  }
  if(uploaded_pages!=pages || upload_spans!=expectedSpans)return Stop("upload_count");
  tt_q38_span_end spanEnd={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,spanEnd) || spanEnd.version!=1 || spanEnd.reserved || spanEnd.closed!=1 || spanEnd.recovery_required)return Stop("span_end");
  auto reply=std::make_unique<tt_q38_recurrent_block_validation>();reply->metadata.version=1;
  bool called=Call(TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE,*reply);
  std::memcpy(reply_header,reply.get(),sizeof(reply_header));reply_captured=true;
  if(!called)return Stop("compute_transport");
  result=reply->metadata;
  const uint32_t expected[80]={1,0,0,0,TT_RUNTIME_PASSED,1,1,0,0,0,uint32_t(generation),uint32_t(generation>>32),0,0,
   68603,512,11480641,11480641,2,1,672,336,73728,1680,1,119,119,120,1,1,25,11406904,22813808,2632668,11406904,5,5120,5120};
  for(unsigned i=0;i<80;i++){
   if(i==8 || i==12){if(!(reply_header[i]|reply_header[i+1]))return Stop("reply_identity_time");i++;continue;}
   if(reply_header[i]!=expected[i])return Stop("reply_metadata");
  }
  if(!source.CompareStage(PioneerRecurrentFinal,reply->values,5120,&comparison) || !Current() || comparison.checked!=5120
   || comparison.passed!=1 || comparison.differences || comparison.nonfinite || comparison.maxAbsolute!=0 || comparison.relativeL2!=0)return Stop("original_values");
  validated=true;return true;
 }
 bool Finish(){
  if(!active || !validated || failed || finished || fd<0)return false;
  tt_q38_bank_session end={};end.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,end) || end.result.version!=1 || end.result.state!=TT_RESIDENT_SESSION_CLOSED
   || end.result.active || end.result.cold || end.result.pci_restored!=1 || end.result.host_released!=1
   || end.result.capabilities || end.result.bytes!=Total)return Stop("end");
  active=validated=false;finished=true;int owned=fd;fd=-1;
  if(io.Close(owned)!=0){failure="close";failed=true;return false;}return true;
 }
 void Abort(){Stop("aborted");} // terminal CLOSE ONLY,never END/peek/reset/retry
};
#endif
