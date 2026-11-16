/* SPDX-License-Identifier: MIT */
// Load-only diagnostic client. It can observe READY, then unload cleanly, but
// deliberately has no Generate send path; safe for metadata/readiness gates.
#include "../../src/apps/assistant/Qwen27Protocol.h"
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <cstdio>
namespace A=Assistant27;
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
   if(result==A::Receiver::Received)return frame.id==id && Now()<=deadline;
   if(result!=A::Receiver::Waiting)return false;
   pollfd p={fd,POLLIN,0};if(poll(&p,1,100)<0 && errno!=EINTR)return false;
  }
 }
 bool Open(const char* worker){
  int pair[2];if(socketpair(AF_UNIX,SOCK_STREAM,0,pair))return false;
  for(int item:pair)if(fcntl(item,F_SETFD,FD_CLOEXEC)){close(pair[0]);close(pair[1]);return false;}
  child=fork();
  if(!child){
   if(dup2(pair[1],0)<0 || dup2(pair[1],1)<0 || fcntl(0,F_SETFD,0)<0 || fcntl(1,F_SETFD,0)<0)_exit(126);
   for(int item:pair)if(item>2)close(item);
   char arg[]="--assistant-channel";char* args[]={const_cast<char*>(worker),arg,nullptr};execv(worker,args);_exit(127);
  }
  close(pair[1]);if(child<0){close(pair[0]);return false;}fd=pair[0];return true;
 }
 bool Exit(double seconds){
  const double deadline=Now()+seconds;
  while(Now()>=0 && Now()<=deadline){
   int status=0;const auto result=waitpid(child,&status,WNOHANG);
   if(result==child){child=-1;return WIFEXITED(status) && !WEXITSTATUS(status);}
   if(result<0 && errno!=EINTR)return false;
   poll(nullptr,0,10);
  }
  return false;
 }
 ~Channel(){if(fd>=0)close(fd);if(child>0){int status=0;waitpid(child,&status,WNOHANG);}}
};
static bool Progress(const A::Frame& frame){
 if(frame.kind!=A::Progress || (frame.value>1000 && frame.value!=UINT32_MAX))return false;
 printf("READY_ONLY_PROGRESS value%u %s\n",frame.value,frame.text.c_str());return true;
}
int main(int argc,char** argv){
 if(argc!=2 || argv[1][0]!='/')return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 Channel channel;if(!channel.Open(argv[1]))return 1;
 const double began=Now();if(began<0 || !channel.Send(A::Load,64))return 1;
 A::Frame frame;
 while(channel.Next(frame,began+1800)){
  if(Progress(frame))continue;
  if(frame.kind==A::Ready && !frame.value && frame.text=="QWEN27_FULL_OFFLOAD_VOCAB_READY"){
   const double ready=Now()-began;
   if(!channel.Send(A::Unload) || !channel.Next(frame,Now()+120) || frame.kind!=A::Closed || frame.value
    || frame.text!="QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END" || !channel.Exit(5)){
    puts("FAIL_READY_ONLY_CLEAN_UNLOAD;NO_GENERATE_WAS_SENT");return 1;
   }
   printf("PASS_READY_ONLY seconds=%.3f CLEAN_UNLOAD NO_GENERATE_SENT\n",ready);return 0;
  }
  if(frame.kind==A::Done || frame.kind==A::Closed || frame.kind==A::Error){
   printf("FAIL_READY_ONLY_TERMINAL kind%u value%u %s;NO_GENERATE_WAS_SENT\n",frame.kind,frame.value,frame.text.c_str());
   if(!channel.Exit(120))return 1;
   return 1;
  }
  puts("FAIL_READY_ONLY_UNEXPECTED_FRAME;NO_GENERATE_WAS_SENT");return 1;
 }
 puts("FAIL_READY_ONLY_TIMEOUT;CHANNEL_CLOSE_ONLY;NO_GENERATE_WAS_SENT");return 1;
}
