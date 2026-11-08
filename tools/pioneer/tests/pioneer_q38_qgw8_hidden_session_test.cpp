/* SPDX-License-Identifier: MIT */
// Actual tag129 USER session and packet handling over a synthetic transport.
#define PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION 1
#define PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128 1
#define PIONEER_Q38_QGW8_HIDDEN_CLIENT_129 1
#define main qgw8_upload_fixture_main
#include "pioneer_q38_qgw8_upload_session_test.cpp"
#undef main
#include "../PioneerQ38HybridFullSession.h"

struct HiddenIO:SimOwner {
 unsigned support=0,starts=0,polls=0,holds=0,reads=0,hiddenCalls=0,finishes=0;
 bool failFirst=false,failSecond=false,badReply=false,changedRequest=false,failFinish=false;
 explicit HiddenIO(Fixture& f):SimOwner(f){tag=PioneerQ38Qgw8Hidden::BuildTag;}
 bool Call(int fd,unsigned op,void* data,size_t size){
  namespace F=PioneerQ38FullOffload;namespace C=PioneerQ38TokenControl;namespace H=PioneerQ38Qgw8Hidden;
  if(op==TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT){
   assert(fd==42 && support<2 && (support==0?!owned:owned && seals==1 && receipt->Sealed()) && size==sizeof(F::Support));
   support++;*static_cast<F::Support*>(data)={55,1,1,1,1,248320,F::AllCompute,0};return true;
  }
  if(op==TT_PCI_PROBE_Q38_QGW8_HIDDEN_READ){
   assert(fd==42 && owned && reads==1 && !finishes && size==sizeof(H::Packet));
   auto& p=*static_cast<H::Packet*>(data);assert(H::RequestShape(p) && p.request.part==hiddenCalls);
   const unsigned part=hiddenCalls++;
   if((part==0 && failFirst) || (part==1 && failSecond))return false;
   auto& r=p.reply;r.version=H::Version;r.part=part;r.started=r.ready=1;r.words=p.request.words;
   const uint32_t d[]={1,1,4,0,0,0,1,part,part*H::ChunkWords,p.request.words,0,64,1,1,1,p.request.words*4};
   std::copy(d,d+16,r.detail);for(unsigned i=0;i<p.request.words;i++)p.words[i]=part*H::ChunkWords+i+1;
   if(badReply && part==1)r.detail[11]--;
   if(changedRequest && part==1)p.request.reserved[0]=1;
   return true;
  }
  if(op!=TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL)return SimOwner::Call(fd,op,data,size);
  assert(fd==42 && owned && seals==1 && receipt->Sealed() && support==2 && size==sizeof(C::Packet));
  auto& p=*static_cast<C::Packet*>(data);assert(F::RequestShape(p) && H::ControlRequestShape(p));
  auto& r=p.reply;const unsigned action=p.request.operation;r.version=1;r.operation=action;r.started=r.ready=1;
  auto* d=r.detail;d[0]=d[1]=1;
  if(action==C::Start){
   assert(!starts++ && p.request.prompts==1 && p.request.limit==1 && p.prompt[0]==H::Input);
   r.stage=d[2]=8;d[3]=1;
  }else if(action==C::Observe){
   assert(starts==1 && !holds);polls++;
   if(polls==1){r.ready=0;r.stage=d[2]=9;}
   else{r.stage=d[2]=11;d[6]=1;d[8]=1;d[9]=0;d[10]=64;}
  }else if(action==C::Hold){
   assert(polls==2 && !holds++);r.stage=d[2]=4;d[6]=1;d[7]=120;
  }else if(action==C::Read){
   assert(holds==1 && !reads++ && p.request.records==1);r.stage=d[2]=4;r.records=1;
   d[6]=1;d[8]=1;d[9]=1;d[10]=256;d[11]=1;
   auto* v=p.records;v[0]=0x31435451;v[1]=1;v[2]=0;v[3]=H::Input;v[4]=108133;v[5]=0x3f800000;
   v[6]=1;v[7]=1;v[8]=v[9]=0;v[10]=64;v[11]=973;
   for(unsigned i=12;i<36;i++)v[i]=100+i;
   v[44]=1;v[45]=48;v[46]=119;
  }else if(action==C::Finish){
   assert(hiddenCalls==2 && !finishes++);r.stage=d[2]=11;r.complete=1;d[3]=d[5]=1;
   if(failFinish)return false;owned=false;
  }else assert(!"unexpected tag129 lifecycle operation");
  return true;
 }
};
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--fail-first") || !strcmp(argv[1],"--fail-second")
  || !strcmp(argv[1],"--bad-reply") || !strcmp(argv[1],"--changed-request")
  || !strcmp(argv[1],"--fail-finish") || !strcmp(argv[1],"--wrong-tag") || !strcmp(argv[1],"--wrong-input"))));
 static_assert(PioneerQ38FullOffload::BuildTag==PioneerQ38Qgw8Hidden::BuildTag,"exact separate debug tag");
 Fixture fixture;auto io=std::make_unique<HiddenIO>(fixture);
 const char* mode=argc==2?argv[1]:"";
 io->failFirst=!strcmp(mode,"--fail-first");io->failSecond=!strcmp(mode,"--fail-second");
 io->badReply=!strcmp(mode,"--bad-reply");io->changedRequest=!strcmp(mode,"--changed-request");
 io->failFinish=!strcmp(mode,"--fail-finish");if(!strcmp(mode,"--wrong-tag"))io->tag=UINT64_C(0x2026110700010128);
 PioneerQ38HybridFullSession<HeadPages,HiddenIO> session(fixture.pages,*io);
 const uint32_t input[]={PioneerQ38Qgw8Hidden::Input-unsigned(!strcmp(mode,"--wrong-input"))};auto snapshot=std::make_unique<PioneerQ38TokenHiddenSnapshot>();
 assert(!session.CopyHiddenSnapshot(snapshot.get(),sizeof(*snapshot)));
 bool okay=session.Begin(*fixture.canonical,input,1,1);
 if(!strcmp(mode,"--wrong-input")){
  assert(!okay && !io->opens && !io->calls && !io->closes && !io->owned);
  puts("PASS wrong debug input refused before open; synthetic only");return 0;
 }
 if(okay)assert(!session.Cancel() && io->starts==1 && !io->polls);
 if(okay)okay=session.Poll();if(okay)okay=session.Poll();if(okay)okay=session.Finish();
 if(argc==1){
  assert(okay && session.Completed() && io->hiddenCalls==2 && io->finishes==1 && !io->owned && !io->retained
   && io->closes==1 && !io->bankEnd && session.CopyHiddenSnapshot(snapshot.get(),sizeof(*snapshot)));
  assert(snapshot->header[2]==0 && snapshot->header[3]==PioneerQ38Qgw8Hidden::Input && snapshot->header[4]==64
   && snapshot->header[6]==1 && snapshot->header[7]==1 && snapshot->header[8]==uint32_t(PioneerQ38Qgw8Hidden::BuildTag)
   && snapshot->header[9]==uint32_t(PioneerQ38Qgw8Hidden::BuildTag>>32) && snapshot->header[10]==20 && snapshot->header[12]==2
   && snapshot->words[0]==1 && snapshot->words[4096]==4097 && snapshot->words[5119]==5120
   && snapshot->footer==PioneerQ38TokenHiddenSnapshotData::Footer);
  assert(!session.CopyHiddenSnapshot(snapshot.get(),sizeof(*snapshot)-1));
  puts("PASS tag129 fixed hidden DATA after hold/drain/two ordered reads/finish/close; synthetic only");
 }else{
  assert(!okay && session.FailedState() && io->closes==1 && !session.CopyHiddenSnapshot(snapshot.get(),sizeof(*snapshot)));
  if(!strcmp(mode,"--wrong-tag"))assert(!io->owned && !io->begins && !io->hiddenCalls);
  else assert(io->owned && io->retained && io->banks.Quarantined() && !io->bankEnd);
  puts("PASS tag129 diagnostic failure refuses output and does not retry/END; synthetic only");
 }
}
