/* SPDX-License-Identifier: MIT */
#include "PioneerQ38GdnOutputValidationReference.h"
#if !defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE) || PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE != 1
#error Separate DEBUG55 original GDN plus output-reference client required
#endif
#ifndef PIONEER_Q38_GDN_OUTPUT_VALIDATION_SESSION_H
#define PIONEER_Q38_GDN_OUTPUT_VALIDATION_SESSION_H
#include "PioneerQ38GdnRendezvousValidationReference.h"
#include "PioneerQ38PublicModelSource.h"
#include "PioneerQ38EmbeddingValidationReference.h"
#include "PioneerQ38LearnedRmsValidationReference.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputValidationProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
// Exact source-bound DEBUG55. Original GDN references BEFORE FD. AFTER
// returned GDN passes strict independent checks,derive output projection
// reference from validated ACTUAL gated values. Original weight bytes ONLY
// uploaded;no CPU numeric substitution,fallback,inference or Work mode.
template<class IO,class Source=PioneerQ38PublicModelSource>class PioneerQ38GdnOutputValidationSession {
public:
 enum class Failure {None,SessionContract,SourceChanged,Transport,ReplyMetadata,NonFinite,AbsoluteTolerance,RelativeTolerance,SubnormalFlush,EndMetadata,Close,Reference,OperandMismatch,ScaleMismatch,OutputMismatch};
 struct Diagnostics {
  Failure reason=Failure::None;
  unsigned op=0,word=UINT32_MAX;
  uint32_t expected=0,actual=0,reply_header[64]={};
  bool reply_captured=false;
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
   case Failure::Reference:return "cpu_reference";
   case Failure::OperandMismatch:return "native_operand_mismatch";
   case Failure::ScaleMismatch:return "scale_mismatch";
   case Failure::OutputMismatch:return "output_mismatch";
  }
  return "unknown";
 }
private:
 IO& io;Source& source;PioneerQwen38PackedPlan plan;
 int fd=-1;bool attempted=false,failed=false,active=false,finished=false,validated=false;
 uint64_t epoch=0,generation=0;
 bool Current()const{return source.Ready() && source.Model().Epoch()==epoch && plan.MatchesSource(source.Model());}
 bool Record(Failure reason,unsigned word=UINT32_MAX,uint32_t expected=0,uint32_t actual=0){
  if(diagnostics.reason==Failure::None){diagnostics.reason=reason;diagnostics.word=word;diagnostics.expected=expected;diagnostics.actual=actual;}
  return false;
 }
 bool Stop(Failure reason=Failure::SessionContract,unsigned word=UINT32_MAX,uint32_t expected=0,uint32_t actual=0){
  Record(reason,word,expected,actual);failed=true;active=validated=false;finished=true;if(fd>=0){io.Close(fd);fd=-1;}return false;
 }
 template<class T>bool Call(unsigned op,T& value){
  if(failed || finished || fd<0)return false;
  diagnostics.op=op;
  if(!Current())return Record(Failure::SourceChanged);
  if(!io.Call(fd,op,&value,sizeof(value)))return Record(Failure::Transport);
  if(failed || finished || fd<0)return false;
  return Current() || Record(Failure::SourceChanged);
 }
public:
 tt_q38_embedding_produce result={};
 PioneerQ38GdnOutputValidationSession(IO& transport,Source& model):io(transport),source(model){}
 ~PioneerQ38GdnOutputValidationSession(){if(fd>=0)io.Close(fd);} // CLOSE ONLY,no implicit END/reset/retry
 PioneerQ38GdnOutputValidationSession(const PioneerQ38GdnOutputValidationSession&)=delete;
 PioneerQ38GdnOutputValidationSession& operator=(const PioneerQ38GdnOutputValidationSession&)=delete;
 bool Validate(unsigned token,unsigned position=0){
  if(attempted || failed || finished || token>=248320 || position)return false;
  attempted=true;if(!source.Ready() || !plan.Build(source.Model(),8192,true,true))return Stop();
  epoch=source.Model().Epoch();const auto& parts=plan.Parts();const auto& tensors=source.Model().Tensors();
  unsigned embedding=UINT32_MAX,norm=UINT32_MAX,firstPage=0,stripes=0,convStripes=0,convRows=0,learned=0,outputStripes=0,outputRows=0,totalPages=12;
  std::vector<unsigned> selected;
  for(unsigned i=0;i<parts.size();i++){
   const auto& p=parts[i];if(p.kind!=PIONEER_Q38_WEIGHT || p.tensor>=tensors.size())continue;
   const auto& t=tensors[p.tensor];
   if(t.name=="token_embd.weight" && token>=p.firstRow && token-p.firstRow<p.rows){
    if(embedding!=UINT32_MAX || t.type!=12 || t.inputs!=5120 || t.outputs!=248320 || p.rowBytes!=2880)return Stop();
    embedding=i;firstPage=((token-p.firstRow)/16*46080)/4096;
   }
   if(p.layer || (p.role!=0 && p.role!=5 && p.role!=6 && p.role!=7 && p.role!=8 && p.role!=9 && p.role!=10 && p.role!=11 && p.role!=12 && p.role!=13))continue;
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
   }else if(p.role==13){
    if(t.name!="blk.0.ssm_out.weight" || t.type!=8 || t.rank!=2 || t.inputs!=6144 || t.outputs!=5120 || t.bytes!=UINT64_C(5120)*6528
     || p.rowBytes!=6912 || !p.rows || (p.rows&15) || (p.firstRow&15) || p.firstRow>=5120 || p.rows>5120-p.firstRow
     || p.bytes!=p.rows*6912)return Stop();
    outputStripes++;outputRows+=p.rows;
   }else{
    PioneerQ38Tensor expected;
    if(!PioneerQ38Expected(0,p.role,expected) || t.name!=expected.name || t.type!=8 || t.rank!=2 || t.inputs!=5120
     || t.outputs!=PioneerQ38ProjectionResultRows(p.role) || p.rowBytes!=5760)return Stop();
    stripes++;
   }
   selected.push_back(i);totalPages+=p.allocated/4096;
  }
  if(embedding==UINT32_MAX || norm==UINT32_MAX || selected.size()!=34 || stripes!=16 || convStripes!=7
   || convRows!=10240 || learned!=3 || outputStripes!=7 || outputRows!=5120 || totalPages!=31878 || !Current())return Stop();
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
   || reload.build_tag!=UINT64_C(0x2026102400010055) || !reload.generation || reload.quiesced || reload.cold
   || reload.active_owner || reload.open_count!=1)return Stop();
  generation=reload.generation;tt_q38_embedding_support support={};
  if(!Call(TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATION_SUPPORT,support) || support.version!=1 || support.driver!=55
   || support.model_namespace!=18 || support.inputs!=838016 || support.vocabulary!=248320 || support.runtime_pages!=512
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
  if(nonce!=149)return Stop(); //111 earlier spans+38 original ssm_out spans (64-row striped layout)
  tt_q38_span_end end={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,end) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required)return Stop();
  auto reply=std::make_unique<tt_q38_gdn_output_validation>();reply->metadata.version=1;reply->metadata.token=token;
  bool called=Call(TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE,*reply);
  // Snapshot only the already-returned CPU buffer,even if the call/source
  // check failed. Never read the card or send END to diagnose a failed run.
  static_assert(offsetof(tt_q38_gdn_output_validation,values)==sizeof(diagnostics.reply_header),"complete fixed header snapshot");
  memcpy(diagnostics.reply_header,reply.get(),sizeof(diagnostics.reply_header));diagnostics.reply_captured=true;
  if(!called)return Stop();
  result=reply->metadata;
  const uint32_t expectedHeader[64]={
   1,token,0,0,TT_RUNTIME_PASSED,1,1,0,0,0,uint32_t(generation),uint32_t(generation>>32),0,0,31878,512,8478154,8478154,
   672,73728,336,1680,4,119,119,119,5,5,120,640,10240,120,
   96,96,96,96,96,96,1,96,829632,8330680,16661360,1185912,8330680,1,8642,0,
   120,119,2,119,1,5,5120,3072,192
  };
  for(unsigned i=0;i<64;i++){
   if(i==8 || i==12){
    if(!(diagnostics.reply_header[i]|diagnostics.reply_header[i+1]))return Stop(Failure::ReplyMetadata,i,1,0);
    i++;continue; // actual session and aggregate elapsed are nonzero,not caller-predicted values
   }
   if(diagnostics.reply_header[i]!=expectedHeader[i])return Stop(Failure::ReplyMetadata,i,expectedHeader[i],diagnostics.reply_header[i]);
  }
  for(unsigned i=0;i<829632;i++){
   double actual=PioneerAttentionFloat(reply->values[i]),expected=PioneerAttentionFloat(reference[i]);
   if(!isfinite(actual))return Stop(Failure::NonFinite,i,reference[i],reply->values[i]);
   if(fabs(actual-expected)>3e-6*(1+fabs(expected)))return Stop(Failure::AbsoluteTolerance,i,reference[i],reply->values[i]);
   if((i%8642>=320 && i%8642<322) || i%8642>=8578){
    if(fabs(expected)>=double(PioneerAttentionFloat(0x00800000))){if((i%8642<322 && actual<=0) || fabs(actual-expected)>3e-6*fabs(expected))return Stop(Failure::RelativeTolerance,i,reference[i],reply->values[i]);}
    else if(actual!=0)return Stop(Failure::SubnormalFlush,i,reference[i],reply->values[i]); // actual-subnormal FTZ only;NORMAL gated values require relative accuracy too
   }
  }
  if(!Current())return Stop(Failure::SourceChanged);
  // ONLY after ALL original GDN fields pass. Tiny legitimate GDN errors
  // may cross Q8 rounding boundaries;never quantize the predicted GDN in
  // place of these accepted actual values,and NEVER upload CPU products.
  std::vector<uint32_t> gated(6144);
  for(unsigned half=0;half<96;half++)memcpy(gated.data()+half*64,reply->values+half*8642+8578,256);
  auto projected=std::make_unique<PioneerQ38GdnOutputReferenceResult>();
  if(!PioneerQ38GdnOutputReference(source,gated.data(),6144,projected.get()) || !Current())
   return Stop(Current()?Failure::Reference:Failure::SourceChanged);
  for(unsigned i=0;i<3072;i++)if(reply->values[829632+i]!=projected->operand[i])
   return Stop(Failure::OperandMismatch,829632+i,projected->operand[i],reply->values[829632+i]);
  for(unsigned i=0;i<192;i++){
   uint32_t actual=reply->values[832704+i],expected=projected->scale[i];
   if((actual&0x7fffffff)>=0x7f800000)return Stop(Failure::NonFinite,832704+i,expected,actual);
   if(actual!=expected)return Stop(Failure::ScaleMismatch,832704+i,expected,actual);
  }
  for(unsigned i=0;i<5120;i++){
   uint32_t actual=reply->values[832896+i],expected=projected->values[i];
   if((actual&0x7fffffff)>=0x7f800000)return Stop(Failure::NonFinite,832896+i,expected,actual);
   if(actual!=expected)return Stop(Failure::OutputMismatch,832896+i,expected,actual);
  }
  if(!Current() || failed || finished)return Stop(Failure::SourceChanged);
  validated=true;return true; // exact output/strict GDN checks BEFORE explicit END,no fallback
 }
 bool Finish(){
  if(!active || !validated || failed || finished || fd<0)return false;
  tt_q38_bank_session end={};end.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,end) || end.result.version!=1 || end.result.state!=TT_RESIDENT_SESSION_CLOSED
   || end.result.active || end.result.cold || end.result.pci_restored!=1 || end.result.host_released!=1
   || end.result.capabilities || end.result.bytes!=PioneerResidentBanks::Total)return Stop(Failure::EndMetadata);
  active=validated=false;finished=true;int owned=fd;fd=-1;return io.Close(owned)==0 || Record(Failure::Close);
 }
 void Abort(){Stop();}
};
#endif
