/* SPDX-License-Identifier: MIT */
// Real uploader and full client over synthetic pages and device ONLY.
#define PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT 1
#define PIONEER_Q38_HYBRID_FULL_CLIENT 1
#define main tracked_upload_fixture_main
#include "pioneer_q38_hybrid_tracked_session_test.cpp"
#undef main
#include "../PioneerQ38HybridFullSession.h"

struct FullIO:TrackedIO {
 unsigned support=0,starts=0,polls=0,cancels=0,holds=0,reads=0,ends=0,cursor=0,prompts=0,limit=0,controls=0;
 unsigned failAction=0,badAction=0;bool badSupport=false,badRecord=false;uint32_t ids[96]={};
 FullIO(const Canonical& c,FakePages& p):TrackedIO(c,p){}
 bool Call(int fd,unsigned op,void* data,size_t size){
  namespace F=PioneerQ38FullOffload;namespace P=PioneerQ38TokenControl;
  if(op==TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT){
   assert(fd==42 && !starts && support<2 && (support==0?!owned:owned && seal==1) && size==sizeof(F::Support));support++;
   *static_cast<F::Support*>(data)={55,1,96,32,128,248320,F::AllCompute,0};
   if(badSupport)static_cast<F::Support*>(data)->compute=0;
   return true;
  }
  if(op!=TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL)return TrackedIO::Call(fd,op,data,size);
  assert(fd==42 && owned && seal==1 && support==2 && !closes && size==sizeof(P::Packet));
  auto& p=*static_cast<P::Packet*>(data);assert(F::RequestShape(p));controls++;
  const unsigned action=p.request.operation;auto& r=p.reply;
  r.version=1;r.operation=action;r.started=r.ready=1;auto* d=r.detail;d[0]=d[1]=1;
  if(action==P::Start){
   assert(!starts++);prompts=p.request.prompts;limit=p.request.limit;
   std::copy(p.prompt,p.prompt+prompts,ids);r.stage=d[2]=8;d[3]=1;
  }else if(action==P::Observe){
   assert(starts==1 && !holds);polls++;
   if(polls==1){r.ready=0;r.stage=d[2]=9;}
   else{r.stage=d[2]=11;d[6]=1;d[7]=cancels!=0;d[8]=cancels?0:limit;
    d[9]=cancels?0:limit-1;d[10]=cancels?1:64;d[11]=cancels?1:0;}
  }else if(action==P::Cancel){
   assert(starts==1 && !cancels++ && !holds);r.stage=d[2]=7;d[9]=1;
  }else if(action==P::Hold){
   assert(polls==2 && !holds++ && !reads);r.stage=d[2]=4;d[6]=1;d[7]=120;
  }else if(action==P::Read){
   const unsigned total=cancels?0:limit,count=std::min(64U,total-cursor);
   assert(holds==1 && p.request.records==count && !ends);reads++;
   r.stage=d[2]=4;r.records=count;d[6]=1;d[7]=cursor;d[8]=count;d[9]=count!=0;d[10]=count*256;d[11]=cursor+count==total;
   for(unsigned n=0;n<count;n++){
    const unsigned pos=cursor+n;auto* v=p.records+n*64;v[0]=0x31435451;v[1]=1;v[2]=pos;
    v[3]=pos<prompts?ids[pos]:1000+pos-1;v[4]=1000+pos;v[5]=0x3f800000;v[6]=pos+1;v[7]=(pos+1)&1;
    v[8]=pos+1<prompts;v[9]=pos+1!=limit;v[10]=64;v[11]=973;
    for(unsigned i=12;i<36;i++)v[i]=100+i;
    v[44]=pos+1;v[45]=48;v[46]=119;
   }
   if(badRecord && count)p.records[4]=F::Vocabulary;
   cursor+=count;
  }else if(action==P::Finish){
   assert(cursor==(cancels?0:limit) && reads && !ends++);r.stage=d[2]=11;r.complete=1;d[3]=d[5]=1;owned=false;
  }else assert(!"unexpected control");
  if(action==badAction)r.unsafe=1;
  if(action==failAction)return false;
  return true;
 }
};
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--long") || !strcmp(argv[1],"--cancel") || !strcmp(argv[1],"--bad-record")
  || !strcmp(argv[1],"--bad-support") || !strcmp(argv[1],"--source-lost")))
  || (argc==3 && (!strcmp(argv[1],"--fail") || !strcmp(argv[1],"--bad"))));
 auto canonical=std::make_unique<Canonical>();assert(canonical->Build());
 FakePages pages(*canonical);FullIO io(*canonical,pages);
 const bool longRun=argc==2 && !strcmp(argv[1],"--long");
 const bool cancel=argc==2 && !strcmp(argv[1],"--cancel");
 const bool lost=argc==2 && !strcmp(argv[1],"--source-lost");
 io.badRecord=argc==2 && !strcmp(argv[1],"--bad-record");
 io.badSupport=argc==2 && !strcmp(argv[1],"--bad-support");
 if(argc==3){unsigned action=unsigned(atoi(argv[2]));assert(action>=1 && action<=6);
  if(!strcmp(argv[1],"--fail"))io.failAction=action;else io.badAction=action;}
 auto session=std::make_unique<PioneerQ38HybridFullSession<FakePages,FullIO>>(pages,io);
 uint32_t input[96];for(unsigned i=0;i<96;i++)input[i]=17+i;
 uint32_t answer[32]={};unsigned count=999;const unsigned inputs=longRun?96:3,outputs=longRun?32:2;
 assert(!session->Poll() && !session->Finish() && !session->CopyGeneratedTokens(answer,32,count));
 bool ok=session->Begin(*canonical,input,inputs,outputs);
 if(ok && lost)pages.live=false;
 if(ok && (cancel || io.failAction==3 || io.badAction==3))ok=session->Cancel();
 if(ok)ok=session->Poll();
 if(ok)ok=session->Poll();
 if(ok){assert(session->TerminalReady());ok=session->Finish();}
 if(argc==1 || longRun || cancel){
  assert(ok && session->Completed() && !session->FailedState() && !io.owned && io.closes==1 && !io.bankEnd);
  assert(io.opens==1 && io.starts==1 && io.polls==2 && io.holds==1 && io.reads==unsigned(longRun?2:1) && io.ends==1);
  assert(session->CopyGeneratedTokens(answer,32,count) && count==(cancel?0:outputs));
  if(!cancel)assert(answer[0]==1000+inputs-1 && answer[count-1]==1000+inputs+outputs-2);
  pages.live=false;assert(!session->CopyGeneratedTokens(answer,32,count));pages.live=true;
  assert(!session->Finish() && !session->Begin(*canonical,input,3,2));
  puts("PASS synthetic full hybrid same FD: upload, seal, START, poll, HOLD, READ, FINISH, close; NO DEVICE");
 }else{
  assert(!ok && session->FailedState() && io.closes==1 && !io.bankEnd);
  assert(!session->CopyGeneratedTokens(answer,32,count) && count==999);
  const auto controls=io.controls;assert(!session->Poll() && !session->Finish() && !session->Cancel());
  assert(io.controls==controls && io.opens==1);
  if(io.badSupport)assert(!io.owned && !io.retained && !io.begin);
  else if(!io.ends)assert(io.owned && io.retained);
  puts("PASS uncertain full hybrid control is CLOSE ONLY; NO DEVICE");
 }
}
