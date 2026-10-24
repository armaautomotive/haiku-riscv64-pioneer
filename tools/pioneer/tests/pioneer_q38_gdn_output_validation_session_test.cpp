/* SPDX-License-Identifier: MIT */
// Production DEBUG55 session;synthetic original GGUF and literal OS replies.
// Transport NEVER calls a production reference or uploads CPU products.
#define PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_q38_projection_validation_fixture.h"
#include "../PioneerQ38GdnOutputValidationSession.h"
struct OutputSessionSource:ProjectionValidationSource {
 bool returned=false;unsigned postReferences=0,dropPost=0,epochPost=0;
 explicit OutputSessionSource(FILE* f):ProjectionValidationSource(f){}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
  if(gpuOpen){
   assert(returned && model.Tensors().at(tensor).name=="blk.0.ssm_out.weight" && !(offset%(16*6528)) && bytes==16*6528 && capacity==bytes);
   postReferences++;
  }
  bool okay=++referenceReads!=failReference && Ready() && model.ReadPacked(tensor,offset,bytes,out,capacity) && Ready();
  if(gpuOpen && postReferences==dropPost)valid=false;
  if(gpuOpen && postReferences==epochPost)assert(model.Open(file));
  return okay;
 }
};
static void SeedOutputSession(Fixture& f,bool dense=false){
 for(const auto& t:f.expected){
  if(t.name=="blk.0.ssm_a"){
   std::vector<uint8_t> data(192);for(unsigned i=0;i<48;i++){uint32_t b=0xbf800000;memcpy(data.data()+i*4,&b,4);}f.Write(t.offset,data);
  }
  if(!dense)continue;
  if(t.name=="token_embd.weight"){
   std::vector<uint8_t> data(2880);
   for(unsigned b=0;b<20;b++){auto* p=data.data()+b*144;p[1]=0x3c;
    for(unsigned g=0;g<4;g++)p[4+g]=p[12+g]=1;
    for(unsigned i=16;i<144;i++)p[i]=0x11;
   }f.Write(t.offset,data); // token0 ORIGINAL embedding is exactly1
  }
  if(t.name=="blk.0.attn_norm.weight" || t.name=="blk.0.ssm_norm.weight" || t.name=="blk.0.ssm_conv1d.weight"){
   std::vector<uint8_t> data(size_t(t.bytes));
   for(unsigned i=0;i<t.bytes/4;i++){
    uint32_t b=t.name=="blk.0.attn_norm.weight"?0x42fe0008:t.name=="blk.0.ssm_norm.weight"?(i?0x3f800000:0x3b810204):(i%4==3?0x3f800000:0);
    memcpy(data.data()+i*4,&b,4);
   }f.Write(t.offset,data);
  }
  for(unsigned role:{5U,6U,13U}){
   PioneerQ38Tensor expected;assert(PioneerQ38Expected(0,role,expected));if(t.name!=expected.name)continue;
   unsigned rowBytes=role==13?6528:5440;
   for(unsigned first=0;first<t.outputs;first+=16){
    std::vector<uint8_t> data(16*rowBytes);
    for(unsigned row=0;row<16;row++){
     unsigned index=first+row;bool used=role==5 || (role==6?(index>=4096 || index%128==0):(first==0 || first==5104));
     if(used){auto* p=data.data()+row*rowBytes;p[1]=0x3c;p[2]=role==13?uint8_t(row&1?128:127):1;}
    }f.Write(t.offset+uint64_t(first)*rowBytes,data);
   }
  }
 }
}
struct OutputSessionIO {
 OutputSessionSource& source;PioneerQwen38PackedPlan plan;unsigned token,partAt=0,pageAt=0;
 struct Part{unsigned index,first,pages;};std::vector<Part> selected;
 unsigned opens=0,closes=0,calls=0,begins=0,spans=0,spanEnds=0,runs=0,ends=0,uploaded=0;
 unsigned failCall=0,sourceDrop=0,epochDrop=0,corruptWord=UINT32_MAX,badReply=UINT32_MAX;
 unsigned badCall=0,badOffset=0;uint32_t badXor=1;
 uint32_t corruptBits=0x3f800000,returnedHeader[64]={},smallBits=0x3f000001;
 bool stopped=false,closeFail=false,dense=false,openFail=false;
 bool (*earlyFinish)(void*)=nullptr;void* client=nullptr;unsigned earlyChecks=0;
 OutputSessionIO(OutputSessionSource& s,unsigned id):source(s),token(id){
  assert(plan.Build(source.Model(),8192,true,true));
  for(unsigned i=0;i<plan.Parts().size();i++){
   const auto& p=plan.Parts()[i];if(p.kind!=PIONEER_Q38_WEIGHT)continue;const auto& t=source.Model().Tensors()[p.tensor];
   if(t.name=="token_embd.weight" && token>=p.firstRow && token-p.firstRow<p.rows)
    selected.insert(selected.begin(),{i,((token-p.firstRow)/16*46080)/4096,12});
   if(!p.layer && (p.role==0 || p.role==5 || p.role==6 || p.role==7 || p.role==8 || p.role==9 || p.role==10 || p.role==11 || p.role==12 || p.role==13))
    selected.push_back({i,0,p.allocated/4096});
  }assert(selected.size()==35);
 }
 int Open(){assert(source.Ready() && source.referenceReads==1675 && !source.gpuOpen && !stopped);opens++;if(openFail)return -1;source.gpuOpen=true;return 73;}
 int Close(int fd){assert(fd==73 && !closes);closes++;source.gpuOpen=false;return closeFail?-1:0;}
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==73 && !closes && !stopped && source.referenceReads==(op==TT_PCI_PROBE_Q38_BANK_END?1995U:1675U));calls++;
  if(earlyFinish && (op==TT_PCI_PROBE_Q38_BANK_BEGIN || op==TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE)){
   unsigned before=calls;assert(!earlyFinish(client) && calls==before);earlyChecks++;
  }
  switch(op){
   case TT_PCI_PROBE_RELOAD_STATUS:{auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
    assert(bytes==sizeof(r) && r.version==1);r.driver=55;r.guard_version=1;r.open_count=1;r.generation=4;r.build_tag=UINT64_C(0x2026102400010055);break;}
   case TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATION_SUPPORT:
    assert(bytes==sizeof(tt_q38_embedding_support));*static_cast<tt_q38_embedding_support*>(data)={1,55,18,838016,248320,512,0,0};break;
   case TT_PCI_PROBE_Q38_BANK_SUPPORT:*static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,PioneerResidentBanks::Capacity};break;
   case TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT:*static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};break;
   case TT_PCI_PROBE_Q38_BANK_BEGIN:{begins++;auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    r.version=1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=PioneerResidentBanks::Total;break;}
   case TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD:{auto& r=*static_cast<tt_q38_span_upload*>(data);assert(partAt<selected.size());
    const auto& part=selected[partAt];const auto& p=plan.Parts()[part.index];unsigned pages=std::min(256U,part.pages-pageAt);
    assert(begins==1 && !runs && !spanEnds && bytes==sizeof(r) && r.version==1 && !r.reserved && r.bank==p.bank
     && r.offset==p.proposedAddress-plan.Base+uint64_t(part.first+pageAt)*4096 && r.bytes==pages*4096 && !r.result.state);
    for(unsigned page=0;page<pages;page++){uint8_t original[4096];assert(plan.ReadWeightPage(source.model,part.index,part.first+pageAt+page,original,4096));
     assert(!memcmp(original,r.data+page*1024,4096));}
    for(unsigned word=pages*1024;word<262144;word++)assert(!r.data[word]);
    spans++;uploaded+=pages;pageAt+=pages;if(pageAt==part.pages){partAt++;pageAt=0;}
    r.result={1,TT_RUNTIME_PASSED,pages*4096,pages,spans,1,1,1,0,0,0,0};break;}
   case TT_PCI_PROBE_Q38_SPAN_END:
    assert(partAt==35 && !pageAt && uploaded==31878 && spans==149 && !runs);spanEnds++;*static_cast<tt_q38_span_end*>(data)={1,0,1,0};break;
   case TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE:{runs++;auto& reply=*static_cast<tt_q38_gdn_output_validation*>(data);auto& r=reply.metadata;
    assert(bytes==sizeof(reply) && PioneerQ38EmbeddingRequest(r) && r.token==token && !r.position && spanEnds==1);
    const auto* raw=reinterpret_cast<const uint8_t*>(&reply);for(unsigned i=sizeof(r);i<sizeof(reply);i++)assert(!raw[i]);
    r.state=TT_RUNTIME_PASSED;r.closed=r.input_ready=1;r.session=7;r.driver_generation=4;r.elapsed_us=123;
    r.source_pages=31878;r.workspace_pages=512;r.saved_words=r.restored_words=8478154;
    reply.gated_published=1;reply.readback_half_words=8642;reply.state_runs=672;reply.state_pages=73728;reply.layer_published=1;
    reply.history_runs=336;reply.history_pages=1680;reply.projections=4;reply.peer_boots=reply.peer_stopped=reply.peer_held=119;
    reply.root_starts=reply.root_holds=5;reply.convolution_workers=120;reply.convolution_jobs=640;reply.convolution_channels=10240;reply.convolution_held=120;
    reply.input_workers=reply.input_installed=reply.input_armed=reply.input_released=reply.input_completed=reply.input_held=96;
    reply.readback_workers=96;reply.readback_words=829632;reply.saved_words=reply.restored_words=8330680;reply.verified_words=16661360;reply.installed_words=1185912;
    reply.output_installed=120;reply.output_booted=119;reply.output_root_runs=2;reply.output_stopped=119;reply.output_published=1;
    reply.output_pages=5;reply.output_words=5120;reply.operand_words=3072;reply.scale_words=192;
    // Literal closed forms,not a call to any production oracle. Dense
    // case has onehot Q=K=1,V=127,beta=.5,raw=63.5/sqrt128. Norm/gate
    // results are within strict independent bounds;smallBits straddles
    // the Q8 half boundary without changing the ORIGINAL model at all.
    for(unsigned half=0;half<96;half++){
     auto* v=reply.values+half*8642;v[320]=v[321]=0x3f000000;
     if(dense){v[0]=v[128]=0x3f800000;
      for(unsigned i=0;i<64;i++){v[256+i]=0x42fe0000;v[322+i]=0x427e0000;v[8514+i]=0x40b39ae9;v[8578+i]=!(half&1) && !i?smallBits:0x42fe0000;}
     }
    }
    for(unsigned i=0;i<3072;i++){
     uint32_t pair=0;
     for(unsigned h=0;h<2;h++){unsigned at=i*2+h,q=dense?(at%128?127U:smallBits>=0x3f000000?1U:0U):0U;pair|=(0x4000|q)<<(h*16);}
     reply.values[829632+i]=pair;
    }
    for(unsigned i=0;i<192;i++)reply.values[832704+i]=dense?0x3f800000:0;
    if(dense && smallBits>=0x3f000000)for(unsigned row=0;row<5120;row++)if(row<16 || row>=5104)
     reply.values[832896+row]=row&1?0xc3000000:0x42fe0000; // ORIGINAL -128/+127 learned bytes,not clipping
    if(corruptWord<838016){reply.values[corruptWord]=corruptBits;stopped=true;}
    if(badReply<64){uint32_t value;auto* field=reinterpret_cast<uint8_t*>(&reply)+badReply*4;memcpy(&value,field,4);
     if(badReply==8 || badReply==12)value=0;else value^=1;memcpy(field,&value,4);stopped=true;}
    memcpy(returnedHeader,&reply,sizeof(returnedHeader));source.returned=true;break;}
   case TT_PCI_PROBE_Q38_BANK_END:{ends++;assert(runs==1);auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    r.version=1;r.state=TT_RESIDENT_SESSION_CLOSED;r.host_released=r.pci_restored=1;r.bytes=PioneerResidentBanks::Total;break;}
   default:assert(!"Old namespace/Work/numeric input/fallback endpoint MUST NOT be called");
  }
  if(calls==badCall){assert(badOffset+4<=bytes);uint32_t value;auto* field=static_cast<uint8_t*>(data)+badOffset;
   memcpy(&value,field,4);value^=badXor;memcpy(field,&value,4);stopped=true;}
  if(calls==sourceDrop)source.valid=false;if(calls==epochDrop)assert(source.model.Open(source.file));
  if(calls==failCall || calls==sourceDrop || calls==epochDrop)stopped=true;
  return calls!=failCall;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;SeedOutputSession(fixture);unsigned positives=0,faults=0;
 using Session=PioneerQ38GdnOutputValidationSession<OutputSessionIO,OutputSessionSource>;using Failure=Session::Failure;
 for(unsigned token:{0U,15U,248319U}){
  SeedProjectionValidationEmbedding(fixture,token);OutputSessionSource source(fixture.file);OutputSessionIO io(source,token);Session session(io,source);
  io.client=&session;io.earlyFinish=[](void* p){return static_cast<Session*>(p)->Finish();};
  assert(!session.Finish() && !session.Validate(token,1) && !io.opens);bool valid=session.Validate(token);
  printf("DEBUG55_SESSION_OBS valid%u reason%s op%x word%u expected%x actual%x refs%u post%u pages%u calls%u spans%u early%u\n",
   unsigned(valid),session.FailureName(session.diagnostics.reason),session.diagnostics.op,session.diagnostics.word,session.diagnostics.expected,session.diagnostics.actual,
   source.referenceReads,source.postReferences,source.reads,io.calls,io.spans,io.earlyChecks);
  assert(valid && source.referenceReads==1995 && source.postReferences==320 && source.reads==31878
   && session.Finish() && io.ends==1 && io.closes==1 && io.calls==157 && io.spans==149 && io.earlyChecks==2);
  assert(session.diagnostics.reason==Failure::None && session.diagnostics.reply_captured);positives++;
  printf("PASS DEBUG55 session token%u/1675GDN refs BEFORE FD+320output refs AFTER validated actual GDN/31878originalpages/149spans/157calls/earlyEND inert\n",token);
 }
 SeedProjectionValidationEmbedding(fixture,0);
 auto check=[&](unsigned kind,unsigned at){
  OutputSessionSource source(fixture.file);OutputSessionIO io(source,0);
  if(kind==0)io.failCall=at;if(kind==1)io.sourceDrop=at;if(kind==2)io.epochDrop=at;if(kind==3)source.failRead=at;
  if(kind==4)source.failReference=at;if(kind==5){io.corruptWord=at;io.corruptBits=0x7fc00000;}
  if(kind==6)io.badReply=at;if(kind==7){io.corruptWord=at;io.corruptBits=0x40004001;}
  if(kind==8){io.corruptWord=at;io.corruptBits=0x3f800000;}if(kind==9){io.corruptWord=at;io.corruptBits=1;}
  if(kind==10)source.dropPost=at;if(kind==11)source.epochPost=at;
  Session session(io,source);bool valid=session.Validate(0);
  assert(valid==(kind<3 && at==157));assert(!session.Finish() && !session.Validate(0));
  if(kind<3)assert(io.calls==at && io.ends==unsigned(at==157));
  else if(kind==3)assert(source.reads==at && !io.runs && !io.ends);
  else if(kind==4)assert(source.referenceReads==at && (at<=1675?!io.opens:io.calls==156) && !io.ends);
  else assert(io.runs==1 && io.calls==156 && !io.ends);
  assert(io.closes==io.opens);const auto& d=session.diagnostics;assert(d.reason!=Failure::None && d.reply_captured==bool(io.runs));
  if(io.runs)assert(!memcmp(d.reply_header,io.returnedHeader,sizeof(d.reply_header)));
  if(kind==0)assert(d.reason==Failure::Transport);
  if(kind==1 || kind==2 || kind==10 || kind==11)assert(d.reason==Failure::SourceChanged);
  if(kind==4 && at>1675)assert(d.reason==Failure::Reference);
  if(kind==5){assert(d.reason==(at>=829632 && at<832704?Failure::OperandMismatch:Failure::NonFinite) && d.word==at && d.actual==0x7fc00000);}
  if(kind==6)assert(d.reason==Failure::ReplyMetadata && d.word==at && !source.postReferences);
  if(kind==7)assert(d.reason==Failure::OperandMismatch && d.word==at);
  if(kind==8)assert(d.reason==Failure::ScaleMismatch && d.word==at);
  if(kind==9)assert(d.reason==Failure::OutputMismatch && d.word==at);
  if((kind==5 && at<829632) || kind==6)assert(!source.postReferences); // NEVER derive projection from rejected GDN/header
  if(kind==7 || kind==8 || kind==9)assert(source.postReferences==320);
  auto before=d;session.Abort();assert(d.reason==before.reason && d.op==before.op && d.word==before.word && d.expected==before.expected
   && d.actual==before.actual && !memcmp(d.reply_header,before.reply_header,sizeof(d.reply_header)));faults++;
 };
 for(unsigned kind=0;kind<3;kind++)for(unsigned at:{1U,5U,6U,154U,155U,156U,157U})check(kind,at);
 for(unsigned at:{1U,12U,31877U,31878U})check(3,at);
 for(unsigned at:{1U,2U,640U,1282U,1288U,1673U,1674U,1675U,1676U,1835U,1995U})check(4,at);
 for(unsigned at:{0U,320U,321U,8513U,8514U,8578U,8641U,829631U,829632U,832703U,832704U,832895U,832896U,838015U})check(5,at);
 for(unsigned at=0;at<64;at++)if(at!=9 && at!=13)check(6,at);
 for(unsigned at:{829632U,831168U,832703U})check(7,at);
 for(unsigned at:{832704U,832800U,832895U})check(8,at);
 for(unsigned at:{832896U,835456U,838015U})check(9,at);
 for(unsigned kind:{10U,11U})for(unsigned at:{1U,160U,320U})check(kind,at);
 auto metadata=[&](unsigned call,unsigned offset,uint32_t bits=1){
  OutputSessionSource source(fixture.file);OutputSessionIO io(source,0);io.badCall=call;io.badOffset=offset;io.badXor=bits;
  Session session(io,source);bool valid=session.Validate(0);assert(valid==(call==157));
  assert(!session.Finish() && io.calls==call && io.closes==1 && io.ends==unsigned(call==157));
  assert(session.diagnostics.reason==(call==157?Failure::EndMetadata:Failure::SessionContract));
  auto diagnostic=session.diagnostics;session.Abort();assert(!session.Validate(0) && !session.Finish() && io.calls==call
   && session.diagnostics.reason==diagnostic.reason && session.diagnostics.op==diagnostic.op);faults++;
 };
 // Every checked handshake/upload/END field. A corrupt first reply must
 // never be followed by another IO,including an attempted cleanup END.
 for(unsigned offset:{0U,32U,36U,40U,44U,48U,52U,56U,60U})metadata(1,offset);
 metadata(1,64,4); // actual generation4 -> zero,not a different valid nonzero generation
 for(unsigned call:{2U,3U,4U})for(unsigned word=0;word<8;word++)metadata(call,word*4);
 for(unsigned word:{0U,1U,2U,3U,7U,8U,9U,12U})metadata(5,8+word*4);
 for(unsigned call:{6U,154U})for(unsigned word=0;word<12;word++)metadata(call,24+word*4);
 for(unsigned word=0;word<4;word++)metadata(155,word*4);
 for(unsigned word:{0U,1U,2U,4U,5U,7U,8U,9U,12U})metadata(157,8+word*4);
 {OutputSessionSource source(fixture.file);OutputSessionIO io(source,0);io.openFail=true;Session session(io,source);
  assert(!session.Validate(0) && !session.Finish() && io.opens==1 && !io.calls && !io.closes && !source.gpuOpen);faults++;}
 {OutputSessionSource source(fixture.file);OutputSessionIO io(source,0);Session session(io,source);
  assert(!session.Validate(248320) && !session.Validate(0,1) && !io.opens && !source.referenceReads);
  source.valid=false;assert(!session.Validate(0) && !io.opens && !source.referenceReads && !session.Finish());faults++;}
 {OutputSessionSource source(fixture.file);OutputSessionIO io(source,0);io.closeFail=true;Session session(io,source);
  assert(session.Validate(0) && !session.Finish() && session.diagnostics.reason==Failure::Close && io.ends==1 && io.closes==1);faults++;}
 {OutputSessionSource source(fixture.file);OutputSessionIO io(source,0);{Session session(io,source);assert(session.Validate(0));}assert(io.closes==1 && !io.ends);faults++;}
 Fixture dense;SeedOutputSession(dense,true);
 for(uint32_t small:{0x3efffffeU,0x3f000001U}){
  OutputSessionSource source(dense.file);OutputSessionIO io(source,0);io.dense=true;io.smallBits=small;Session session(io,source);
  assert(session.Validate(0) && session.Finish() && source.postReferences==320);positives++;
  printf("PASS DEBUG55 SAME original model/nonzero GDN/accepted actual gated bits%08x/DIFFERENT Q8 operands+ssm_out rows BOTH validated;original -128/+127 kept\n",small);
 }
 printf("PASS DEBUG55 isolated session positives%u/faults%u/tag55/namespace18/strict GDN FIRST/exact actual-input projection/64word first-failure snapshot/explicit validated-only END/CLOSE ONLY after first failure;SYNTHETIC_NOT_PHYSICAL_OR_SPEED\n",positives,faults);
}
