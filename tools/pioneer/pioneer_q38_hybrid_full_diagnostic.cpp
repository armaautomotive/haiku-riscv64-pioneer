/* SPDX-License-Identifier: MIT */
// Explicit one-shot candidate test. Token IDs only; no CPU model or fallback.
#if !defined(PIONEER_Q38_HYBRID_FULL_CLIENT) || PIONEER_Q38_HYBRID_FULL_CLIENT != 1 \
 || !defined(PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC) || PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC != 1
#error Hybrid full candidate diagnostic needs its explicit build roles
#endif
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "PioneerQ38HybridVerifiedModel.h"
#include "PioneerQ38HybridFullSession.h"

struct Transport {
 int Open(){return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,unsigned op,void* data,size_t size){return ioctl(fd,op,data,size)==0;}
 int Close(int fd){return close(fd);}
};
static uint64_t Now(){
 timespec t={};if(clock_gettime(CLOCK_MONOTONIC,&t) || t.tv_sec<0 || t.tv_nsec<0 || t.tv_nsec>=1000000000)return 0;
 return uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec);
}
static bool Number(const char* text,unsigned max,unsigned& value){
 if(!text || *text<'0' || *text>'9')return false;
 char* end=nullptr;errno=0;const unsigned long result=strtoul(text,&end,10);
 if(errno || !end || *end || result>max)return false;
 value=unsigned(result);return true;
}
template<class Session> static void ReportFailure(const Session& session){
 typename Session::ControlObservation observed;
 if(!session.FailureObservation(observed))return;
 const auto& r=observed.reply;
 fprintf(stderr,"HYBRID_CONTROL_OBSERVATION action=%u call_passed=%u request_unchanged=%u "
  "version=%u operation=%u started=%u ready=%u unsafe=%u complete=%u stage=%u records=%u status=%ld detail=",
  observed.action,observed.callPassed,observed.requestUnchanged,r.version,r.operation,r.started,r.ready,r.unsafe,
  r.complete,r.stage,r.records,(long)r.status);
 for(unsigned i=0;i<16;i++)fprintf(stderr,"%s%u",i?",":"",r.detail[i]);
 fputs("; UNTRUSTED_DIAGNOSTIC_ONLY_NO_RETRY_OR_CLEANUP_AUTHORITY\n",stderr);
}
int main(int argc,char** argv){
 namespace F=PioneerQ38FullOffload;
 if(argc<6 || argc>5+F::MaxPrompt || strcmp(argv[1],"--run-hybrid-candidate") || argv[2][0]!='/' || argv[3][0]!='/'){
  fputs("Use --run-hybrid-candidate ABSOLUTE_BFP4_CACHE ABSOLUTE_BFP8_CACHE OUTPUT_COUNT TOKEN_ID...\n",stderr);return 2;
 }
 unsigned outputs=0,ids[F::MaxPrompt]={},prompts=unsigned(argc-5);
 if(!Number(argv[4],F::MaxOutput,outputs) || !outputs || prompts+outputs-1>F::MaxPositions)return 2;
 for(unsigned i=0;i<prompts;i++)if(!Number(argv[5+i],F::Vocabulary-1,ids[i]))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet");
 auto model=std::make_unique<PioneerQ38HybridVerifiedModel>();std::string error;
 if(!model->Prepare(argv[2],argv[3],error,[](unsigned done,unsigned total){
  if(done%21==0 || done==total)printf("HYBRID_PREFLIGHT %u/%u\n",done,total);
 }) || !model->Ready()){
  fprintf(stderr,"HYBRID_PREFLIGHT_REFUSED %s; no device open\n",error.c_str());return 1;
 }
 auto* pages=model->PreparedPages();const auto* canonical=model->Canonical();if(!pages || !canonical)return 1;
 Transport io;auto session=std::make_unique<PioneerQ38HybridFullSession<PioneerQ38HybridVerifiedModel::Pages,Transport>>(*pages,io);
 const uint64_t begin=Now();if(!begin)return 1;uint64_t next=UINT64_C(256)*1024*1024;
 if(!session->Begin(*canonical,ids,prompts,outputs,[&](uint64_t done,uint64_t total){
  if(done>=next || done==total){printf("HYBRID_UPLOAD %llu/%llu\n",(unsigned long long)done,(unsigned long long)total);
   while(next<=done)next+=UINT64_C(256)*1024*1024;}
 })){
  fprintf(stderr,"HYBRID_START_FAILED %s; close only\n",session->failure.c_str());ReportFailure(*session);return 1;
 }
 const uint64_t started=Now();if(!started || started<begin){session->Abort();return 1;}
 printf("HYBRID_STARTED upload_and_start_seconds=%.3f\n",double(started-begin)*1e-9);
 while(!session->TerminalReady()){
  if(!session->Poll()){fprintf(stderr,"HYBRID_OBSERVE_FAILED %s; close only\n",session->failure.c_str());ReportFailure(*session);return 1;}
  if(!session->TerminalReady()){
   const uint64_t now=Now();if(!now || now<started || now-started>UINT64_C(7200)*1000000000){
    session->Abort();fputs("HYBRID_TIMEOUT close only; recovery may be required\n",stderr);return 1;
   }
   usleep(100000);
  }
 }
 const uint64_t terminal=Now();
 if(!terminal || terminal<started){session->Abort();return 1;}
 if(!session->Finish()){fprintf(stderr,"HYBRID_FINISH_FAILED %s; close only\n",session->failure.c_str());ReportFailure(*session);return 1;}
 unsigned tokens[F::MaxOutput]={},count=0;
 if(!session->CopyGeneratedTokens(tokens,F::MaxOutput,count))return 1;
 printf("HYBRID_RESULT prompt_tokens=%u generated_tokens=%u post_start_seconds=%.3f (includes prompt processing)\n",
  prompts,count,double(terminal-started)*1e-9);
 for(unsigned i=0;i<count;i++)printf("HYBRID_TOKEN %u\n",tokens[i]);
 puts("HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated");
 return fflush(stdout)?1:0;
}
