/* SPDX-License-Identifier: MIT */
// Production DEBUG46 session,synthetic original zero matrices/OS endpoints.
// No accelerator or reference helper is invoked by the transport mock.
#define PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_q38_projection_validation_fixture.h"
#include "../PioneerQ38GdnInputValidationSession.h"
static void SeedGdnLearned(Fixture& f,float a=-1){
 for(const auto& t:f.expected)if(t.name=="blk.0.ssm_a"){
  std::vector<uint8_t> bytes(192);uint32_t bits=PioneerAttentionBits(a);
  for(unsigned i=0;i<48;i++)for(unsigned b=0;b<4;b++)bytes[i*4+b]=uint8_t(bits>>(8*b));f.Write(t.offset,bytes);
 }
}
struct GdnValidationIO {
 ProjectionValidationSource& source;PioneerQwen38PackedPlan plan;unsigned token,partAt=0,pageAt=0;
 struct Part{unsigned index,first,pages;};std::vector<Part> selected;
 unsigned opens=0,closes=0,calls=0,begins=0,spans=0,spanEnds=0,runs=0,ends=0,uploaded=0;
 unsigned failCall=0,sourceDrop=0,epochDrop=0,corruptWord=UINT32_MAX,badReply=UINT32_MAX;
 uint32_t decay=0x3f000000,corruptBits=0x3f800000;bool stopped=false;
 GdnValidationIO(ProjectionValidationSource& s,unsigned id):source(s),token(id){
  assert(plan.Build(source.Model(),8192,true,true));
  for(unsigned i=0;i<plan.Parts().size();i++){
   const auto& p=plan.Parts()[i];if(p.kind!=PIONEER_Q38_WEIGHT)continue;
   const auto& t=source.Model().Tensors()[p.tensor];
   if(t.name=="token_embd.weight" && token>=p.firstRow && token-p.firstRow<p.rows)
    selected.insert(selected.begin(),{i,((token-p.firstRow)/16*46080)/4096,12});
   if(!p.layer && (p.role==0 || p.role==5 || p.role==6 || p.role==7 || p.role==8 || p.role==10 || p.role==11 || p.role==12))
    selected.push_back({i,0,p.allocated/4096});
  }assert(selected.size()==27);
 }
 int Open(){assert(source.Ready() && source.referenceReads==1290 && !source.gpuOpen && !stopped);opens++;source.gpuOpen=true;return 73;}
 int Close(int fd){assert(fd==73 && !closes);closes++;return 0;}
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==73 && !closes && !stopped && source.referenceReads==1290);calls++;
  switch(op){
   case TT_PCI_PROBE_RELOAD_STATUS:{auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
    assert(bytes==sizeof(r) && r.version==1);r.driver=55;r.guard_version=1;r.open_count=1;r.generation=4;r.build_tag=UINT64_C(0x2026102300010046);break;}
   case TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATION_SUPPORT:
    assert(bytes==sizeof(tt_q38_embedding_support));*static_cast<tt_q38_embedding_support*>(data)={1,55,15,30912,248320,512,0,0};break;
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
    assert(partAt==27 && !pageAt && uploaded==23237 && spans==110 && !runs);spanEnds++;*static_cast<tt_q38_span_end*>(data)={1,0,1,0};break;
   case TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE:{runs++;auto& reply=*static_cast<tt_q38_gdn_input_validation*>(data);auto& r=reply.metadata;
    assert(bytes==sizeof(reply) && PioneerQ38EmbeddingRequest(r) && r.token==token && !r.position && spanEnds==1);
    const auto* raw=reinterpret_cast<const uint8_t*>(&reply);for(unsigned i=sizeof(r);i<sizeof(reply);i++)assert(!raw[i]);
    r.state=TT_RUNTIME_PASSED;r.closed=r.input_ready=1;r.session=7;r.driver_generation=4;r.elapsed_us=123;
    r.source_pages=23237;r.workspace_pages=512;r.saved_words=r.restored_words=8281546;
    reply.history_runs=336;reply.history_pages=1680;reply.projections=4;reply.peer_boots=reply.peer_stopped=reply.peer_held=119;
    reply.root_starts=reply.root_holds=5;reply.convolution_workers=120;reply.convolution_jobs=640;reply.convolution_channels=10240;reply.convolution_held=120;
    reply.input_workers=reply.input_installed=reply.input_armed=reply.input_released=reply.input_completed=reply.input_held=96;
    reply.readback_workers=96;reply.readback_words=30912;reply.saved_words=reply.restored_words=8134072;reply.verified_words=16268144;reply.installed_words=989304;
    // Zero original Q8/F32 matrices produce zero q/k/v/alpha/beta logits.
    // A=-1,dt=0 gives decay=beta=.5. A=-120 gives NORMAL decay near2^-120;
    // FP32 Softplus/product rounding gives bits0380001e,NOT exact2^-120.
    // Never zero despite tiny absolute magnitude. Literal expected bytes,
    // NOT a call to any oracle or CPU products uploaded to a fake device.
    for(unsigned half=0;half<96;half++){reply.values[half*322+320]=decay;reply.values[half*322+321]=0x3f000000;}
    if(corruptWord<30912){reply.values[corruptWord]=corruptBits;stopped=true;}
    if(badReply<42){uint32_t value;auto* field=reinterpret_cast<uint8_t*>(&reply)+badReply*4;memcpy(&value,field,4);
     if(badReply==8 || badReply==12)value=0;else value^=1;memcpy(field,&value,4);stopped=true;}
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
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;SeedGdnLearned(fixture);unsigned positives=0,faults=0;
 for(unsigned token:{0U,15U,248319U}){
  SeedProjectionValidationEmbedding(fixture,token);ProjectionValidationSource source(fixture.file);GdnValidationIO io(source,token);
  PioneerQ38GdnInputValidationSession<GdnValidationIO,ProjectionValidationSource> session(io,source);
  assert(!session.Validate(token,1) && !io.opens && session.Validate(token) && source.referenceReads==1290 && source.reads==23237
   && session.Finish() && io.ends==1 && io.closes==1 && io.calls==118 && io.spans==110);positives++;
  printf("PASS GDN DEBUG session token%u/reference1290/source23237/spans110/calls118\n",token);
 }
 SeedProjectionValidationEmbedding(fixture,0);
 auto check=[&](unsigned kind,unsigned at){
  ProjectionValidationSource source(fixture.file);GdnValidationIO io(source,0);
  if(kind==0)io.failCall=at;if(kind==1)io.sourceDrop=at;if(kind==2)io.epochDrop=at;if(kind==3)source.failRead=at;
  if(kind==4)source.failReference=at;if(kind==5)io.corruptWord=at;
  if(kind==6){io.corruptWord=at;io.corruptBits=0x7fc00000;}if(kind==7)io.badReply=at;
  if(kind==8){io.corruptWord=at;io.corruptBits=0;io.decay=0x0380001e;}
  PioneerQ38GdnInputValidationSession<GdnValidationIO,ProjectionValidationSource> session(io,source);
  bool valid=session.Validate(0);assert(valid==(kind<3 && at==118));assert(!session.Finish() && !session.Validate(0));
  if(kind<3)assert(io.calls==at && io.ends==unsigned(at==118));
  else if(kind==3)assert(source.reads==at && !io.runs && !io.ends);
  else if(kind==4)assert(source.referenceReads==at && !io.opens && !io.calls);
  else assert(io.runs==1 && io.calls==117 && !io.ends);
  assert(io.closes==unsigned(source.gpuOpen));faults++;
 };
 for(unsigned kind=0;kind<3;kind++)for(unsigned at:{1U,5U,6U,115U,116U,117U,118U})check(kind,at);
 for(unsigned at:{1U,12U,23236U,23237U})check(3,at);
 for(unsigned at:{1U,2U,642U,1282U,1283U,1288U,1289U,1290U})check(4,at);
 for(unsigned half=0;half<96;half++)check(5,half*322+(half%3==0?127:half%3==1?255:319));
 for(unsigned at:{0U,127U,128U,255U,256U,319U,320U,321U,30911U})check(6,at);
 for(unsigned at=0;at<42;at++)if(at!=9 && at!=13)check(7,at);
 SeedGdnLearned(fixture,-120);
 {ProjectionValidationSource source(fixture.file);GdnValidationIO io(source,0);io.decay=0x0380001e;
  PioneerQ38GdnInputValidationSession<GdnValidationIO,ProjectionValidationSource> session(io,source);
  assert(session.Validate(0) && session.Finish());positives++;}
 for(unsigned at:{320U,95U*322+320})check(8,at); // a zero NORMAL decay must fail,not pass absolute3e-6
 printf("PASS source-bound GDN DEBUG session positives%u/faults%u/tag46/namespace15/ALL96halves/strict-relative-gates/1290references BEFORE FD/23237ORIGINALpages/CLOSE ONLY after first fault;SYNTHETIC_NOT_CARD_OR_SPEED\n",positives,faults);
}
