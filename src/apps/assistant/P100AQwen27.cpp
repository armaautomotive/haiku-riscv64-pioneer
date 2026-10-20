/* SPDX-License-Identifier: MIT */
#include "P100AQwen27.h"
#include "Qwen27Protocol.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <poll.h>
#include <time.h>
namespace {
#ifndef ASSISTANT_QWEN27_WORKER
const char* const Worker="/boot/home/config/non-packaged/servers/HaikuAssistantQwen27";
#else
const char* const Worker=ASSISTANT_QWEN27_WORKER; // compile-time offline test ONLY
#endif
double Now() { timespec t={}; return clock_gettime(CLOCK_MONOTONIC,&t)?-1:double(t.tv_sec)+double(t.tv_nsec)*1e-9; }
}
struct AssistantP100AQwen27::Data {
 int fd=-1; pid_t child=-1; uint32_t id=0; Assistant27::Receiver receiver; bool fault=false;
 bool Send(unsigned kind,unsigned value=0,const std::string& text="") {
  if (fd<0 || id==UINT32_MAX) return false;
  Assistant27::Frame f; f.kind=kind; f.id=++id; f.value=value; f.text=text; return Assistant27::Send(fd,f);
 }
 void Lose() { fault=true; if (fd>=0) close(fd); fd=-1; } // NO signal/kill/retry/device END
 bool Next(Assistant27::Frame& f,double deadline) {
  for (;;) {
   auto r=receiver.Read(fd,f);
   if (r==Assistant27::Receiver::Received) return f.id==id;
   if (r!=Assistant27::Receiver::Waiting || Now()<0 || Now()>deadline) return false;
   pollfd p={fd,POLLIN,0}; int n=poll(&p,1,100); if (n<0 && errno!=EINTR) return false;
  }
 }
 bool Exit(int& status) {
  if (child<=0) return false;
  double deadline=Now()+5;
  for (;;) {
   pid_t p=waitpid(child,&status,WNOHANG);
   if (p==child) { child=-1; return true; }
   if ((p<0 && errno!=EINTR) || Now()<0 || Now()>deadline) return false;
   poll(NULL,0,10); // process bookkeeping ONLY; never signal/kill a worker
  }
 }
 ~Data() { if (fd>=0) close(fd); if (child>0) { int status; waitpid(child,&status,WNOHANG); } }
};
AssistantP100AQwen27::AssistantP100AQwen27():fData(NULL),fFaulted(false) {}
AssistantP100AQwen27::~AssistantP100AQwen27() { std::string error; Unload(error); delete fData; }
bool AssistantP100AQwen27::Available() { return access(Worker,X_OK)==0; }
bool AssistantP100AQwen27::Load(const char* path,unsigned threads,std::string& error,const std::function<void(float,const char*)>& progress) {
 if (fData || fFaulted || !path || strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") || !threads || threads>64 || !Available()) {
  error="Pinned Qwen27B worker/model is unavailable. No CPU fallback."; return false;
 }
 int pair[2]; if (socketpair(AF_UNIX,SOCK_STREAM,0,pair)) { error="Cannot create private Qwen27B channel."; return false; }
 for (int fd:pair) fcntl(fd,F_SETFD,FD_CLOEXEC);
 pid_t child=fork();
 if (!child) {
  if (dup2(pair[1],0)<0 || dup2(pair[1],1)<0) _exit(126);
  if (fcntl(0,F_SETFD,0)<0 || fcntl(1,F_SETFD,0)<0) _exit(126);
  for (int fd:pair) if (fd>2) close(fd);
  char arg[]="--assistant-channel"; char* args[]={const_cast<char*>(Worker),arg,NULL}; execv(Worker,args); _exit(127);
 }
 close(pair[1]); if (child<0) { close(pair[0]); error="Cannot start isolated Qwen27B worker."; return false; }
 fData=new Data; fData->child=child; fData->fd=pair[0];
 if (!fData->Send(Assistant27::Load,threads)) { fData->Lose(); fFaulted=true; error="Qwen27B worker channel failed."; return false; }
 Assistant27::Frame f; double deadline=Now()+1800;
 while (fData->Next(f,deadline)) {
  if (f.kind==Assistant27::Progress && (f.value<=1000 || f.value==UINT32_MAX)) { progress(f.value==UINT32_MAX?-1:float(f.value)/1000,f.text.c_str()); continue; }
  if (f.kind==Assistant27::Ready && !f.value && f.text=="QWEN27_WORK_HOST_READY_497") return true;
  if (f.kind==Assistant27::Error && f.value==Assistant27::Rejected) {
   error=f.text; close(fData->fd); fData->fd=-1; int status=0;
   if (!fData->Exit(status) || !WIFEXITED(status) || WEXITSTATUS(status)!=1) fFaulted=true;
   return false;
  }
  break;
 }
 fData->Lose(); fFaulted=true; error="Qwen27B load/channel failed; no retry or CPU fallback."; return false;
}
bool AssistantP100AQwen27::Complete(const std::string& question,int tokens,std::string& answer,std::string& error,
 const std::function<bool()>& cancelled,const std::function<void(const std::string&,const char*)>& progress,
 const std::function<void(float,const char*)>& memoryProgress) {
 if (!fData || fData->fd<0 || fFaulted || !Assistant27::Question(question) || tokens<1 || tokens>Assistant27::MaxTokens) {
  error="Qwen27B Work: one question, at most256 bytes and1-32 output tokens; no fallback."; return false;
 }
 if (!fData->Send(Assistant27::Generate,unsigned(tokens),question)) { fData->Lose(); fFaulted=true; error="Qwen27B request channel failed."; return false; }
 bool cancelSent=false; double deadline=Now()+7200;
 for (;;) {
  if (!cancelSent && cancelled()) {
   Assistant27::Frame stop; stop.kind=Assistant27::Cancel; stop.id=fData->id;
   if (!Assistant27::Send(fData->fd,stop)) break;
   cancelSent=true;
  }
  Assistant27::Frame f; auto r=fData->receiver.Read(fData->fd,f);
  if (r==Assistant27::Receiver::Waiting) {
   if (Now()<0 || Now()>deadline) break;
   pollfd p={fData->fd,POLLIN,0}; if (poll(&p,1,100)<0 && errno!=EINTR) break; continue;
  }
  if (r!=Assistant27::Receiver::Received || f.id!=fData->id) break;
  if (f.kind==Assistant27::Progress && (f.value<=1000 || f.value==UINT32_MAX)) {
   memoryProgress(f.value==UINT32_MAX?-1:float(f.value)/1000,f.text.c_str()); progress(answer,f.text.c_str()); continue;
  }
  if (f.kind==Assistant27::Text && !f.value && f.text.size()<=Assistant27::MaxAnswer) { answer=f.text; progress(answer,"Qwen27B Work — P100A projections; CPU attention/state; validation OFF."); continue; }
  if (f.kind==Assistant27::Done && f.value<=Assistant27::Rejected) {
   if (f.value) error=f.text; else progress(answer,f.text.c_str());
   return !f.value;
  }
  if (f.kind==Assistant27::Error) { error=f.text; break; }
  break;
 }
 fData->Lose(); fFaulted=true;
 if (error.empty()) error="Qwen27B worker/execution failed; backend disabled. No retry/reset/CPU fallback.";
 return false;
}
bool AssistantP100AQwen27::Unload(std::string& error) {
 if (!fData || fData->fd<0) return !fFaulted;
 if (fFaulted || !fData->Send(Assistant27::Unload)) { fData->Lose(); fFaulted=true; error="Qwen27B unload channel failed; no retry."; return false; }
 Assistant27::Frame f; bool clean=fData->Next(f,Now()+120) && f.kind==Assistant27::Closed && !f.value && f.text=="QWEN27_KNOWN_CLEAN_END";
 if (!clean) { fData->Lose(); fFaulted=true; error="Qwen27B clean END not proven; backend disabled, no retry/reset."; return false; }
 close(fData->fd); fData->fd=-1;
 int status=0; clean=fData->Exit(status) && WIFEXITED(status) && !WEXITSTATUS(status);
 if (!clean) { fFaulted=true; error="Qwen27B worker did not exit cleanly."; }
 return clean;
}
