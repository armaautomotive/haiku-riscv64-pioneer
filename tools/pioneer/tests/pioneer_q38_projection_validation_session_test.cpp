/* SPDX-License-Identifier: MIT */
// Production DEBUG session with sparse ORIGINAL GGUF/OS endpoint only.
// Zero Q8 matrices make genuine independent references zero;the mock NEVER
// evaluates the reference helpers or supplies a CPU result to the device.
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_q38_projection_validation_fixture.h"
#include "../PioneerQ38ProjectionValidationSession.h"
struct ProjectionValidationIO {
 ProjectionValidationSource& source;PioneerQwen38PackedPlan plan;unsigned token,partAt=0,pageAt=0;
 struct Part{unsigned index,first,pages;};std::vector<Part> selected;
 unsigned opens=0,closes=0,calls=0,begins=0,spans=0,spanEnds=0,runs=0,ends=0,uploaded=0;
 unsigned failCall=0,badCall=0,sourceDrop=0,epochDrop=0,corruptWord=UINT32_MAX,badReply=UINT32_MAX;
 bool stopped=false,nonfinite=false,failOpen=false;
 ProjectionValidationIO(ProjectionValidationSource& s,unsigned id):source(s),token(id){
  assert(plan.Build(source.Model(),8192,true,true));
  for(unsigned i=0;i<plan.Parts().size();i++){
   const auto& p=plan.Parts()[i];if(p.kind!=PIONEER_Q38_WEIGHT)continue;
   const auto& t=source.Model().Tensors()[p.tensor];
   if(t.name=="token_embd.weight" && token>=p.firstRow && token-p.firstRow<p.rows)
    selected.insert(selected.begin(),{i,((token-p.firstRow)/16*46080)/4096,12});
   if(!p.layer && (p.role==0 || p.role==5 || p.role==6 || p.role==10 || p.role==11))selected.push_back({i,0,p.allocated/4096});
  }assert(selected.size()==18);
 }
 int Open(){assert(source.Ready() && source.referenceReads==1032 && !source.gpuOpen && !stopped);opens++;if(failOpen)return -1;source.gpuOpen=true;return 73;}
 int Close(int fd){assert(fd==73 && !closes);closes++;return 0;}
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==73 && !closes && !stopped && source.referenceReads==1032);bool bad=++calls==badCall;
  switch(op){
   case TT_PCI_PROBE_RELOAD_STATUS:{auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
    assert(bytes==sizeof(r) && r.version==1);r.driver=55;r.guard_version=1;r.open_count=1;r.generation=4;
    r.build_tag=bad?UINT64_C(0x2026102200010041):UINT64_C(0x2026102200010042);break;}
   case TT_PCI_PROBE_Q38_PROJECTION_VALIDATION_SUPPORT:
    assert(bytes==sizeof(tt_q38_embedding_support));*static_cast<tt_q38_embedding_support*>(data)={1,55,bad?12U:13U,16480,248320,512,0,0};break;
   case TT_PCI_PROBE_Q38_BANK_SUPPORT:
    *static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,bad?3U:4U,1,PioneerResidentBanks::Capacity};break;
   case TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT:
    *static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,bad?1U:0U};break;
   case TT_PCI_PROBE_Q38_BANK_BEGIN:{begins++;auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    r.version=1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=PioneerResidentBanks::Total;r.cold=bad;break;}
   case TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD:{auto& r=*static_cast<tt_q38_span_upload*>(data);
    assert(partAt<selected.size());const auto& part=selected[partAt];const auto& p=plan.Parts()[part.index];
    unsigned pages=std::min(256U,part.pages-pageAt);
    assert(begins==1 && !runs && !spanEnds && bytes==sizeof(r) && r.version==1 && !r.reserved
     && r.bank==p.bank && r.offset==p.proposedAddress-plan.Base+uint64_t(part.first+pageAt)*4096
     && r.bytes==pages*4096 && !r.result.state);
    for(unsigned page=0;page<pages;page++){
     uint8_t original[4096];assert(plan.ReadWeightPage(source.model,part.index,part.first+pageAt+page,original,4096));
     assert(!memcmp(original,r.data+page*1024,4096));
    }
    for(unsigned word=pages*1024;word<262144;word++)assert(!r.data[word]);
    spans++;uploaded+=pages;pageAt+=pages;if(pageAt==part.pages){partAt++;pageAt=0;}
    r.result={1,TT_RUNTIME_PASSED,pages*4096,pages,spans,1,1,1,bad?1U:0U,0,0,0};break;}
   case TT_PCI_PROBE_Q38_SPAN_END:
    assert(partAt==18 && !pageAt && uploaded==23193 && !runs);spanEnds++;*static_cast<tt_q38_span_end*>(data)={1,0,bad?0U:1U,0};break;
   case TT_PCI_PROBE_Q38_PROJECTION_VALIDATE:{runs++;auto& reply=*static_cast<tt_q38_projection_validation*>(data);auto& r=reply.metadata;
    assert(bytes==sizeof(reply) && PioneerQ38EmbeddingRequest(r) && r.token==token && r.position==8191 && spanEnds==1);
    const auto* raw=reinterpret_cast<const uint8_t*>(&reply);
    for(unsigned i=sizeof(r);i<sizeof(reply);i++)assert(!raw[i]);
    r.state=TT_RUNTIME_PASSED;r.closed=r.input_ready=1;r.session=7;r.driver_generation=4;r.elapsed_us=123;
    r.source_pages=23193;r.workspace_pages=512;r.saved_words=r.restored_words=8281546;r.recovery_required=bad;
    reply.projections=4;reply.peer_boots=reply.peer_stopped=reply.peer_held=119;reply.root_starts=reply.root_holds=5;
    reply.readback_pages=18;reply.readback_words=18432;reply.saved_words=reply.restored_words=8134072;
    reply.verified_words=16268144;reply.installed_words=989304;
    if(corruptWord<18432){reply.values[corruptWord]=nonfinite?0x7fc00000:1;stopped=true;}
    if(badReply<30){
     uint32_t v;auto* field=reinterpret_cast<uint8_t*>(&reply)+badReply*4;memcpy(&v,field,4);
     // Every checked fixed-width field,including high half of 64bit IDs.
     if(badReply==8 || badReply==12)v=0;else v^=1;
     memcpy(field,&v,4);stopped=true;
    }break;}
   case TT_PCI_PROBE_Q38_BANK_END:{ends++;assert(runs==1);auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    r.version=1;r.state=TT_RESIDENT_SESSION_CLOSED;r.host_released=1;r.pci_restored=!bad;r.bytes=PioneerResidentBanks::Total;break;}
   default:assert(!"Old namespace/Work/numeric input/fallback endpoint MUST NOT be called");
  }
  if(calls==sourceDrop)source.valid=false;
  if(calls==epochDrop)assert(source.model.Open(source.file));
  if(bad || calls==failCall || calls==sourceDrop || calls==epochDrop)stopped=true;
  return calls!=failCall;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;unsigned positives=0,faults=0,totalCalls=0,spans=0;
 for(unsigned token:{0U,15U,248319U}){
  SeedProjectionValidationEmbedding(fixture,token);ProjectionValidationSource source(fixture.file);ProjectionValidationIO io(source,token);
  PioneerQ38ProjectionValidationSession<ProjectionValidationIO,ProjectionValidationSource> session(io,source);
  assert(session.Validate(token,8191) && source.referenceReads==1032 && source.reads==23193 && session.Finish()
   && io.ends==1 && io.closes==1 && io.calls==io.spans+8);totalCalls=io.calls;spans=io.spans;positives++;
  printf("PASS projection DEBUG session token%u/source pages%u/spans%u/calls%u\n",token,source.reads,spans,totalCalls);
 }
 SeedProjectionValidationEmbedding(fixture,0);
 auto check=[&](unsigned kind,unsigned at){
  ProjectionValidationSource source(fixture.file);ProjectionValidationIO io(source,0);
  if(kind==0)io.failCall=at;if(kind==1)io.badCall=at;if(kind==2)io.sourceDrop=at;if(kind==3)io.epochDrop=at;
  if(kind==4)source.failRead=at;if(kind==5)source.failReference=at;if(kind==6)io.corruptWord=at;
  if(kind==7){io.corruptWord=at;io.nonfinite=true;}if(kind==8)io.badReply=at;if(kind==9)io.failOpen=true;
  PioneerQ38ProjectionValidationSession<ProjectionValidationIO,ProjectionValidationSource> session(io,source);
  bool valid=session.Validate(0,8191);assert(valid==(kind<4 && at==totalCalls));
  assert(!session.Finish() && !session.Validate(0,8191));
  if(kind<4)assert(io.calls==at && io.ends==unsigned(at==totalCalls));
  else if(kind==4)assert(source.reads==at && !io.runs && !io.ends);
  else if(kind==5)assert(source.referenceReads==at && !io.opens && !io.calls);
  else if(kind==9)assert(io.opens==1 && !source.gpuOpen && !io.calls);
  else assert(io.runs==1 && io.calls==totalCalls-1 && !io.ends);
  assert(io.closes==unsigned(source.gpuOpen));faults++;
  printf("PASS projection DEBUG session refusal kind%u/at%u\n",kind,at);
 };
 // Handshakes,first+last original upload,SPAN_END,run,explicit BANK_END.
 for(unsigned kind=0;kind<4;kind++)for(unsigned at:{1U,2U,3U,4U,5U,6U,5U+spans,6U+spans,7U+spans,8U+spans})check(kind,at);
 for(unsigned at:{1U,12U,13U,23193U})check(4,at);
 for(unsigned at:{1U,2U,3U,1032U})check(5,at);
 // Each page boundary,including padding between the two 48-value outputs.
 for(unsigned page=0;page<18;page++)for(unsigned at:{page*1024,page*1024+1023})check(6,at);
 for(unsigned at:{0U,47U,48U,1024U,1071U,1072U,2048U,8191U,8192U,18431U})check(7,at);
 // 18 metadata words: session nonzero + exact driver generation,elapsed
 // nonzero. High session/elapsed halves are not constrained,so omit them.
 for(unsigned at:{0U,1U,2U,3U,4U,5U,6U,7U,8U,10U,11U,12U,14U,15U,16U,17U,
  18U,19U,20U,21U,22U,23U,24U,25U,26U,27U,28U,29U})check(8,at);
 check(9,0);
 printf("PASS source-bound projection DEBUG session positives%u/faults%u;1032 original reference reads BEFORE FD,23193 original pages,tag42/namespace13,ALL18432words/padding checked,CLOSE ONLY after first fault;SYNTHETIC_GGUF_TRANSPORT_NOT_CARD_ARITHMETIC_OR_SPEED\n",positives,faults);
}
