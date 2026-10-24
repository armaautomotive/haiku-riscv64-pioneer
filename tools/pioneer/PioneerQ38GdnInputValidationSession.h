/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_INPUT_VALIDATION_SESSION_H
#define PIONEER_Q38_GDN_INPUT_VALIDATION_SESSION_H
#include "PioneerQ38GdnInputValidationReference.h"
#include "PioneerQ38PublicModelSource.h"
#include "PioneerQ38EmbeddingValidationReference.h"
#include "PioneerQ38LearnedRmsValidationReference.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnInputValidationProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
// Exact source-bound DEBUG46 session. ALL references BEFORE FD,original
// tiled weight bytes ONLY on upload. Not inference or a Work fallback.
template<class IO,class Source=PioneerQ38PublicModelSource>class PioneerQ38GdnInputValidationSession {
 IO& io;Source& source;PioneerQwen38PackedPlan plan;
 int fd=-1;bool attempted=false,failed=false,active=false,finished=false;
 uint64_t epoch=0,generation=0;
 bool Current()const{return source.Ready() && source.Model().Epoch()==epoch && plan.MatchesSource(source.Model());}
 bool Stop(){failed=true;active=false;finished=true;if(fd>=0){io.Close(fd);fd=-1;}return false;}
 template<class T>bool Call(unsigned op,T& value){return Current() && io.Call(fd,op,&value,sizeof(value)) && Current();}
public:
 tt_q38_embedding_produce result={};
 PioneerQ38GdnInputValidationSession(IO& transport,Source& model):io(transport),source(model){}
 ~PioneerQ38GdnInputValidationSession(){if(fd>=0)io.Close(fd);} // CLOSE ONLY,no implicit END/reset/retry
 PioneerQ38GdnInputValidationSession(const PioneerQ38GdnInputValidationSession&)=delete;
 PioneerQ38GdnInputValidationSession& operator=(const PioneerQ38GdnInputValidationSession&)=delete;
 bool Validate(unsigned token,unsigned position=0){
  if(attempted || failed || finished || token>=248320 || position)return false;
  attempted=true;if(!source.Ready() || !plan.Build(source.Model(),8192,true,true))return Stop();
  epoch=source.Model().Epoch();const auto& parts=plan.Parts();const auto& tensors=source.Model().Tensors();
  unsigned embedding=UINT32_MAX,norm=UINT32_MAX,firstPage=0,stripes=0,convStripes=0,convRows=0,learned=0,totalPages=12;
  std::vector<unsigned> selected;
  for(unsigned i=0;i<parts.size();i++){
   const auto& p=parts[i];if(p.kind!=PIONEER_Q38_WEIGHT || p.tensor>=tensors.size())continue;
   const auto& t=tensors[p.tensor];
   if(t.name=="token_embd.weight" && token>=p.firstRow && token-p.firstRow<p.rows){
    if(embedding!=UINT32_MAX || t.type!=12 || t.inputs!=5120 || t.outputs!=248320 || p.rowBytes!=2880)return Stop();
    embedding=i;firstPage=((token-p.firstRow)/16*46080)/4096;
   }
   if(p.layer || (p.role!=0 && p.role!=5 && p.role!=6 && p.role!=7 && p.role!=8 && p.role!=10 && p.role!=11 && p.role!=12))continue;
   if(!p.allocated || (p.allocated&4095) || p.proposedAddress<plan.Base)return Stop();
   if(!p.role){
    if(norm!=UINT32_MAX || t.name!="blk.0.attn_norm.weight" || t.type || t.rank!=1 || t.inputs!=5120 || t.outputs!=1
     || p.firstRow || p.rows!=1 || p.rowBytes!=20480 || p.bytes!=20480 || p.allocated!=20480)return Stop();
    norm=i;
   }else if(p.role==7 || p.role==8){
    if(t.name!=(p.role==7?"blk.0.ssm_a":"blk.0.ssm_dt.bias") || t.type || t.rank!=1 || t.inputs!=48 || t.outputs!=1
     || t.bytes!=192 || p.firstRow || p.rows!=1 || p.rowBytes!=192 || p.bytes!=192 || p.allocated!=4096)return Stop();
    learned++;
   }else if(p.role==12){
    if(t.name!="blk.0.ssm_conv1d.weight" || t.type || t.rank!=2 || t.inputs!=4 || t.outputs!=10240 || t.bytes!=163840
     || p.rowBytes!=16 || !p.rows || (p.rows&15) || (p.firstRow&15) || p.firstRow>=10240 || p.rows>10240-p.firstRow
     || p.bytes!=p.rows*16 || p.allocated!=24576)return Stop();
    convStripes++;convRows+=p.rows;
   }else{
    PioneerQ38Tensor expected;
    if(!PioneerQ38Expected(0,p.role,expected) || t.name!=expected.name || t.type!=8 || t.rank!=2 || t.inputs!=5120
     || t.outputs!=PioneerQ38ProjectionResultRows(p.role) || p.rowBytes!=5760)return Stop();
    stripes++;
   }
   selected.push_back(i);totalPages+=p.allocated/4096;
  }
  if(embedding==UINT32_MAX || norm==UINT32_MAX || selected.size()!=26 || stripes!=16 || convStripes!=7
   || convRows!=10240 || learned!=2 || totalPages!=23237 || !Current())return Stop();
  std::vector<uint32_t> raw(5120),normalized(5120),qkv(10240),conv(40960),alpha(48),beta(48),reference(30912);
  if(!PioneerQ38EmbeddingReference(source,parts[embedding].tensor,token,raw.data()) || !Current()
   || !PioneerQ38LearnedRmsReference(source,parts[norm].tensor,raw.data(),normalized.data()) || !Current()
   || !PioneerQ38ProjectionReference(source,6,normalized.data(),5120,qkv.data(),10240) || !Current()
   || !PioneerQ38ResidentDecoderReference(source,qkv.data(),10240,conv.data(),40960) || !Current()
   || !PioneerQ38ProjectionReference(source,10,normalized.data(),5120,alpha.data(),48) || !Current()
   || !PioneerQ38ProjectionReference(source,11,normalized.data(),5120,beta.data(),48) || !Current()
   || !PioneerQ38GdnInputReference(source,conv.data(),40960,alpha.data(),beta.data(),48,reference.data(),30912) || !Current())return Stop();
  fd=io.Open();if(fd<0)return Stop();
  tt_pioneer_driver_reload reload={};reload.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,reload) || reload.version!=1 || reload.driver!=55 || reload.guard_version!=1
   || reload.build_tag!=UINT64_C(0x2026102300010046) || !reload.generation || reload.quiesced || reload.cold
   || reload.active_owner || reload.open_count!=1)return Stop();
  generation=reload.generation;tt_q38_embedding_support support={};
  if(!Call(TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATION_SUPPORT,support) || support.version!=1 || support.driver!=55
   || support.model_namespace!=15 || support.inputs!=30912 || support.vocabulary!=248320 || support.runtime_pages!=512
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
  if(nonce!=110)return Stop();
  tt_q38_span_end end={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,end) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required)return Stop();
  auto reply=std::make_unique<tt_q38_gdn_input_validation>();reply->metadata.version=1;reply->metadata.token=token;
  if(!Call(TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE,*reply))return Stop();
  result=reply->metadata;
  if(result.version!=1 || result.token!=token || result.position || result.reserved
   || result.state!=TT_RUNTIME_PASSED || result.closed!=1 || result.input_ready!=1 || result.recovery_required
   || !result.session || result.driver_generation!=generation || !result.elapsed_us || result.source_pages!=23237
   || result.workspace_pages!=512 || result.saved_words!=8281546 || result.restored_words!=8281546
   || reply->history_runs!=336 || reply->history_pages!=1680 || reply->projections!=4 || reply->peer_boots!=119
   || reply->peer_stopped!=119 || reply->peer_held!=119 || reply->root_starts!=5 || reply->root_holds!=5
   || reply->convolution_workers!=120 || reply->convolution_jobs!=640 || reply->convolution_channels!=10240 || reply->convolution_held!=120
   || reply->input_workers!=96 || reply->input_installed!=96 || reply->input_armed!=96 || reply->input_released!=96
   || reply->input_completed!=96 || reply->input_held!=96 || reply->readback_workers!=96 || reply->readback_words!=30912
   || reply->saved_words!=8134072 || reply->verified_words!=16268144 || reply->installed_words!=989304 || reply->restored_words!=8134072)return Stop();
  for(unsigned i=0;i<30912;i++){
   double actual=PioneerAttentionFloat(reply->values[i]),expected=PioneerAttentionFloat(reference[i]);
   if(!isfinite(actual) || fabs(actual-expected)>3e-6*(1+fabs(expected)))return Stop();
   if(i%322>=320){
    if(expected>=double(PioneerAttentionFloat(0x00800000))){if(actual<=0 || fabs(actual-expected)>3e-6*expected)return Stop();}
    else if(actual!=0)return Stop(); // actual-subnormal FTZ only,not an absolute-tolerance escape
   }
  }
  if(!Current())return Stop();
  return true; // ALL output checks BEFORE explicit END,NO sampling or numeric fallback
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
