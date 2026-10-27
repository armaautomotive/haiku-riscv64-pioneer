/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE
#error Recurrent projection image is separate from CPU validation
#endif
// Gates BEFORE guard: late role changes/defined0 must not become aliases.
#if !defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT) || PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT != 1 \
 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1
#error Exact separate CPU dense DEBUG70 client required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE) || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_MIXED_LIVE_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_MIXED_LIVE_EMIT_CONTEXT_BODY) \
 || defined(PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK) \
 || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_DECODER_RUNTIME) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) \
 || defined(PIONEER_Q38_DENSE_PREPARE_IMAGE) || defined(PIONEER_Q38_DENSE_ATTENTION_IMAGE) \
 || defined(PIONEER_Q38_DENSE_PROJECTION_IMAGE) || defined(PIONEER_Q38_DENSE_PARTITION_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_CONTINUE_IMAGE) || defined(PIONEER_Q38_RESIDENT_ADVANCE_IMAGE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT)
#error Dense CPU DEBUG70 client is not native,old DEBUG,image,kernel or Work,including defined0
#endif
#ifndef PIONEER_Q38_MIXED_LIVE_VALIDATION_SESSION_H
#define PIONEER_Q38_MIXED_LIVE_VALIDATION_SESSION_H
#include "PioneerQ38MixedLiveOriginalSource.h"
#include "PioneerQwen38PackedPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveValidationProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <cstring>
// Original-file observer is prepared/sealed BEFORE the accelerator opens.
// Only raw learned pages upload;original comparison AFTER capture but
// BEFORE explicit END. Any uncertainty CLOSES ONLY,retaining real owners.
template<class IO,class Source=PioneerQ38MixedLiveOriginalSource>class PioneerQ38MixedLiveValidationSession {
 IO& io;Source& source;PioneerQwen38PackedPlan plan;const PioneerQwen38Gguf* model=nullptr;
 int fd=-1;bool attempted=false,failed=false,active=false,finished=false,validated=false;
 uint64_t epoch=0,generation=0;
 bool Current()const{return model && source.Ready() && source.SealedModel()==model && model->Epoch()==epoch && plan.MatchesSource(*model);}
 bool Stop(const char* why){
  if(!failure)failure=why;
  failed=true;active=validated=false;finished=true;
  if(fd>=0){int owned=fd;fd=-1;io.Close(owned);}return false;
 }
 template<class T>bool Call(unsigned op,T& value){
  operation=op;
  return !failed && !finished && fd>=0 && Current() && io.Call(fd,op,&value,sizeof(value)) && Current();
 }
public:
 const char* failure=nullptr;unsigned operation=0,uploaded_pages=0,upload_spans=0;
 tt_q38_embedding_produce result={};PioneerQ38MixedLiveReferenceComparison comparison;
 uint32_t reply_header[64]={};bool reply_captured=false;
 PioneerQ38MixedLiveValidationSession(IO& transport,Source& original):io(transport),source(original){}
 ~PioneerQ38MixedLiveValidationSession(){if(fd>=0)io.Close(fd);} // NEVER implicit END/reset/retry
 PioneerQ38MixedLiveValidationSession(const PioneerQ38MixedLiveValidationSession&)=delete;
 PioneerQ38MixedLiveValidationSession& operator=(const PioneerQ38MixedLiveValidationSession&)=delete;
 bool Validate(unsigned token=0,unsigned position=0){
  if(attempted || failed || finished || token || position)return false;
  attempted=true;
  if(!source.Prepare() || !source.SealBeforeNativeContext() || !source.Ready() || source.ReferenceReads()!=3717
   || !source.ReferenceFingerprint() || !(model=source.SealedModel()) || !plan.Build(*model,8192,true,true))return Stop("source_preparation");
  epoch=model->Epoch();const auto& parts=plan.Parts();const auto& tensors=model->Tensors();
  unsigned embedding=UINT32_MAX,pages=12,spans=1;std::vector<unsigned> selected;
  for(unsigned i=0;i<parts.size();i++){
   const auto& p=parts[i];if(p.kind!=PIONEER_Q38_WEIGHT || p.tensor>=tensors.size())continue;
   const auto& t=tensors[p.tensor];
   if(t.name=="token_embd.weight" && !p.firstRow){
    if(embedding!=UINT32_MAX || p.layer!=64 || p.role || t.type!=12 || t.inputs!=5120 || t.outputs!=248320
     || p.rowBytes!=2880 || p.allocated<49152)return Stop("embedding_geometry");
    embedding=i;
   }
   if(p.layer!=3)continue;
   if(p.bank>=7 || p.proposedAddress<plan.Base || !p.allocated || (p.allocated&4095)
    || uint64_t(p.proposedAddress)-plan.Base+p.allocated>plan.Capacity)return Stop("dense_geometry");
   selected.push_back(i);pages+=p.allocated/4096;spans+=(p.allocated/4096+255)/256;
  }
  if(embedding==UINT32_MAX || selected.size()!=53 || pages!=63266 || !Current())return Stop("source_geometry");
  selected.insert(selected.begin(),embedding);
  fd=io.Open();if(fd<0)return Stop("open");
  tt_pioneer_driver_reload reload={};reload.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,reload) || reload.version!=1 || reload.driver!=55 || reload.guard_version!=1
   || reload.build_tag!=UINT64_C(0x2026102700010070) || !reload.generation || reload.quiesced || reload.cold
   || reload.active_owner || reload.open_count!=1)return Stop("driver_identity");
  generation=reload.generation;tt_q38_embedding_support support={};
  if(!Call(TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATION_SUPPORT,support) || support.version!=1 || support.driver!=55
   || support.model_namespace!=21 || support.inputs!=5120 || support.vocabulary!=248320 || support.runtime_pages!=512
   || support.no_cpu_reference || support.model_inference)return Stop("diagnostic_identity");
  tt_q38_bank_support storage={};tt_q38_span_support span={};
  if(!Call(TT_PCI_PROBE_Q38_BANK_SUPPORT,storage) || storage.driver!=45 || storage.version!=1 || storage.banks!=7
   || storage.page_bytes!=4096 || storage.max_pages!=4 || storage.storage_only!=1 || storage.bytes_per_bank!=PioneerResidentBanks::Capacity
   || !Call(TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,span) || span.driver!=55 || span.version!=1 || span.banks!=7
   || span.page_bytes!=4096 || span.max_bytes!=1048576 || span.before_cohort!=1 || span.explicit_end!=1 || span.reserved)return Stop("storage_identity");
  tt_q38_bank_session begin={};begin.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_BEGIN,begin) || begin.result.version!=1 || begin.result.state!=TT_RESIDENT_SESSION_READY
   || begin.result.active!=1 || begin.result.host_ready!=1 || begin.result.cold || begin.result.capabilities
   || begin.result.bytes!=PioneerResidentBanks::Total)return Stop("begin");
  active=true;auto upload=std::make_unique<tt_q38_span_upload>();
  for(unsigned part:selected){
   const auto& p=parts[part];unsigned countPages=part==embedding?12:p.allocated/4096;
   for(unsigned at=0;at<countPages;){
    unsigned count=std::min(256U,countPages-at);std::memset(upload.get(),0,sizeof(*upload));
    upload->version=1;upload->bank=p.bank;upload->bytes=count*4096;
    upload->offset=uint64_t(p.proposedAddress)-plan.Base+uint64_t(at)*4096;
    for(unsigned page=0;page<count;page++){
     auto* bytes=reinterpret_cast<uint8_t*>(upload->data+page*1024);
     if(!Current() || !source.ReadWeightPage(part,at+page,bytes,4096) || !Current())return Stop("source_changed");
     // Token0 needs ONLY its46080-byte tile. The last physical page must
     // not upload the next embedding tile's original bytes as padding.
     if(part==embedding && at+page==11)std::memset(bytes+1024,0,3072);
    }
    if(!Call(TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,*upload))return Stop("upload");
    const auto& r=upload->result;
    if(r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.bytes!=count*4096 || r.pages!=count || r.nonce!=upload_spans+1
     || r.started!=1 || r.completed!=1 || r.held!=1 || r.host_exact || r.matched_words || r.recovery_required || r.reserved)return Stop("upload_reply");
    at+=count;uploaded_pages+=count;upload_spans++;
   }
  }
  if(uploaded_pages!=pages || upload_spans!=spans)return Stop("upload_count");
  tt_q38_span_end spanEnd={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,spanEnd) || spanEnd.version!=1 || spanEnd.reserved || spanEnd.closed!=1 || spanEnd.recovery_required)return Stop("span_end");
  auto reply=std::make_unique<tt_q38_mixed_live_validation>();reply->metadata.version=1;
  bool called=Call(TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE,*reply);
  std::memcpy(reply_header,reply.get(),sizeof(reply_header));reply_captured=true;
  if(!called)return Stop("compute_transport");
  result=reply->metadata;
  const uint32_t expected[64]={1,0,0,0,TT_RUNTIME_PASSED,1,1,0,0,0,uint32_t(generation),uint32_t(generation>>32),0,0,
   63266,512,11472449,11472449,2,1,1,119,119,120,1,1,4,11398712,22797424,2630395,11398712,5,5120,5120};
  for(unsigned i=0;i<64;i++){
   if(i==8 || i==12){if(!(reply_header[i]|reply_header[i+1]))return Stop("reply_identity_time");i++;continue;}
   if(reply_header[i]!=expected[i])return Stop("reply_metadata");
  }
  if(!source.CompareFinal(reply->values,5120,&comparison) || !Current() || comparison.checked!=5120
   || comparison.passed!=1 || comparison.differences || comparison.nonfinite || comparison.maxAbsolute!=0 || comparison.relativeL2!=0)return Stop("original_values");
  validated=true;return true;
 }
 bool Finish(){
  if(!active || !validated || failed || finished || fd<0)return false;
  tt_q38_bank_session end={};end.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,end) || end.result.version!=1 || end.result.state!=TT_RESIDENT_SESSION_CLOSED
   || end.result.active || end.result.cold || end.result.pci_restored!=1 || end.result.host_released!=1
   || end.result.capabilities || end.result.bytes!=PioneerResidentBanks::Total)return Stop("end");
  active=validated=false;finished=true;int owned=fd;fd=-1;
  if(io.Close(owned)!=0){failure="close";failed=true;return false;}return true;
 }
 void Abort(){Stop("aborted");} // CLOSE ONLY;never END,reset,another peek or retry
};
#endif
