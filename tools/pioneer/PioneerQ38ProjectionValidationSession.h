/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_VALIDATION_SESSION_H
#define PIONEER_Q38_PROJECTION_VALIDATION_SESSION_H
#if !defined(PIONEER_Q38_PROJECTION_VALIDATION_CLIENT) || PIONEER_Q38_PROJECTION_VALIDATION_CLIENT != 1 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1 || !defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE) || PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE != 1 || defined(PIONEER_Q38_EMBEDDING_CLIENT) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT) || defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(_KERNEL_MODE)
#error Separate source-bound projection DEBUG client ONLY,never Work/native/old embedding role
#endif
#include "PioneerQ38PublicModelSource.h"
#include "PioneerQ38EmbeddingValidationReference.h"
#include "PioneerQ38LearnedRmsValidationReference.h"
#include "PioneerQ38ProjectionValidationReference.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionValidationProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <memory>
// Independent embedding/RMS/ALL4projection references BEFORE FD. Upload
// ONLY original tiled embedding/norm/16 learned stripes,never these CPU
// products. Readback is qualification,not inference or a numeric fallback.
template<class IO,class Source=PioneerQ38PublicModelSource>class PioneerQ38ProjectionValidationSession {
 IO& io;Source& source;PioneerQwen38PackedPlan plan;
 int fd=-1;bool attempted=false,failed=false,active=false,finished=false;
 uint64_t epoch=0,generation=0;
 bool Current()const{return source.Ready() && source.Model().Epoch()==epoch && plan.MatchesSource(source.Model());}
 bool Stop(){failed=true;active=false;finished=true;if(fd>=0){io.Close(fd);fd=-1;}return false;}
 template<class T>bool Call(unsigned op,T& value){return Current() && io.Call(fd,op,&value,sizeof(value)) && Current();}
public:
 tt_q38_embedding_produce result={};
 PioneerQ38ProjectionValidationSession(IO& transport,Source& model):io(transport),source(model){}
 ~PioneerQ38ProjectionValidationSession(){if(fd>=0)io.Close(fd);} // NO automatic END/retry/fallback
 PioneerQ38ProjectionValidationSession(const PioneerQ38ProjectionValidationSession&)=delete;
 PioneerQ38ProjectionValidationSession& operator=(const PioneerQ38ProjectionValidationSession&)=delete;
 bool Validate(unsigned token,unsigned position){
  if(attempted || failed || finished || token>=248320 || position>=8192)return false;
  attempted=true;if(!source.Ready() || !plan.Build(source.Model(),8192,true,true))return Stop();
  epoch=source.Model().Epoch();const auto& parts=plan.Parts();const auto& tensors=source.Model().Tensors();
  unsigned embedding=UINT32_MAX,norm=UINT32_MAX,firstPage=0,stripes=0,totalPages=12;
  std::vector<unsigned> selected;
  for(unsigned i=0;i<parts.size();i++){
   const auto& p=parts[i];if(p.kind!=PIONEER_Q38_WEIGHT || p.tensor>=tensors.size())continue;
   const auto& t=tensors[p.tensor];
   if(t.name=="token_embd.weight" && token>=p.firstRow && token-p.firstRow<p.rows){
    if(embedding!=UINT32_MAX || t.type!=12 || t.inputs!=5120 || t.outputs!=248320 || p.rowBytes!=2880)return Stop();
    embedding=i;firstPage=((token-p.firstRow)/16*46080)/4096;
   }
   if(p.layer || (p.role!=0 && p.role!=5 && p.role!=6 && p.role!=10 && p.role!=11))continue;
   if(!p.allocated || (p.allocated&4095) || p.proposedAddress<plan.Base)return Stop();
   if(!p.role){
    if(norm!=UINT32_MAX || t.name!="blk.0.attn_norm.weight" || t.type || t.rank!=1 || t.inputs!=5120 || t.outputs!=1
     || p.firstRow || p.rows!=1 || p.rowBytes!=20480 || p.bytes!=20480 || p.allocated!=20480)return Stop();
    norm=i;
   }else{
    PioneerQ38Tensor expected;
    if(!PioneerQ38Expected(0,p.role,expected) || t.name!=expected.name || t.type!=8 || t.rank!=2 || t.inputs!=5120
     || t.outputs!=PioneerQ38ProjectionResultRows(p.role) || p.rowBytes!=5760)return Stop();
    stripes++;
   }
   selected.push_back(i);totalPages+=p.allocated/4096;
  }
  if(embedding==UINT32_MAX || norm==UINT32_MAX || selected.size()!=17 || stripes!=16 || totalPages!=23193 || !Current())return Stop();
  std::vector<uint32_t> raw(5120),normalized(5120),reference(18432,0);
  if(!PioneerQ38EmbeddingReference(source,parts[embedding].tensor,token,raw.data()) || !Current()
   || !PioneerQ38LearnedRmsReference(source,parts[norm].tensor,raw.data(),normalized.data()) || !Current())return Stop();
  const unsigned roles[]={10,11,5,6};
  for(unsigned role:roles)if(!PioneerQ38ProjectionReference(source,role,normalized.data(),5120,
   reference.data()+(PioneerQ38ProjectionResultOffset(role)-TT_Q38_PROJECTION_RESULT_BASE)/4,PioneerQ38ProjectionResultRows(role)) || !Current())return Stop();
  fd=io.Open();if(fd<0)return Stop();
  tt_pioneer_driver_reload reload={};reload.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,reload) || reload.version!=1 || reload.driver!=55 || reload.guard_version!=1
   || reload.build_tag!=UINT64_C(0x2026102200010042) || !reload.generation || reload.quiesced || reload.cold
   || reload.active_owner || reload.open_count!=1)return Stop();
  generation=reload.generation;tt_q38_embedding_support support={};
  if(!Call(TT_PCI_PROBE_Q38_PROJECTION_VALIDATION_SUPPORT,support) || support.version!=1 || support.driver!=55
   || support.model_namespace!=13 || support.inputs!=16480 || support.vocabulary!=248320 || support.runtime_pages!=512
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
  active=true;auto upload=std::make_unique<tt_q38_span_upload>();unsigned nonce=0;
  selected.insert(selected.begin(),embedding);
  for(unsigned part:selected){
   const auto& p=parts[part];unsigned first=part==embedding?firstPage:0,pages=part==embedding?12:p.allocated/4096;
   for(unsigned at=0;at<pages;){
    unsigned count=pages-at<256?pages-at:256;
    memset(upload.get(),0,sizeof(*upload));upload->version=1;upload->bank=p.bank;upload->bytes=count*4096;
    upload->offset=uint64_t(p.proposedAddress)-plan.Base+uint64_t(first+at)*4096;
    for(unsigned page=0;page<count;page++)if(!source.ReadWeightPage(plan,part,first+at+page,
     reinterpret_cast<uint8_t*>(upload->data+page*1024),4096) || !Current())return Stop();
    if(!Call(TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,*upload))return Stop();
    const auto& r=upload->result;
    if(r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.bytes!=count*4096 || r.pages!=count || r.nonce!=++nonce
     || r.started!=1 || r.completed!=1 || r.held!=1 || r.host_exact || r.matched_words || r.recovery_required || r.reserved)return Stop();
    at+=count;
   }
  }
  tt_q38_span_end end={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,end) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required)return Stop();
  auto reply=std::make_unique<tt_q38_projection_validation>();reply->metadata.version=1;reply->metadata.token=token;reply->metadata.position=position;
  if(!Call(TT_PCI_PROBE_Q38_PROJECTION_VALIDATE,*reply))return Stop();
  result=reply->metadata;
  if(result.version!=1 || result.token!=token || result.position!=position || result.reserved
   || result.state!=TT_RUNTIME_PASSED || result.closed!=1 || result.input_ready!=1 || result.recovery_required
   || !result.session || result.driver_generation!=generation || !result.elapsed_us || result.source_pages!=23193
   || result.workspace_pages!=512 || result.saved_words!=8281546 || result.restored_words!=8281546
   || reply->projections!=4 || reply->peer_boots!=119 || reply->peer_stopped!=119 || reply->peer_held!=119
   || reply->root_starts!=5 || reply->root_holds!=5 || reply->readback_pages!=18 || reply->readback_words!=18432
   || reply->saved_words!=8134072 || reply->verified_words!=16268144 || reply->installed_words!=989304 || reply->restored_words!=8134072)return Stop();
  if(memcmp(reply->values,reference.data(),73728) || !Current())return Stop(); // padding too,not only sampled outputs
  return true;
 }
 bool Finish(){
  if(!active || failed || finished || fd<0)return false;
  tt_q38_bank_session end={};end.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,end) || end.result.version!=1 || end.result.state!=TT_RESIDENT_SESSION_CLOSED
   || end.result.active || end.result.cold || end.result.pci_restored!=1 || end.result.host_released!=1
   || end.result.capabilities || end.result.bytes!=PioneerResidentBanks::Total)return Stop();
  active=false;finished=true;int owned=fd;fd=-1;return io.Close(owned)==0;
 }
 void Abort(){Stop();}
};
#endif
