/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE) || defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE) || defined(PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE)
#error Resident operator IMAGE never contains a CPU validation session
#endif
#ifdef PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE
#error Native66 never contains a CPU validation session
#endif
#ifdef PIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT
#if PIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT != 1 || !defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT) || PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT != 1 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1 || !defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE) || PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE != 1
#error Exact separate original-source CPU DEBUG64 observer required
#endif
#endif
#if defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_SESSION_H) && (defined(PIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT) != defined(PIONEER_Q38_CONV_PRECISION_SESSION_BOUND))
#error DEBUG52 and DEBUG64 session roles cannot change after body binding
#endif
#ifndef PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_SESSION_H
#define PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_SESSION_H
#include "PioneerQ38GdnRendezvousValidationReference.h"
#include "PioneerQ38PublicModelSource.h"
#include "PioneerQ38EmbeddingValidationReference.h"
#include "PioneerQ38LearnedRmsValidationReference.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnRendezvousValidationProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
// Exact source-bound DEBUG52 session. ALL references BEFORE FD,original
// tiled weight bytes ONLY on upload. Not inference or a Work fallback.
template<class IO,class Source=PioneerQ38PublicModelSource>class PioneerQ38GdnRendezvousValidationSession {
public:
#ifdef PIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT
#define PIONEER_Q38_CONV_PRECISION_SESSION_BOUND 1
 static constexpr uint64_t ExpectedBuildTag=UINT64_C(0x2026102400010064);
#else
 static constexpr uint64_t ExpectedBuildTag=UINT64_C(0x2026102400010052);
#endif
 // Initial projection-image ledger only; decoder overlays have their own
 // exact-image checks and must not be added to this reply field.
 static constexpr unsigned ExpectedInstalledWords=1185912;
 enum class Failure {None,SessionContract,SourceChanged,Transport,ReplyMetadata,NonFinite,AbsoluteTolerance,RelativeTolerance,SubnormalFlush,EndMetadata,Close};
 struct Diagnostics {
  Failure reason=Failure::None;
  unsigned op=0,word=UINT32_MAX;
  uint32_t expected=0,actual=0,reply_header[48]={};
  bool reply_captured=false;
  unsigned gated_head=UINT32_MAX,gated_lane=UINT32_MAX;
  uint32_t paired_raw_expected[128]={},paired_raw_actual[128]={},original_z[128]={};
  uint32_t paired_prepared_expected[2][322]={},paired_prepared_actual[2][322]={};
  uint32_t failing_next_row_expected[128]={},failing_next_row_actual[128]={};
  bool gated_context_captured=false;
 } diagnostics;
 static const char* FailureName(Failure reason){
  switch(reason){
   case Failure::None:return "none";
   case Failure::SessionContract:return "session_contract";
   case Failure::SourceChanged:return "source_changed";
   case Failure::Transport:return "transport";
   case Failure::ReplyMetadata:return "reply_metadata";
   case Failure::NonFinite:return "nonfinite_value";
   case Failure::AbsoluteTolerance:return "absolute_tolerance";
   case Failure::RelativeTolerance:return "relative_tolerance";
   case Failure::SubnormalFlush:return "subnormal_flush";
   case Failure::EndMetadata:return "end_metadata";
   case Failure::Close:return "close";
  }
  return "unknown";
 }
private:
 IO& io;Source& source;PioneerQwen38PackedPlan plan;
 int fd=-1;bool attempted=false,failed=false,active=false,finished=false;
 uint64_t epoch=0,generation=0;
 bool Current()const{return source.Ready() && source.Model().Epoch()==epoch && plan.MatchesSource(source.Model());}
 bool Record(Failure reason,unsigned word=UINT32_MAX,uint32_t expected=0,uint32_t actual=0){
  if(diagnostics.reason==Failure::None){diagnostics.reason=reason;diagnostics.word=word;diagnostics.expected=expected;diagnostics.actual=actual;}
  return false;
 }
 bool Stop(Failure reason=Failure::SessionContract,unsigned word=UINT32_MAX,uint32_t expected=0,uint32_t actual=0){
  Record(reason,word,expected,actual);failed=true;active=false;finished=true;if(fd>=0){io.Close(fd);fd=-1;}return false;
 }
 template<class T>bool Call(unsigned op,T& value){
  diagnostics.op=op;
  if(!Current())return Record(Failure::SourceChanged);
  if(!io.Call(fd,op,&value,sizeof(value)))return Record(Failure::Transport);
  return Current() || Record(Failure::SourceChanged);
 }
public:
 tt_q38_embedding_produce result={};
 PioneerQ38GdnRendezvousValidationSession(IO& transport,Source& model):io(transport),source(model){}
 ~PioneerQ38GdnRendezvousValidationSession(){if(fd>=0)io.Close(fd);} // CLOSE ONLY,no implicit END/reset/retry
 PioneerQ38GdnRendezvousValidationSession(const PioneerQ38GdnRendezvousValidationSession&)=delete;
 PioneerQ38GdnRendezvousValidationSession& operator=(const PioneerQ38GdnRendezvousValidationSession&)=delete;
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
   if(p.layer || (p.role!=0 && p.role!=5 && p.role!=6 && p.role!=7 && p.role!=8 && p.role!=9 && p.role!=10 && p.role!=11 && p.role!=12))continue;
   if(!p.allocated || (p.allocated&4095) || p.proposedAddress<plan.Base)return Stop();
   if(!p.role){
    if(norm!=UINT32_MAX || t.name!="blk.0.attn_norm.weight" || t.type || t.rank!=1 || t.inputs!=5120 || t.outputs!=1
     || p.firstRow || p.rows!=1 || p.rowBytes!=20480 || p.bytes!=20480 || p.allocated!=20480)return Stop();
    norm=i;
   }else if(p.role==7 || p.role==8){
    if(t.name!=(p.role==7?"blk.0.ssm_a":"blk.0.ssm_dt.bias") || t.type || t.rank!=1 || t.inputs!=48 || t.outputs!=1
     || t.bytes!=192 || p.firstRow || p.rows!=1 || p.rowBytes!=192 || p.bytes!=192 || p.allocated!=4096)return Stop();
    learned++;
   }else if(p.role==9){
    if(t.name!="blk.0.ssm_norm.weight" || t.type || t.rank!=1 || t.inputs!=128 || t.outputs!=1 || t.bytes!=512
     || p.firstRow || p.rows!=1 || p.rowBytes!=512 || p.bytes!=512 || p.allocated!=4096)return Stop();
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
  if(embedding==UINT32_MAX || norm==UINT32_MAX || selected.size()!=27 || stripes!=16 || convStripes!=7
   || convRows!=10240 || learned!=3 || totalPages!=23238 || !Current())return Stop();
  std::vector<uint32_t> raw(5120),normalized(5120),z(6144),qkv(10240),conv(40960),alpha(48),beta(48),reference(829632);
  if(!PioneerQ38EmbeddingReference(source,parts[embedding].tensor,token,raw.data()) || !Current()
   || !PioneerQ38LearnedRmsReference(source,parts[norm].tensor,raw.data(),normalized.data()) || !Current()
   || !PioneerQ38ProjectionReference(source,6,normalized.data(),5120,qkv.data(),10240) || !Current()
   || !PioneerQ38ResidentDecoderReference(source,qkv.data(),10240,conv.data(),40960) || !Current()
   || !PioneerQ38ProjectionReference(source,10,normalized.data(),5120,alpha.data(),48) || !Current()
   || !PioneerQ38ProjectionReference(source,11,normalized.data(),5120,beta.data(),48) || !Current()
   || !PioneerQ38ProjectionReference(source,5,normalized.data(),5120,z.data(),6144) || !Current()
   || !PioneerQ38GdnRendezvousReference(source,conv.data(),40960,alpha.data(),beta.data(),48,z.data(),6144,reference.data(),829632) || !Current())return Stop();
  fd=io.Open();if(fd<0)return Stop();
  tt_pioneer_driver_reload reload={};reload.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,reload) || reload.version!=1 || reload.driver!=55 || reload.guard_version!=1
   || reload.build_tag!=ExpectedBuildTag || !reload.generation || reload.quiesced || reload.cold
   || reload.active_owner || reload.open_count!=1)return Stop();
  generation=reload.generation;tt_q38_embedding_support support={};
  if(!Call(TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATION_SUPPORT,support) || support.version!=1 || support.driver!=55
   || support.model_namespace!=17 || support.inputs!=829632 || support.vocabulary!=248320 || support.runtime_pages!=512
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
  if(nonce!=111)return Stop();
  tt_q38_span_end end={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,end) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required)return Stop();
  auto reply=std::make_unique<tt_q38_gdn_rendezvous_validation>();reply->metadata.version=1;reply->metadata.token=token;
  bool called=Call(TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE,*reply);
  // Snapshot only the already-returned CPU buffer,even if the call/source
  // check failed. Never read the card or send END to diagnose a failed run.
  static_assert(offsetof(tt_q38_gdn_rendezvous_validation,values)==sizeof(diagnostics.reply_header),"complete fixed header snapshot");
  memcpy(diagnostics.reply_header,reply.get(),sizeof(diagnostics.reply_header));diagnostics.reply_captured=true;
  if(!called)return Stop();
  result=reply->metadata;
  if(result.version!=1 || result.token!=token || result.position || result.reserved
   || result.state!=TT_RUNTIME_PASSED || result.closed!=1 || result.input_ready!=1 || result.recovery_required
   || !result.session || result.driver_generation!=generation || !result.elapsed_us || result.source_pages!=23238
   || result.workspace_pages!=512 || result.saved_words!=8478154 || result.restored_words!=8478154
   || reply->gated_published!=1 || reply->readback_half_words!=8642 || reply->state_runs!=672 || reply->state_pages!=73728 || reply->layer_published!=1 || reply->reserved
   || reply->history_runs!=336 || reply->history_pages!=1680 || reply->projections!=4 || reply->peer_boots!=119
   || reply->peer_stopped!=119 || reply->peer_held!=119 || reply->root_starts!=5 || reply->root_holds!=5
   || reply->convolution_workers!=120 || reply->convolution_jobs!=640 || reply->convolution_channels!=10240 || reply->convolution_held!=120
   || reply->input_workers!=96 || reply->input_installed!=96 || reply->input_armed!=96 || reply->input_released!=96
   || reply->input_completed!=96 || reply->input_held!=96 || reply->readback_workers!=96 || reply->readback_words!=829632
   || reply->saved_words!=8330680 || reply->verified_words!=16661360 || reply->installed_words!=ExpectedInstalledWords || reply->restored_words!=8330680)return Stop(Failure::ReplyMetadata);
  auto numericStop=[&](Failure reason,unsigned i){
   // CPU memory ONLY,after the single existing reply. Capture the paired
   // raw input and original Z for arithmetic diagnosis,never query the card
   // or replace any result/reference. Source reads/calls/bounds unchanged.
   if(i%8642>=8578 && !diagnostics.gated_context_captured){
    unsigned head=i/8642/2;diagnostics.gated_head=head;diagnostics.gated_lane=(i/8642%2)*64+i%8642-8578;
    for(unsigned j=0;j<128;j++){
     unsigned at=(head*2+j/64)*8642+8514+j%64;
     diagnostics.paired_raw_expected[j]=reference[at];diagnostics.paired_raw_actual[j]=reply->values[at];diagnostics.original_z[j]=z[head*128+j];
    }
    for(unsigned half=0;half<2;half++)for(unsigned j=0;j<322;j++){
     unsigned at=(head*2+half)*8642+j;
     diagnostics.paired_prepared_expected[half][j]=reference[at];diagnostics.paired_prepared_actual[half][j]=reply->values[at];
    }
    for(unsigned key=0;key<128;key++){
     unsigned at=(head*2+diagnostics.gated_lane/64)*8642+322+key*64+diagnostics.gated_lane%64;
     diagnostics.failing_next_row_expected[key]=reference[at];diagnostics.failing_next_row_actual[key]=reply->values[at];
    } // diagnostic partner half may be unchecked;NONE of these bytes is a native producer grant
    diagnostics.gated_context_captured=true;
   }
   return Stop(reason,i,reference[i],reply->values[i]);
  };
  for(unsigned i=0;i<829632;i++){
   double actual=PioneerAttentionFloat(reply->values[i]),expected=PioneerAttentionFloat(reference[i]);
   if(!isfinite(actual))return numericStop(Failure::NonFinite,i);
   if(fabs(actual-expected)>3e-6*(1+fabs(expected)))return numericStop(Failure::AbsoluteTolerance,i);
   if((i%8642>=320 && i%8642<322) || i%8642>=8578){
    if(fabs(expected)>=double(PioneerAttentionFloat(0x00800000))){if((i%8642<322 && actual<=0) || fabs(actual-expected)>3e-6*fabs(expected))return numericStop(Failure::RelativeTolerance,i);}
    else if(actual!=0)return numericStop(Failure::SubnormalFlush,i); // actual-subnormal FTZ only;NORMAL gated values require relative accuracy too
   }
  }
  if(!Current())return Stop(Failure::SourceChanged);
  return true; // ALL output checks BEFORE explicit END,NO sampling or numeric fallback
 }
 bool Finish(){
  if(!active || failed || finished || fd<0)return false;
  tt_q38_bank_session end={};end.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,end) || end.result.version!=1 || end.result.state!=TT_RESIDENT_SESSION_CLOSED
   || end.result.active || end.result.cold || end.result.pci_restored!=1 || end.result.host_released!=1
   || end.result.capabilities || end.result.bytes!=PioneerResidentBanks::Total)return Stop(Failure::EndMetadata);
  active=false;finished=true;int owned=fd;fd=-1;return io.Close(owned)==0 || Record(Failure::Close);
 }
 void Abort(){Stop();}
};
#endif
