/* SPDX-License-Identifier: MIT */
// One explicit public Hello./four-token end-to-end candidate request.
// NO model/ggml/CPU inference API. Child must be the separately staged full
// card worker;the external physical parent authenticates BOTH binaries.
#include "../../src/apps/assistant/Qwen27Protocol.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <poll.h>
#include <time.h>
#include <cstdio>
namespace A=Assistant27;
#ifndef PIONEER_FULL_OFFLOAD_SMOKE_WORKER
#define PIONEER_FULL_OFFLOAD_SMOKE_WORKER "/boot/home/pioneer-q38-full-offload-20261104-a/HaikuAssistantQwen27FullOffload-all5"
#endif
static double Now(){timespec t={};return clock_gettime(CLOCK_MONOTONIC,&t)?-1:double(t.tv_sec)+double(t.tv_nsec)*1e-9;}
struct Channel {
 int fd=-1;pid_t child=-1;uint32_t id=0;A::Receiver receiver;
 bool Send(unsigned kind,unsigned value=0,const std::string& text=""){
  A::Frame frame;frame.kind=kind;frame.id=++id;frame.value=value;frame.text=text;return A::Send(fd,frame);
 }
 bool Next(A::Frame& frame,double deadline){
  for(;;){
   const double now=Now();if(now<0 || now>deadline)return false;
   const auto result=receiver.Read(fd,frame);
   if(result==A::Receiver::Received){const double received=Now();return frame.id==id && received>=0 && received<=deadline;}
   if(result!=A::Receiver::Waiting)return false;
   pollfd p={fd,POLLIN,0};if(poll(&p,1,100)<0 && errno!=EINTR)return false;
  }
 }
 bool Open(){
  int pair[2];if(socketpair(AF_UNIX,SOCK_STREAM,0,pair))return false;
  for(int item:pair)if(fcntl(item,F_SETFD,FD_CLOEXEC)){close(pair[0]);close(pair[1]);return false;}
  child=fork();
  if(!child){
   if(dup2(pair[1],0)<0 || dup2(pair[1],1)<0 || fcntl(0,F_SETFD,0)<0 || fcntl(1,F_SETFD,0)<0)_exit(126);
   for(int item:pair)if(item>2)close(item);
   char path[]=PIONEER_FULL_OFFLOAD_SMOKE_WORKER,arg[]="--assistant-channel";
   char* args[]={path,arg,nullptr};execv(path,args);_exit(127);
  }
  close(pair[1]);if(child<0){close(pair[0]);return false;}fd=pair[0];return true;
 }
 bool Exit(){
  const double deadline=Now()+5;
  while(Now()>=0 && Now()<=deadline){
   int status=0;const auto result=waitpid(child,&status,WNOHANG);
   if(result==child){child=-1;return WIFEXITED(status) && !WEXITSTATUS(status);}
   if(result<0 && errno!=EINTR)return false;
   poll(nullptr,0,10);
  }
  return false;
 }
 ~Channel(){if(fd>=0)close(fd);if(child>0){int status=0;waitpid(child,&status,WNOHANG);}}
}; // loss=close only;no signal/kill/unload/retry/reset/CPU fallback
static bool Progress(const A::Frame& frame){
 if(frame.kind!=A::Progress || (frame.value>1000 && frame.value!=UINT32_MAX))return false;
 printf("FULL_OFFLOAD_PROGRESS id%u value%u %s\n",frame.id,frame.value,frame.text.c_str());return true;
}
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--full-card-hello-four"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 Channel channel;if(!channel.Open())return 1;
 const double began=Now();if(began<0 || !channel.Send(A::Load,64))return 1;
 A::Frame frame;bool ready=false;
 while(channel.Next(frame,began+1800)){
  if(Progress(frame))continue;
  ready=frame.kind==A::Ready && !frame.value && frame.text=="QWEN27_FULL_OFFLOAD_VOCAB_READY";break;
 }
 if(!ready){puts("FAIL_FULL_OFFLOAD_HOST_READY;CLOSE_ONLY_NO_INFERENCE_FALLBACK");return 1;}
 puts("FULL_OFFLOAD_VOCABULARY_READY_NO_CPU_CONTEXT;GPU_UPLOAD_NOT_YET_STARTED");
 if(!channel.Send(A::Generate,4,"Hello."))return 1;
 const double generate=Now();bool complete=false;std::string answer,status;
 while(generate>=0 && channel.Next(frame,generate+7200)){
  if(Progress(frame))continue;
  if(frame.kind==A::Text && !frame.value){answer=frame.text;continue;}
  if(frame.kind==A::Done && frame.value==A::Complete){status=frame.text;complete=true;}
  break;
 }
 if(!complete){puts("FAIL_FULL_OFFLOAD_GENERATION;CLOSE_ONLY_NO_END_RETRY_RESET_OR_CPU_FALLBACK");return 1;}
 // Worker completes the actual device STOP/HOLD/drain/restore/END BEFORE
 // publishing its answer. No cleanup is fabricated from the text match.
 printf("FULL_OFFLOAD_DONE %s\n",status.c_str());
 const bool numericalText=answer=="Hello! How can";
 printf("FULL_OFFLOAD_PUBLIC_ANSWER bytes%zu text=%s\n",answer.size(),answer.c_str());
 if(!channel.Send(A::Unload) || !channel.Next(frame,Now()+120)
  || frame.kind!=A::Closed || frame.value || frame.text!="QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END" || !channel.Exit()){
  puts("FAIL_FULL_OFFLOAD_WORKER_CLEAN_CLOSE;NO_RETRY");return 1;
 }
 printf("%s_FULL_OFFLOAD_HELLO_FOUR clean_worker_exit1 expected_public_text%u;CPU_INFERENCE_CONTEXTS0;NOT_STEADY_TOKEN_RATE_OR_BROAD_MODEL_QUALITY\n",
  numericalText?"PASS":"FAIL",unsigned(numericalText));
 return numericalText?0:1;
}
