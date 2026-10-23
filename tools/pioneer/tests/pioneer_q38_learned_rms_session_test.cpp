/* SPDX-License-Identifier: MIT */
// Synthetic GGUF/source/transport tests ONLY,not native card arithmetic.
#define PIONEER_Q38_FIXTURE_ONLY
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38LearnedRmsSession.h"
struct RmsSource {
 PioneerQwen38Gguf model;FILE* file;bool valid=true,gpuOpen=false;
 unsigned reads=0,referenceReads=0,failRead=0,failReference=0;
 explicit RmsSource(FILE* f):file(f){assert(model.Open(f));}
 bool Ready()const{return valid && model.Stable();}const auto& Model()const{return model;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
  assert(!gpuOpen);return ++referenceReads!=failReference && Ready() && model.ReadPacked(tensor,offset,bytes,out,capacity) && Ready();
 }
 bool ReadWeightPage(const PioneerQwen38PackedPlan& plan,unsigned part,unsigned page,uint8_t* out,unsigned bytes){
  return Ready() && ++reads!=failRead && plan.ReadWeightPage(model,part,page,out,bytes);
 }
};
static float NormWeight(unsigned i){return 0.5f+float(i%7)*0.125f;}
static void Seed(Fixture& f,unsigned token){
 for(const auto& t:f.expected){
  if(t.name=="token_embd.weight"){
   std::vector<uint8_t> row(2880);unsigned shift=(token*7+token/16)%16;
   for(unsigned block=0;block<20;block++){
    auto* b=row.data()+block*144;b[1]=b[3]=0x3c;
    for(unsigned g=0;g<4;g++){b[4+g]=1;b[8+g]=2;b[12+g]=0x21;}
    for(unsigned g=0;g<4;g++)for(unsigned c=0;c<32;c++)b[16+g*32+c]=uint8_t((c+block+shift)%16|(((c+block+shift+7)%16)<<4));
   }f.Write(t.offset+uint64_t(token)*2880,row);
  }
  if(t.name=="blk.0.attn_norm.weight"){
   std::vector<uint8_t> bytes(20480);
   for(unsigned i=0;i<5120;i++){uint32_t value=PioneerAttentionBits(NormWeight(i));for(unsigned b=0;b<4;b++)bytes[i*4+b]=uint8_t(value>>(b*8));}
   f.Write(t.offset,bytes);
  }
 }
}
struct RmsIO {
 RmsSource& source;PioneerQwen38PackedPlan plan;unsigned token,embedding=UINT32_MAX,norm=UINT32_MAX,firstPage=0;
 unsigned opens=0,closes=0,calls=0,begins=0,spans=0,spanEnds=0,runs=0,ends=0;
 unsigned failCall=0,badCall=0,sourceDrop=0,epochDrop=0,corruptWord=UINT32_MAX,corruptPlane=0;
 bool stopped=false,nonfinite=false;
 RmsIO(RmsSource& s,unsigned id):source(s),token(id){
  assert(plan.Build(source.Model(),8192,true,true));
  for(unsigned i=0;i<plan.Parts().size();i++){
   const auto& p=plan.Parts()[i];if(p.kind!=PIONEER_Q38_WEIGHT)continue;
   const auto& t=source.Model().Tensors()[p.tensor];
   if(t.name=="token_embd.weight" && token>=p.firstRow && token-p.firstRow<p.rows){embedding=i;firstPage=((token-p.firstRow)/16*46080)/4096;}
   if(t.name=="blk.0.attn_norm.weight")norm=i;
  }assert(embedding!=UINT32_MAX && norm!=UINT32_MAX);
 }
 int Open(){assert(source.Ready() && source.referenceReads==2 && !source.gpuOpen && !stopped);source.gpuOpen=true;opens++;return 73;}
 int Close(int fd){assert(fd==73 && !closes);closes++;return 0;}
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==73 && !closes && !stopped);bool bad=++calls==badCall;
  switch(op){
   case TT_PCI_PROBE_RELOAD_STATUS:{auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
    assert(bytes==sizeof(r) && r.version==1);r.driver=55;r.guard_version=1;r.open_count=1;r.generation=4;
    r.build_tag=bad?UINT64_C(0x2026102200010040):UINT64_C(0x2026102200010041);break;}
   case TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT:
    *static_cast<tt_q38_embedding_support*>(data)={1,55,bad?11U:12U,5120,248320,512,0,0};break;
   case TT_PCI_PROBE_Q38_BANK_SUPPORT:
    *static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,bad?3U:4U,1,PioneerResidentBanks::Capacity};break;
   case TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT:
    *static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,bad?1U:0U};break;
   case TT_PCI_PROBE_Q38_BANK_BEGIN:{begins++;auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    r.version=1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=PioneerResidentBanks::Total;r.cold=bad;break;}
   case TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD:{auto& r=*static_cast<tt_q38_span_upload*>(data);
    unsigned part=spans?norm:embedding,first=spans?0:firstPage,pages=spans?5:12;const auto& p=plan.Parts()[part];
    assert(begins==1 && spans<2 && !runs && !spanEnds && bytes==sizeof(r) && r.version==1 && !r.reserved
     && r.bank==p.bank && r.offset==p.proposedAddress-plan.Base+uint64_t(first)*4096 && r.bytes==pages*4096 && !r.result.state);
    for(unsigned page=0;page<pages;page++){uint8_t original[4096];assert(plan.ReadWeightPage(source.model,part,first+page,original,4096));assert(!memcmp(original,r.data+page*1024,4096));}
    spans++;r.result={1,TT_RUNTIME_PASSED,pages*4096,pages,spans,1,1,1,bad?1U:0U,0,0,0};break;}
   case TT_PCI_PROBE_Q38_SPAN_END:
    assert(spans==2 && !runs);spanEnds++;*static_cast<tt_q38_span_end*>(data)={1,0,bad?0U:1U,0};break;
   case TT_PCI_PROBE_Q38_RMS_VALIDATE:{runs++;auto& reply=*static_cast<tt_q38_learned_rms_validation*>(data);auto& r=reply.metadata;
    assert(bytes==sizeof(reply) && PioneerQ38EmbeddingRequest(r) && r.token==token && r.position==8191 && spanEnds==1);
    for(unsigned i=0;i<5120;i++)assert(!reply.normalized[i] && !reply.residual[i]);
    double sum=0;unsigned shift=(token*7+token/16)%16;
    for(unsigned i=0;i<5120;i++){
     unsigned block=i/256,group=i/32%8,column=i%32;
     float value=float(int((column+block+shift+((group&1)?7:0))%16)-2);
     reply.residual[i]=PioneerAttentionBits(value);float square=value*value;sum+=double(square);
    }
    float scale=1.f/sqrtf(float(sum/5120)+1e-6f);
    for(unsigned i=0;i<5120;i++)reply.normalized[i]=PioneerAttentionBits((PioneerAttentionFloat(reply.residual[i])*scale)*NormWeight(i));
    if(corruptWord<5120){auto& value=(corruptPlane?reply.residual:reply.normalized)[corruptWord];value=nonfinite?0x7fc00000:(value^1);stopped=true;}
    r.state=TT_RUNTIME_PASSED;r.closed=r.input_ready=1;r.session=7;r.driver_generation=4;r.elapsed_us=123;
    r.source_pages=17;r.workspace_pages=512;r.saved_words=r.restored_words=147474;r.recovery_required=bad;break;}
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
 Fixture fixture;unsigned positives=0,faults=0;
 for(unsigned token:{0U,15U,35519U,71039U,106495U,141951U,177407U,212863U,248319U}){
  Seed(fixture,token);RmsSource source(fixture.file);RmsIO io(source,token);PioneerQ38LearnedRmsSession<RmsIO,RmsSource> session(io,source);
  assert(session.Validate(token,8191) && source.referenceReads==2 && source.reads==17 && session.Finish()
   && io.calls==10 && io.ends==1 && io.closes==1);positives++;
 }
 Seed(fixture,0);
 for(unsigned plane=0;plane<2;plane++)for(unsigned word:{0U,31U,255U,1023U,1024U,4095U,5119U})for(bool nan:{false,true}){
  RmsSource source(fixture.file);RmsIO io(source,0);io.corruptWord=word;io.corruptPlane=plane;io.nonfinite=nan;
  PioneerQ38LearnedRmsSession<RmsIO,RmsSource> session(io,source);
  assert(!session.Validate(0,8191) && !session.Finish() && !session.Validate(0,8191)
   && source.referenceReads==2 && io.calls==9 && !io.ends && io.closes==1);faults++;
 }
 for(unsigned category=0;category<4;category++)for(unsigned at=1;at<=10;at++){
  RmsSource source(fixture.file);RmsIO io(source,0);
  if(category==0)io.failCall=at;if(category==1)io.badCall=at;if(category==2)io.sourceDrop=at;if(category==3)io.epochDrop=at;
  PioneerQ38LearnedRmsSession<RmsIO,RmsSource> session(io,source);
  assert(session.Validate(0,8191)==(at==10));assert(!session.Finish() && io.calls==at && io.closes==1 && io.ends==unsigned(at==10));faults++;
 }
 for(unsigned at=1;at<=17;at++){
  RmsSource source(fixture.file);RmsIO io(source,0);source.failRead=at;PioneerQ38LearnedRmsSession<RmsIO,RmsSource> session(io,source);
  assert(!session.Validate(0,8191) && !session.Finish() && source.reads==at && io.calls==(at<=12?5U:6U) && !io.ends && io.closes==1);faults++;
 }
 for(unsigned at=1;at<=2;at++){
  RmsSource source(fixture.file);RmsIO io(source,0);source.failReference=at;PioneerQ38LearnedRmsSession<RmsIO,RmsSource> session(io,source);
  assert(!session.Validate(0,8191) && source.referenceReads==at && !source.gpuOpen && !io.opens && !io.calls);faults++;
 }
 printf("PASS DEBUG learned RMS source/session positives%u/faults%u;BOTH original references BEFORE FD,17actual original tiled pages,tag41/namespace12only,ALL10240returned words checked,first mismatch/source/epoch/transport CLOSE ONLY/no END/retry/substitution;SYNTHETIC_GGUF_TRANSPORT_NOT_CARD_ARITHMETIC\n",positives,faults);
}
