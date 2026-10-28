/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT) || PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT != 1 \
 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1
#error Exact separate prefix DEBUG73 CPU-validation client required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RECURRENT_PREFIX_EMIT_CONTEXT_BODY) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE) || defined(PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_DECODER_RUNTIME) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || defined(PIONEER_Q38_GDN_MUTABLE_STATE)
#error Prefix DEBUG73 client cannot alias native,image,kernel,CPU oracle,old client or Work roles,including defined0
#endif
#ifndef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_SESSION_H
#define PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_SESSION_H
#include "PioneerQ38RecurrentPrefixValidationSource.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixValidationProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <algorithm>
#include <cstring>
#include <vector>
// All authenticated RAW extents and private chained CPU reference seal
// BEFORE Open. Successful compute reply includes actual final-END proof;
// NEVER send a second BANK_END. Unknowns terminally CLOSE ONLY.
template<class IO,class Source=PioneerQ38RecurrentPrefixValidationSource>class PioneerQ38RecurrentPrefixValidationSession {
 static constexpr uint32_t Base=0x04000000;
 static constexpr uint64_t Capacity=UINT64_C(3)*1024*1024*1024,Total=7*Capacity;
 IO& io;Source& source;std::vector<PioneerRecurrentPrefixValidationExtent> spans;
 uint64_t fingerprint[3]={},generation=0;
 int fd=-1;bool attempted=false,failed=false,finished=false,validated=false,deviceEnded=false;
 unsigned expectedSpans=0;
 bool Current()const{
  if(!source.Ready() || source.ReferenceReads()!=11557 || source.Spans()!=169 || source.Pages()!=205785 || source.Bytes()!=842821248)return false;
  for(unsigned layer=0;layer<3;layer++)if(!fingerprint[layer] || source.ReferenceFingerprint(layer)!=fingerprint[layer])return false;
  return true;
 }
 bool Stop(const char* why){
  if(!failure)failure=why;
  failed=true;validated=false;finished=true;
  if(fd>=0){int owned=fd;fd=-1;io.Close(owned);}return false;
 }
 template<class T>bool Call(unsigned op,T& value){
  operation=op;
  return !failed && !finished && fd>=0 && Current() && io.Call(fd,op,&value,sizeof(value)) && Current();
 }
 bool Add(const PioneerRecurrentPrefixValidationExtent& e){
  static constexpr uint32_t routes[]={786,978,1170,785,977,1169,1361};
  if(e.bank>=7 || e.route!=routes[e.bank] || !e.bytes || !e.pages || e.pages!=(uint64_t(e.bytes)+4095)/4096 || (e.address&4095) || e.address<Base
   || uint64_t(e.address)-Base+uint64_t(e.pages)*4096>Capacity || !source.Contains(e.route,e.address,e.bytes))return false;
  for(const auto& s:spans)if(s.bank==e.bank && uint64_t(e.address)<uint64_t(s.address)+uint64_t(s.pages)*4096
   && uint64_t(s.address)<uint64_t(e.address)+uint64_t(e.pages)*4096)return false;
  spans.push_back(e);expectedSpans+=(e.pages+255)/256;return true;
 }
public:
 const char* failure=nullptr;unsigned operation=0,uploaded_pages=0,upload_spans=0;
 tt_q38_embedding_produce result={};PioneerRecurrentOriginalComparison comparison;
 uint32_t reply_header[96]={};bool reply_captured=false;
 PioneerQ38RecurrentPrefixValidationSession(IO& transport,Source& original):io(transport),source(original){}
 ~PioneerQ38RecurrentPrefixValidationSession(){if(fd>=0)io.Close(fd);} // no implicit END/reset/retry
 PioneerQ38RecurrentPrefixValidationSession(const PioneerQ38RecurrentPrefixValidationSession&)=delete;
 PioneerQ38RecurrentPrefixValidationSession& operator=(const PioneerQ38RecurrentPrefixValidationSession&)=delete;
 bool Validate(unsigned token=0,unsigned position=0){
  if(attempted || failed || finished || token || position)return false;
  attempted=true;
  if(!source.Prepare() || !source.SealBeforeNativeContext() || !source.Ready())return Stop("source_preparation");
  for(unsigned layer=0;layer<3;layer++)fingerprint[layer]=source.ReferenceFingerprint(layer);
  if(!Current())return Stop("source_identity");
  uint64_t rawBytes=0;unsigned pages=0,embedding=0;
  for(unsigned i=0;i<169;i++){
   PioneerRecurrentPrefixValidationExtent extent;
   if(!Current() || !source.Extent(i,extent) || !Add(extent) || !Current())return Stop("prefix_geometry");
   if(!extent.bank && extent.address==Base && extent.bytes==46080 && extent.pages==12)embedding++;
   rawBytes+=extent.bytes;pages+=extent.pages;
  }
  if(spans.size()!=169 || embedding!=1 || rawBytes!=842821248 || pages!=205785 || !Current())return Stop("source_geometry");
  fd=io.Open();if(fd<0)return Stop("open");
  tt_pioneer_driver_reload reload={};reload.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,reload) || !PioneerDriverReloadRequest(reload) || reload.expected_tag || reload.expected_generation
   || reload.driver!=55 || reload.guard_version!=1 || reload.build_tag!=TT_Q38_RECURRENT_PREFIX_VALIDATION_BUILD_TAG
   || !reload.generation || reload.quiesced || reload.cold || reload.active_owner || reload.open_count!=1)return Stop("driver_identity");
  generation=reload.generation;tt_q38_embedding_support support={};
  if(!Call(TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATION_SUPPORT,support) || support.version!=1 || support.driver!=55
   || support.model_namespace!=TT_Q38_RECURRENT_PREFIX_VALIDATION_NAMESPACE || support.inputs!=5120 || support.vocabulary!=248320 || support.runtime_pages!=512
   || support.no_cpu_reference || support.model_inference)return Stop("diagnostic_identity");
  tt_q38_bank_support storage={};tt_q38_span_support span={};
  if(!Call(TT_PCI_PROBE_Q38_BANK_SUPPORT,storage) || storage.driver!=45 || storage.version!=1 || storage.banks!=7
   || storage.page_bytes!=4096 || storage.max_pages!=4 || storage.storage_only!=1 || storage.bytes_per_bank!=Capacity
   || !Call(TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,span) || span.driver!=55 || span.version!=1 || span.banks!=7
   || span.page_bytes!=4096 || span.max_bytes!=1048576 || span.before_cohort!=1 || span.explicit_end!=1 || span.reserved)return Stop("storage_identity");
  tt_q38_bank_session begin={};begin.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_BEGIN,begin) || begin.result.version!=1 || begin.result.state!=TT_RESIDENT_SESSION_READY
   || begin.result.active!=1 || begin.result.host_ready!=1 || begin.result.cold || begin.result.capabilities || begin.result.bytes!=Total)return Stop("begin");
  auto upload=std::make_unique<tt_q38_span_upload>();
  for(const auto& selected:spans)for(unsigned at=0;at<selected.pages;){
   const unsigned count=std::min(256U,selected.pages-at);std::memset(upload.get(),0,sizeof(*upload));
   upload->version=1;upload->bank=selected.bank;upload->bytes=count*4096;
   upload->offset=uint64_t(selected.address)-Base+uint64_t(at)*4096;
   for(unsigned page=0;page<count;page++){
    const unsigned offset=(at+page)*4096;if(offset>=selected.bytes)return Stop("source_geometry");
    const unsigned bytes=std::min(4096U,selected.bytes-offset);auto* data=reinterpret_cast<uint8_t*>(upload->data+page*1024);
    if(!Current() || !source.Read(selected.route,selected.address+offset,data,bytes) || !Current())return Stop("source_changed");
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
  auto reply=std::make_unique<tt_q38_recurrent_prefix_validation>();reply->metadata.version=1;
  bool called=Call(TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE,*reply);
  std::memcpy(reply_header,reply.get(),sizeof(reply_header));reply_captured=true;
  if(!called)return Stop("compute_transport");
  result=reply->metadata;
  const uint32_t expected[96]={1,0,0,0,TT_RUNTIME_PASSED,1,1,0,0,0,uint32_t(generation),uint32_t(generation>>32),0,0,
   205785,512,11480641,11480641,2,1,672,336,73728,1680,1,119,119,120,1,1,57,11406904,22813808,2633615,11406904,5,5120,5120,
   3,24,4,TT_RESIDENT_SESSION_CLOSED,1,1,0};
  for(unsigned i=0;i<96;i++){
   if(i==8 || i==12){if(!(reply_header[i]|reply_header[i+1]))return Stop("reply_identity_time");i++;continue;}
   if(reply_header[i]!=expected[i])return Stop("reply_metadata");
  }
  deviceEnded=true;
  if(!source.CompareFinal(reply->values,5120,&comparison) || !Current() || comparison.checked!=5120 || comparison.passed!=1
   || comparison.differences || comparison.nonfinite || comparison.maxAbsolute!=0 || comparison.relativeL2!=0)return Stop("original_values");
  validated=true;return true;
 }
 bool Finish(){
  if(!validated || !deviceEnded || failed || finished || fd<0)return false;
  validated=false;finished=true;int owned=fd;fd=-1;
  if(io.Close(owned)!=0){failure="close";failed=true;return false;}return true;
 } // actual final-END already proved by fixed reply;NEVER issue BANK_END twice
 void Abort(){Stop("aborted");}
};
#endif
