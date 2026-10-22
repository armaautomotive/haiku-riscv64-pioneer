/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_EMBEDDING_SESSION_FIXTURE_ONLY
#include "pioneer_q38_embedding_session_test.cpp"
struct ValidationIO:EmbeddingIO {
 unsigned corruptWord=UINT32_MAX;bool nonfinite=false;
 using EmbeddingIO::EmbeddingIO;
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  bool okay=EmbeddingIO::Call(fd,op,data,bytes);
  if(op==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE && corruptWord<5120){
   auto& value=static_cast<tt_q38_embedding_validation*>(data)->values[corruptWord];
   value=nonfinite?0x7fc00000:(value^1);stopped=true;
  }return okay;
 }
};
static void Seed(Fixture& fixture,unsigned token){
 const PioneerQ38Tensor* tensor=nullptr;for(const auto& t:fixture.expected)if(t.name=="token_embd.weight")tensor=&t;
 assert(tensor);std::vector<uint8_t> row(2880);
 for(unsigned block=0;block<20;block++){
  auto* b=row.data()+block*144;b[1]=b[3]=0x3c;
  for(unsigned g=0;g<4;g++){b[4+g]=1;b[8+g]=2;b[12+g]=0x21;}
  unsigned shift=(token*7+token/16)%16;
  for(unsigned g=0;g<4;g++)for(unsigned c=0;c<32;c++)b[16+g*32+c]=uint8_t((c+block+shift)%16|(((c+block+shift+7)%16)<<4));
 }fixture.Write(tensor->offset+uint64_t(token)*2880,row);
}
int main(){
 Fixture fixture;unsigned positives=0,faults=0;
 for(unsigned token:{0U,15U,35519U,71039U,106495U,141951U,177407U,212863U,248319U}){
  Seed(fixture,token);EmbeddingSource source(fixture.file);ValidationIO io(source,token);
  PioneerQ38EmbeddingSession<ValidationIO,EmbeddingSource> session(io,source);
  assert(session.Produce(token,8191) && source.referenceReads==1 && source.gpuOpen && source.reads==12
   && session.Finish() && io.calls==9 && io.ends==1 && io.closes==1);positives++;
 }
 // Reference was prepared from original bytes before FD,NEVER from reply.
 for(unsigned word:{0U,31U,255U,1023U,1024U,4095U,5119U})for(bool nan:{false,true}){
  EmbeddingSource source(fixture.file);ValidationIO io(source,0);io.corruptWord=word;io.nonfinite=nan;
  PioneerQ38EmbeddingSession<ValidationIO,EmbeddingSource> session(io,source);
  assert(!session.Produce(0,8191) && !session.Finish() && source.referenceReads==1 && io.calls==8 && !io.ends && io.closes==1);
  assert(!session.Produce(0,8191) && io.calls==8);faults++;
 }
 for(unsigned category=0;category<4;category++)for(unsigned at=1;at<=9;at++){
  EmbeddingSource source(fixture.file);ValidationIO io(source,0);
  if(category==0)io.failCall=at;if(category==1)io.badCall=at;if(category==2)io.sourceDrop=at;if(category==3)io.epochDrop=at;
  PioneerQ38EmbeddingSession<ValidationIO,EmbeddingSource> session(io,source);
  assert(session.Produce(0,8191)==(at==9));assert(!session.Finish() && io.calls==at && io.closes==1 && io.ends==unsigned(at==9));faults++;
 }
 {EmbeddingSource source(fixture.file);ValidationIO io(source,0);source.referenceFail=true;
  PioneerQ38EmbeddingSession<ValidationIO,EmbeddingSource> session(io,source);
  assert(!session.Produce(0,0) && source.referenceReads==1 && !source.gpuOpen && !io.opens && !io.calls);faults++;}
 printf("PASS DEBUG embedding real source/session reference:positive%u/faults%u,original row decoded BEFORE FD/tag40only/12original tiled pages,ALL5120bits independent reference compare,finite+nonfinite corruption/source/epoch/transport CLOSE ONLY/no END/retry/substitution;SYNTHETIC_GGUF_TRANSPORT_NOT_CARD_ARITHMETIC\n",positives,faults);
}
