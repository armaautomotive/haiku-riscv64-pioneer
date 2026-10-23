/* SPDX-License-Identifier: MIT */
// Production source-bound DEBUG session;synthetic ORIGINAL zero Q8/F32
// weights and OS endpoints ONLY. Mock never evaluates a reference helper.
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_q38_projection_validation_fixture.h"
#include "../PioneerQ38ResidentDecoderValidationSession.h"
struct DecoderValidationIO {
 ProjectionValidationSource& source;PioneerQwen38PackedPlan plan;unsigned token,partAt=0,pageAt=0;
 struct Part{unsigned index,first,pages;};std::vector<Part> selected;
 unsigned opens=0,closes=0,calls=0,begins=0,spans=0,spanEnds=0,runs=0,ends=0,uploaded=0;
 unsigned failCall=0,sourceDrop=0,epochDrop=0,corruptWord=UINT32_MAX,badReply=UINT32_MAX;
 bool stopped=false,nonfinite=false;
 DecoderValidationIO(ProjectionValidationSource& s,unsigned id):source(s),token(id){
  assert(plan.Build(source.Model(),8192,true,true));
  for(unsigned i=0;i<plan.Parts().size();i++){
   const auto& p=plan.Parts()[i];if(p.kind!=PIONEER_Q38_WEIGHT)continue;
   const auto& t=source.Model().Tensors()[p.tensor];
   if(t.name=="token_embd.weight" && token>=p.firstRow && token-p.firstRow<p.rows)
    selected.insert(selected.begin(),{i,((token-p.firstRow)/16*46080)/4096,12});
   if(!p.layer && (p.role==0 || p.role==5 || p.role==6 || p.role==10 || p.role==11 || p.role==12))selected.push_back({i,0,p.allocated/4096});
  }assert(selected.size()==25);
 }
 int Open(){assert(source.Ready() && source.referenceReads==1282 && !source.gpuOpen && !stopped);opens++;source.gpuOpen=true;return 73;}
 int Close(int fd){assert(fd==73 && !closes);closes++;return 0;}
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==73 && !closes && !stopped && source.referenceReads==1282);calls++;
  switch(op){
   case TT_PCI_PROBE_RELOAD_STATUS:{auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
    assert(bytes==sizeof(r) && r.version==1);r.driver=55;r.guard_version=1;r.open_count=1;r.generation=4;r.build_tag=UINT64_C(0x2026102300010044);break;}
   case TT_PCI_PROBE_Q38_DECODER_VALIDATION_SUPPORT:
    assert(bytes==sizeof(tt_q38_embedding_support));*static_cast<tt_q38_embedding_support*>(data)={1,55,14,10240,248320,512,0,0};break;
   case TT_PCI_PROBE_Q38_BANK_SUPPORT:
    *static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,PioneerResidentBanks::Capacity};break;
   case TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT:
    *static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};break;
   case TT_PCI_PROBE_Q38_BANK_BEGIN:{begins++;auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    r.version=1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=PioneerResidentBanks::Total;break;}
   case TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD:{auto& r=*static_cast<tt_q38_span_upload*>(data);
    assert(partAt<selected.size());const auto& part=selected[partAt];const auto& p=plan.Parts()[part.index];unsigned pages=std::min(256U,part.pages-pageAt);
    assert(begins==1 && !runs && !spanEnds && bytes==sizeof(r) && r.version==1 && !r.reserved && r.bank==p.bank
     && r.offset==p.proposedAddress-plan.Base+uint64_t(part.first+pageAt)*4096 && r.bytes==pages*4096 && !r.result.state);
    for(unsigned page=0;page<pages;page++){
     uint8_t original[4096];assert(plan.ReadWeightPage(source.model,part.index,part.first+pageAt+page,original,4096));
     assert(!memcmp(original,r.data+page*1024,4096));
    }
    for(unsigned word=pages*1024;word<262144;word++)assert(!r.data[word]);
    spans++;uploaded+=pages;pageAt+=pages;if(pageAt==part.pages){partAt++;pageAt=0;}
    r.result={1,TT_RUNTIME_PASSED,pages*4096,pages,spans,1,1,1,0,0,0,0};break;}
   case TT_PCI_PROBE_Q38_SPAN_END:
    assert(partAt==25 && !pageAt && uploaded==23235 && spans==108 && !runs);spanEnds++;*static_cast<tt_q38_span_end*>(data)={1,0,1,0};break;
   case TT_PCI_PROBE_Q38_DECODER_VALIDATE:{runs++;auto& reply=*static_cast<tt_q38_resident_decoder_validation*>(data);auto& r=reply.metadata;
    assert(bytes==sizeof(reply) && PioneerQ38EmbeddingRequest(r) && r.token==token && !r.position && spanEnds==1);
    const auto* raw=reinterpret_cast<const uint8_t*>(&reply);for(unsigned i=sizeof(r);i<sizeof(reply);i++)assert(!raw[i]);
    r.state=TT_RUNTIME_PASSED;r.closed=r.input_ready=1;r.session=7;r.driver_generation=4;r.elapsed_us=123;
    r.source_pages=23235;r.workspace_pages=512;r.saved_words=r.restored_words=8281546;
    reply.history_runs=336;reply.history_pages=1680;reply.projections=4;reply.peer_boots=reply.peer_stopped=reply.peer_held=119;
    reply.root_starts=reply.root_holds=5;reply.convolution_workers=120;reply.convolution_jobs=640;reply.convolution_channels=10240;reply.convolution_held=120;
    reply.readback_pages=40;reply.readback_words=40960;reply.saved_words=reply.restored_words=8134072;reply.verified_words=16268144;reply.installed_words=989304;
    // Original zero Q8 matrices yield zero QKV,so all native-byte outputs
    // are zero. No invocation of CPU oracles or upload of CPU products.
    if(corruptWord<40960){reply.values[corruptWord]=nonfinite?0x7fc00000U:corruptWord<10240?0x3f800000U:1U;stopped=true;}
    if(badReply<36){uint32_t v;auto* field=reinterpret_cast<uint8_t*>(&reply)+badReply*4;memcpy(&v,field,4);
     if(badReply==8 || badReply==12)v=0;else v^=1;memcpy(field,&v,4);stopped=true;}
    break;}
   case TT_PCI_PROBE_Q38_BANK_END:{ends++;assert(runs==1);auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    r.version=1;r.state=TT_RESIDENT_SESSION_CLOSED;r.host_released=r.pci_restored=1;r.bytes=PioneerResidentBanks::Total;break;}
   default:assert(!"Old namespace/Work/numeric input/fallback endpoint MUST NOT be called");
  }
  if(calls==sourceDrop)source.valid=false;if(calls==epochDrop)assert(source.model.Open(source.file));
  if(calls==failCall || calls==sourceDrop || calls==epochDrop)stopped=true;
  return calls!=failCall;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;unsigned positives=0,faults=0;
 for(unsigned token:{0U,15U,248319U}){
  SeedProjectionValidationEmbedding(fixture,token);ProjectionValidationSource source(fixture.file);DecoderValidationIO io(source,token);
  PioneerQ38ResidentDecoderValidationSession<DecoderValidationIO,ProjectionValidationSource> session(io,source);
  assert(!session.Validate(token,1) && !io.opens && session.Validate(token) && source.referenceReads==1282 && source.reads==23235 && session.Finish()
   && io.ends==1 && io.closes==1 && io.calls==116 && io.spans==108);positives++;
  printf("PASS decoder DEBUG session token%u/source pages23235/spans108/calls116\n",token);
 }
 SeedProjectionValidationEmbedding(fixture,0);
 auto check=[&](unsigned kind,unsigned at){
  ProjectionValidationSource source(fixture.file);DecoderValidationIO io(source,0);
  if(kind==0)io.failCall=at;if(kind==1)io.sourceDrop=at;if(kind==2)io.epochDrop=at;if(kind==3)source.failRead=at;
  if(kind==4)source.failReference=at;if(kind==5)io.corruptWord=at;if(kind==6){io.corruptWord=at;io.nonfinite=true;}if(kind==7)io.badReply=at;
  PioneerQ38ResidentDecoderValidationSession<DecoderValidationIO,ProjectionValidationSource> session(io,source);
  bool valid=session.Validate(0);assert(valid==(kind<3 && at==116));assert(!session.Finish() && !session.Validate(0));
  if(kind<3)assert(io.calls==at && io.ends==unsigned(at==116));
  else if(kind==3)assert(source.reads==at && !io.runs && !io.ends);
  else if(kind==4)assert(source.referenceReads==at && !io.opens && !io.calls);
  else assert(io.runs==1 && io.calls==115 && !io.ends);
  assert(io.closes==unsigned(source.gpuOpen));faults++;
 };
 for(unsigned kind=0;kind<3;kind++)for(unsigned at:{1U,5U,6U,113U,114U,115U,116U})check(kind,at);
 for(unsigned at:{1U,12U,23235U})check(3,at);for(unsigned at:{1U,2U,642U,643U,1282U})check(4,at);
 for(unsigned page=0;page<40;page++)check(5,page*1024+(page&1?1023:0));
 for(unsigned at:{0U,10239U,10240U,40959U})check(6,at);
 for(unsigned at:{0U,1U,2U,3U,4U,5U,6U,7U,8U,10U,11U,12U,14U,15U,16U,17U,
  18U,19U,20U,21U,22U,23U,24U,25U,26U,27U,28U,29U,30U,31U,32U,33U,34U,35U})check(7,at);
 printf("PASS source-bound decoder DEBUG session positives%u/faults%u/1282 original-reference reads BEFORE FD/23235 ORIGINAL pages/tag44/namespace14/ALL40pages checked/CLOSE ONLY after first fault;SYNTHETIC_TRANSPORT_NOT_CARD_ARITHMETIC_OR_SPEED\n",positives,faults);
}
