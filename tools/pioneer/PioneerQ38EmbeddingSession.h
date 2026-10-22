/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EMBEDDING_SESSION_H
#define PIONEER_Q38_EMBEDDING_SESSION_H
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT
#if PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT != 1 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1 || defined(PIONEER_Q38_EMBEDDING_CLIENT) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_EMBEDDING_NATIVE)
#error Separate DEBUG embedding validation client only;NO Work/native promotion
#endif
#elif !defined(PIONEER_Q38_EMBEDDING_CLIENT) || PIONEER_Q38_EMBEDDING_CLIENT != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_EMBEDDING_NATIVE)
#error Separate source-bound embedding client only;NO old Work/CPU-reference/native role promotion
#endif
#include "PioneerQ38PublicModelSource.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <memory>
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT
#include "PioneerQ38EmbeddingValidationReference.h"
#endif
// One source-bound learned embedding;NO numeric result/CPU reference.
// Same fully authenticated FILE/epoch before and after EVERY device call.
// ALL pages uploaded BEFORE binding;close-only after any uncertain result.
template<class IO,class Source=PioneerQ38PublicModelSource>class PioneerQ38EmbeddingSession {
 IO& io;Source& source;PioneerQwen38PackedPlan plan;
 int fd=-1;bool attempted=false,failed=false,active=false,finished=false;
 uint64_t epoch=0,generation=0;unsigned token=0,position=0,fragment=UINT32_MAX,firstPage=0;
 bool Current()const{return source.Ready() && source.Model().Epoch()==epoch && plan.MatchesSource(source.Model());}
 bool Stop(){failed=true;active=false;finished=true;if(fd>=0){io.Close(fd);fd=-1;}return false;}
 template<class T>bool Call(unsigned op,T& data){return Current() && io.Call(fd,op,&data,sizeof(data)) && Current();}
public:
 tt_q38_embedding_produce result={};
 PioneerQ38EmbeddingSession(IO& transport,Source& model):io(transport),source(model){}
 ~PioneerQ38EmbeddingSession(){if(fd>=0)io.Close(fd);} // NO implicit END/retry/numeric fallback
 PioneerQ38EmbeddingSession(const PioneerQ38EmbeddingSession&)=delete;
 PioneerQ38EmbeddingSession& operator=(const PioneerQ38EmbeddingSession&)=delete;
 bool Produce(unsigned selectedToken,unsigned selectedPosition){
  if(attempted || failed || finished || selectedToken>=248320 || selectedPosition>=8192)return false;
  attempted=true;token=selectedToken;position=selectedPosition;
  if(!source.Ready() || !plan.Build(source.Model(),8192,true,true))return Stop();
  epoch=source.Model().Epoch();
  const auto& tensors=source.Model().Tensors();const auto& parts=plan.Parts();
  for(unsigned i=0;i<parts.size();i++){
   const auto& p=parts[i];if(p.kind!=PIONEER_Q38_WEIGHT || p.tensor>=tensors.size())continue;
   const auto& t=tensors[p.tensor];if(t.name!="token_embd.weight")continue;
   if(t.type!=12 || t.inputs!=5120 || t.outputs!=248320 || p.rowBytes!=2880)return Stop();
   if(token>=p.firstRow && token-p.firstRow<p.rows){
    if(fragment!=UINT32_MAX)return Stop();
    fragment=i;firstPage=((token-p.firstRow)/16*46080)/4096;
   }
  }
  if(fragment==UINT32_MAX || !Current())return Stop();
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT
  std::vector<uint32_t> reference(5120);
  if(!PioneerQ38EmbeddingReference(source,parts[fragment].tensor,token,reference.data()) || !Current())return Stop();
  constexpr uint64_t expectedTag=UINT64_C(0x2026102200010040);
  constexpr unsigned expectedNamespace=11,referenceFree=0,supportOperation=TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT;
#else
  constexpr uint64_t expectedTag=UINT64_C(0x2026102200010039);
  constexpr unsigned expectedNamespace=10,referenceFree=1,supportOperation=TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT;
#endif
  fd=io.Open();if(fd<0)return Stop();
  tt_pioneer_driver_reload reload={};reload.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,reload) || reload.version!=1 || reload.driver!=55 || reload.guard_version!=1
   || reload.build_tag!=expectedTag || !reload.generation || reload.quiesced || reload.cold || reload.active_owner || reload.open_count!=1)return Stop();
  generation=reload.generation;
  tt_q38_embedding_support support={};
  if(!Call(supportOperation,support) || support.version!=1 || support.driver!=55 || support.model_namespace!=expectedNamespace
   || support.inputs!=5120 || support.vocabulary!=248320 || support.runtime_pages!=512 || support.no_cpu_reference!=referenceFree || support.model_inference)return Stop();
  tt_q38_bank_support storage={};tt_q38_span_support span={};
  if(!Call(TT_PCI_PROBE_Q38_BANK_SUPPORT,storage) || storage.driver!=45 || storage.version!=1 || storage.banks!=7 || storage.page_bytes!=4096
   || storage.max_pages!=4 || storage.storage_only!=1 || storage.bytes_per_bank!=PioneerResidentBanks::Capacity
   || !Call(TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,span) || span.driver!=55 || span.version!=1 || span.banks!=7 || span.page_bytes!=4096
   || span.max_bytes!=1048576 || span.before_cohort!=1 || span.explicit_end!=1 || span.reserved)return Stop();
  tt_q38_bank_session begin={};begin.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_BEGIN,begin) || begin.result.version!=1 || begin.result.state!=TT_RESIDENT_SESSION_READY
   || begin.result.active!=1 || begin.result.host_ready!=1 || begin.result.cold || begin.result.capabilities || begin.result.bytes!=PioneerResidentBanks::Total)return Stop();
  active=true;auto upload=std::make_unique<tt_q38_span_upload>();const auto& p=parts[fragment];
  upload->version=1;upload->bank=p.bank;upload->bytes=12*4096;upload->offset=uint64_t(p.proposedAddress)-plan.Base+uint64_t(firstPage)*4096;
  for(unsigned page=0;page<12;page++)if(!source.ReadWeightPage(plan,fragment,firstPage+page,reinterpret_cast<uint8_t*>(upload->data+page*1024),4096) || !Current())return Stop();
  if(!Call(TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,*upload))return Stop();
  const auto& u=upload->result;
  if(u.version!=1 || u.state!=TT_RUNTIME_PASSED || u.bytes!=49152 || u.pages!=12 || u.nonce!=1 || u.started!=1 || u.completed!=1
   || u.held!=1 || u.host_exact || u.matched_words || u.recovery_required || u.reserved)return Stop();
  tt_q38_span_end end={1,0,0,0};if(!Call(TT_PCI_PROBE_Q38_SPAN_END,end) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required)return Stop();
  result.version=1;result.token=token;result.position=position;
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT
  auto validation=std::make_unique<tt_q38_embedding_validation>();validation->metadata=result;
  if(!Call(TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE,*validation))return Stop();
  result=validation->metadata;
#else
  if(!Call(TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE,result))return Stop();
#endif
  if(result.version!=1 || result.token!=token || result.position!=position || result.reserved
   || result.state!=TT_RUNTIME_PASSED || result.closed!=1 || result.input_ready!=1 || result.recovery_required || !result.session
   || result.driver_generation!=generation || !result.elapsed_us || result.source_pages!=12 || result.workspace_pages!=512
   || result.saved_words!=73737 || result.restored_words!=73737)return Stop();
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT
  if(memcmp(validation->values,reference.data(),20480) || !Current())return Stop(); // first numeric mismatch CLOSE ONLY,no END/substitution
#endif
  return true; // GDDR input private to SAME native context;not CPU vector or decoder output
 }
 bool Finish(){
  if(!active || failed || finished || fd<0)return false;
  tt_q38_bank_session end={};end.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,end) || end.result.version!=1 || end.result.state!=TT_RESIDENT_SESSION_CLOSED
   || end.result.active || end.result.cold || end.result.pci_restored!=1 || end.result.host_released!=1 || end.result.capabilities || end.result.bytes!=PioneerResidentBanks::Total)return Stop();
  active=false;finished=true;int owned=fd;fd=-1;return io.Close(owned)==0;
 }
 void Abort(){Stop();}
};
#endif
