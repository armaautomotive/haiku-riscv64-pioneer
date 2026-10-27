/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE)
#error New recurrent ROOT/capsule is not a CPU reference or validation client
#endif
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE
#error Recurrent projection image is separate from CPU validation
#endif
#if defined(PIONEER_Q38_DENSE_PREPARE_IMAGE) || defined(PIONEER_Q38_DENSE_ATTENTION_IMAGE) || defined(PIONEER_Q38_DENSE_PROJECTION_IMAGE) || defined(PIONEER_Q38_DENSE_PARTITION_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) || defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE)
#error Local dense-prepare IMAGE is not a CPU DEBUG69 client
#endif
#ifdef PIONEER_Q38_RESIDENT_CONTINUE_IMAGE
#error Continuation IMAGE has no existing native/CPU/Work/component role
#endif
#ifdef PIONEER_Q38_RESIDENT_ADVANCE_IMAGE
#error On-card advance IMAGE is not a CPU DEBUG69 client
#endif
// Gates precede the include guard:late mixed-role inclusion MUST fail.
#ifdef PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE
#error On-card link IMAGE is not a CPU DEBUG69 client
#endif
#if !defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT) || PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT != 1 \
 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1
#error Exact separate CPU DEBUG69 client required
#endif
#if defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_OWNER) || defined(PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE) || defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE) || defined(PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE) \
 || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE) \
 || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) \
 || defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_GDN_GATED_NATIVE) \
 || defined(PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_ATTENTION_FREESTANDING) \
 || defined(_KERNEL_MODE) || defined(__FAST_MATH__)
#error CPU DEBUG69 client cannot alias native,image,old client,kernel or Work roles,including defined0
#endif
#ifndef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_SESSION_H
#define PIONEER_Q38_RESIDENT_LAYER_VALIDATION_SESSION_H
#include "PioneerQ38OriginalSource.h"
#include "PioneerQ38ResidentLayerComparison.h"
#include "PioneerQwen38PackedPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerValidationProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <cstring>
// Fixed token0/position0/layer0 diagnostic. SAME authenticated original
// file supplies references and upload bytes. No caller numeric inputs,
// physical addresses,readiness/owner flags,Work route or inference claim.
template<class IO,class Source=DecoderOriginalSource>class PioneerQ38ResidentLayerValidationSession {
public:
 enum class Failure {None,SourcePreparation,SessionContract,SourceChanged,Transport,ReplyMetadata,OriginalValues,EndMetadata,Close};
 struct Diagnostics {
  Failure reason=Failure::None;unsigned op=0,word=UINT32_MAX;
  uint32_t expected=0,actual=0,reply_header[64]={};bool reply_captured=false;
 } diagnostics;
 static const char* FailureName(Failure r){
  switch(r){
   case Failure::None:return "none";case Failure::SourcePreparation:return "source_preparation";
   case Failure::SessionContract:return "session_contract";case Failure::SourceChanged:return "source_changed";
   case Failure::Transport:return "transport";case Failure::ReplyMetadata:return "reply_metadata";
   case Failure::OriginalValues:return "original_values";case Failure::EndMetadata:return "end_metadata";
   case Failure::Close:return "close";
  }return "unknown";
 }
private:
 IO& io;Source& source;PioneerQwen38PackedPlan plan;const PioneerQwen38Gguf* model=nullptr;
 int fd=-1;bool attempted=false,failed=false,active=false,finished=false,validated=false;
 uint64_t epoch=0,generation=0;
 bool Current()const{return model && source.Ready() && source.SealedModel()==model && model->Epoch()==epoch && plan.MatchesSource(*model);}
 bool Record(Failure why,unsigned word=UINT32_MAX,uint32_t expected=0,uint32_t actual=0){
  if(diagnostics.reason==Failure::None){diagnostics.reason=why;diagnostics.word=word;diagnostics.expected=expected;diagnostics.actual=actual;}return false;
 }
 bool Stop(Failure why=Failure::SessionContract,unsigned word=UINT32_MAX,uint32_t expected=0,uint32_t actual=0){
  Record(why,word,expected,actual);failed=true;active=validated=false;finished=true;
  if(fd>=0){int owned=fd;fd=-1;io.Close(owned);}return false;
 }
 template<class T>bool Call(unsigned op,T& value){
  if(failed || finished || fd<0)return false;
  diagnostics.op=op;
  if(!Current())return Record(Failure::SourceChanged);
  if(!io.Call(fd,op,&value,sizeof(value)))return Record(Failure::Transport);
  return Current() || Record(Failure::SourceChanged);
 }
public:
 tt_q38_embedding_produce result={};unsigned uploaded_pages=0,upload_spans=0;
 PioneerQ38ResidentLayerComparison idealComparison,instructionComparison;
 PioneerQ38ResidentLayerValidationSession(IO& transport,Source& original):io(transport),source(original){}
 ~PioneerQ38ResidentLayerValidationSession(){if(fd>=0)io.Close(fd);} // CLOSE ONLY;never implicit END/reset/retry
 PioneerQ38ResidentLayerValidationSession(const PioneerQ38ResidentLayerValidationSession&)=delete;
 PioneerQ38ResidentLayerValidationSession& operator=(const PioneerQ38ResidentLayerValidationSession&)=delete;
 bool Validate(unsigned token=0,unsigned position=0){
  if(attempted || failed || finished || token || position)return false;
  attempted=true;
  // Entire original-GGUF authentication/reference preparation/seal precedes
  // ANY accelerator FD. A failed/changed source is never retried/reopened.
  if(!source.Prepare(0) || !source.SealBeforeNativeContext() || !source.FfnWeightsReady() || !source.ResidentLayerReferenceReady() || source.ReferenceReads()!=4810
   || !(model=source.SealedModel()) || !source.Ready() || !plan.Build(*model,8192,true,true))return Stop(Failure::SourcePreparation);
  epoch=model->Epoch();const auto& parts=plan.Parts();const auto& tensors=model->Tensors();
  unsigned embedding=UINT32_MAX,totalPages=12,expectedSpans=1;
  std::vector<unsigned> selected;
  for(unsigned i=0;i<parts.size();i++){
   const auto& p=parts[i];if(p.kind!=PIONEER_Q38_WEIGHT || p.tensor>=tensors.size())continue;
   const auto& t=tensors[p.tensor];
   if(t.name=="token_embd.weight" && p.firstRow==0){
    if(embedding!=UINT32_MAX || p.layer!=64 || p.role || t.type!=12 || t.inputs!=5120 || t.outputs!=248320
     || p.rowBytes!=2880 || p.allocated<49152)return Stop();
    embedding=i;
   }
   if(p.layer || p.role>13)continue;
   if(p.bank>=7 || p.proposedAddress<plan.Base || !p.allocated || (p.allocated&4095)
    || uint64_t(p.proposedAddress)-plan.Base+p.allocated>plan.Capacity)return Stop();
   selected.push_back(i);totalPages+=p.allocated/4096;expectedSpans+=(p.allocated/4096+255)/256;
  }
  if(embedding==UINT32_MAX || selected.size()!=56 || totalPages!=68603 || expectedSpans!=300 || !Current())return Stop();
  selected.insert(selected.begin(),embedding);
  fd=io.Open();if(fd<0)return Stop(Failure::Transport);
  tt_pioneer_driver_reload reload={};reload.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,reload) || reload.version!=1 || reload.driver!=55 || reload.guard_version!=1
   || reload.build_tag!=UINT64_C(0x2026102600010069) || !reload.generation || reload.quiesced || reload.cold
   || reload.active_owner || reload.open_count!=1)return Stop();
  generation=reload.generation;tt_q38_embedding_support support={};
  // Both inherited driver fields are55;the EXACT build tag identifies69.
  // Namespace20/word count additionally forbid old DEBUG55 aliasing.
  if(!Call(TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATION_SUPPORT,support) || support.version!=1 || support.driver!=55
   || support.model_namespace!=20 || support.inputs!=1630208 || support.vocabulary!=248320 || support.runtime_pages!=512
   || support.no_cpu_reference || support.model_inference)return Stop();
  tt_q38_bank_support storage={};tt_q38_span_support span={};
  if(!Call(TT_PCI_PROBE_Q38_BANK_SUPPORT,storage) || storage.driver!=45 || storage.version!=1 || storage.banks!=7
   || storage.page_bytes!=4096 || storage.max_pages!=4 || storage.storage_only!=1 || storage.bytes_per_bank!=PioneerResidentBanks::Capacity
   || !Call(TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,span) || span.driver!=55 || span.version!=1 || span.banks!=7
   || span.page_bytes!=4096 || span.max_bytes!=1048576 || span.before_cohort!=1 || span.explicit_end!=1 || span.reserved)return Stop();
  tt_q38_bank_session begin={};begin.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_BEGIN,begin) || begin.result.version!=1 || begin.result.state!=TT_RESIDENT_SESSION_READY
   || begin.result.active!=1 || begin.result.host_ready!=1 || begin.result.cold || begin.result.capabilities
   || begin.result.bytes!=PioneerResidentBanks::Total)return Stop();
  active=true;auto upload=std::make_unique<tt_q38_span_upload>();
  for(unsigned part:selected){
   const auto& p=parts[part];unsigned pages=part==embedding?12:p.allocated/4096;
   for(unsigned at=0;at<pages;){
    unsigned count=pages-at<256?pages-at:256;
    std::memset(upload.get(),0,sizeof(*upload));upload->version=1;upload->bank=p.bank;upload->bytes=count*4096;
    upload->offset=uint64_t(p.proposedAddress)-plan.Base+uint64_t(at)*4096;
    for(unsigned page=0;page<count;page++)if(!source.ReadWeightPage(plan,part,at+page,
     reinterpret_cast<uint8_t*>(upload->data+page*1024),4096) || !Current())return Stop(Failure::SourceChanged);
    if(!Call(TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,*upload))return Stop();
    const auto& r=upload->result;
    if(r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.bytes!=count*4096 || r.pages!=count || r.nonce!=upload_spans+1
     || r.started!=1 || r.completed!=1 || r.held!=1 || r.host_exact || r.matched_words || r.recovery_required || r.reserved)return Stop();
    at+=count;uploaded_pages+=count;upload_spans++;
   }
  }
  if(upload_spans!=expectedSpans || uploaded_pages!=totalPages)return Stop();
  tt_q38_span_end end={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,end) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required)return Stop();
  auto reply=std::make_unique<tt_q38_resident_layer_validation>();reply->metadata.version=1;
  bool called=Call(TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATE,*reply);
  std::memcpy(diagnostics.reply_header,reply.get(),sizeof(diagnostics.reply_header));diagnostics.reply_captured=true;
  if(!called)return Stop();
  result=reply->metadata;
  const uint32_t expectedHeader[64]={
   1,0,0,0,TT_RUNTIME_PASSED,1,1,0,0,0,uint32_t(generation),uint32_t(generation>>32),0,0,68603,512,19282434,19282434,
   672,73728,336,1680,4,119,5,5,119,1,1,119,119,640,96,24,5120,1592,1630208,0,1,
   11000888,22001776,2270704,11000888,8134072,16268144,989304,8134072,357,88,5120
  };
  for(unsigned i=0;i<64;i++){
   if(i==8 || i==12){
    if(!(diagnostics.reply_header[i]|diagnostics.reply_header[i+1]))return Stop(Failure::ReplyMetadata,i,1,0);
    i++;continue;
   }
   if(diagnostics.reply_header[i]!=expectedHeader[i])return Stop(Failure::ReplyMetadata,i,expectedHeader[i],diagnostics.reply_header[i]);
  }
  // Post-return ONLY. No CPU numeric products ever enter upload/requests.
  if(!source.CompareResidentLayer(reply->values,TT_Q38_RESIDENT_LAYER_VALIDATION_WORDS,&idealComparison)
   || !source.CompareResidentLayerInstruction(reply->values,TT_Q38_RESIDENT_LAYER_VALIDATION_WORDS,&instructionComparison)
   || idealComparison.version!=1 || idealComparison.checked!=TT_Q38_RESIDENT_LAYER_VALIDATION_WORDS || idealComparison.reserved
   || instructionComparison.version!=1 || instructionComparison.checked!=TT_Q38_RESIDENT_LAYER_VALIDATION_WORDS
   || instructionComparison.passed!=1 || instructionComparison.reserved)return Stop(Failure::OriginalValues);
  // Ideal libm report remains DISTINCT,not relabelled as a bit-equality pass.
  // The policy was independently bounded against libm BEFORE this native FD.
  if(!Current())return Stop(Failure::SourceChanged);
  validated=true;return true;
 }
 bool Finish(){
  if(!active || !validated || failed || finished || fd<0)return false;
  tt_q38_bank_session end={};end.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,end) || end.result.version!=1 || end.result.state!=TT_RESIDENT_SESSION_CLOSED
   || end.result.active || end.result.cold || end.result.pci_restored!=1 || end.result.host_released!=1
   || end.result.capabilities || end.result.bytes!=PioneerResidentBanks::Total)return Stop(Failure::EndMetadata);
  active=validated=false;finished=true;int owned=fd;fd=-1;return io.Close(owned)==0 || Record(Failure::Close);
 }
 void Abort(){Stop();} // Close ONLY;no END/reset/readback/retry after uncertainty.
};
#endif
