/* SPDX-License-Identifier: MIT */
// Offline protocol peer. No model, driver header, device file or accelerator IO.
#include "../../../src/apps/assistant/Qwen27Protocol.h"
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <vector>
namespace A=Assistant27;
static bool Next(A::Receiver& r,A::Frame& f) {
 for (;;) { auto n=r.Read(0,f); if(n==A::Receiver::Received)return true;
  if(n!=A::Receiver::Waiting)return false;
  pollfd p={0,POLLIN,0}; poll(&p,1,100); }
}
static void Record(const char* event) {
 const char* path=getenv("ASSISTANT27_TEST_LOG"); assert(path);
 int fd=open(path,O_WRONLY|O_APPEND); assert(fd>=0);
 assert(write(fd,event,strlen(event))==ssize_t(strlen(event))); close(fd);
}
static void Emit(unsigned kind,unsigned id,unsigned value,const char* text) {
 A::Frame f; f.kind=kind; f.id=id; f.value=value; f.text=text; assert(A::Send(1,f));
}
static void ClockBurst(unsigned id,const char* mode) {
 // Queue the entire finite trace in ONE write. This deliberately tests
 // deadlines even when Ready is already queued, not just recv Waiting.
 std::vector<uint8_t> bytes;
 auto append=[&](unsigned kind,unsigned value,const char* text) {
  const size_t begin=bytes.size(),length=strlen(text);bytes.resize(begin+24+length);
  uint8_t* h=bytes.data()+begin;A::Put(h,0x31413851);A::Put(h+4,1);A::Put(h+8,kind);
  A::Put(h+12,id);A::Put(h+16,value);A::Put(h+20,unsigned(length));memcpy(h+24,text,length);
 };
 const bool cache=!strncmp(mode,"clock-cache",11);
 if(!strcmp(mode,"clock-late-banner"))append(A::Progress,500,"host metadata");
 append(A::Progress,(cache && strcmp(mode,"clock-cache-value")) || !strcmp(mode,"clock-late-banner")?UINT32_MAX:500,
  !strcmp(mode,"clock-cache-text")?"Preparing/reusing lossless disk-cached weights before accelerator access. ":
  cache || !strcmp(mode,"clock-late-banner")?A::LosslessCachePreparation:"host metadata");
 if(!strcmp(mode,"clock-cache-repeat"))for(unsigned i=0;i<2;i++)append(A::Progress,UINT32_MAX,A::LosslessCachePreparation);
 append(A::Ready,0,"QWEN27_WORK_HOST_READY_497");
 assert(write(1,bytes.data(),bytes.size())==ssize_t(bytes.size()));
}
int main(int argc,char** argv) {
 assert(argc==2 && !strcmp(argv[1],"--assistant-channel"));
 const char* mode=getenv("ASSISTANT27_TEST_MODE"); assert(mode);
 A::Receiver r; A::Frame f; assert(Next(r,f) && f.kind==A::Load && f.value==4 && f.text.empty());
 Record("load\n");
 if(!strcmp(mode,"reject")) { Emit(A::Error,f.id,A::Rejected,"host load rejected"); return 1; }
 if(!strcmp(mode,"badready")) { Emit(A::Ready,f.id,0,"wrong ready"); return 70; }
 const bool clockMode=!strncmp(mode,"clock-",6);
 if(clockMode) { if(strcmp(mode,"clock-start-error"))ClockBurst(f.id,mode); }
 else {
 if(!strcmp(mode,"cachewarm") || !strcmp(mode,"cacheconverted") || !strcmp(mode,"cachemissing") || !strncmp(mode,"cacheinfer-",11)) {
  if(strcmp(mode,"cacheinfer-nobanner"))Emit(A::Progress,f.id,UINT32_MAX,A::LosslessCachePreparation);
  if(!strcmp(mode,"cacheconverted") || !strcmp(mode,"cacheinfer-converted")) Emit(A::Progress,f.id,UINT32_MAX,"Preparing lossless weight cache: tensor 1/851, 1.0/1.0 MiB; no GPU session yet.");
  for(unsigned i=1;i<=851;i++) {
   if(i==2 && (!strcmp(mode,"cachemissing") || !strcmp(mode,"cacheinfer-missing")))continue;
   char message[192];snprintf(message,sizeof(message),"Verifying lossless weight cache: tensor %u/851, 1.0/1.0 MiB; no GPU session yet.",i);
   Emit(A::Progress,f.id,UINT32_MAX,message);
  }
 }
 Emit(A::Progress,f.id,500,"host metadata"); Emit(A::Ready,f.id,0,"QWEN27_WORK_HOST_READY_497");
 }
 while(Next(r,f)) {
  if(f.kind==A::Threads) {
   Record("threads\n"); assert(f.text.empty());
   if(!strcmp(mode,"threadfault")) { Emit(A::Ready,f.id,f.value+1,"QWEN27_WORK_THREADS_READY"); return 70; }
   if(f.value==8) Emit(A::Error,f.id,A::Rejected,"pool allocation rejected");
   else Emit(A::Ready,f.id,f.value,"QWEN27_WORK_THREADS_READY");
   continue;
  }
  if(f.kind==A::Unload) {
   Record("unload\n"); Emit(A::Closed,f.id,0,(!strcmp(mode,"badclose") || !strcmp(mode,"cacheinfer-badclose"))?"wrong close":"QWEN27_KNOWN_CLEAN_END"); return 0;
  }
  assert(f.kind==A::Generate && A::Question(f.text) && f.value>=1 && f.value<=32); Record("generate\n");
  if(!strncmp(mode,"cacheinfer-",11)) {
   assert(f.text=="Hello." && f.value==4);
   if(!strcmp(mode,"cacheinfer-fatal")){Emit(A::Error,f.id,A::Fatal,"fake unproven execution");return 70;}
   if(!strcmp(mode,"cacheinfer-reject")){Emit(A::Done,f.id,A::Rejected,"fake rejected request");continue;}
   Emit(A::Text,f.id,0,!strcmp(mode,"cacheinfer-empty")?"":"four test tokens");
   Emit(A::Done,f.id,A::Complete,!strcmp(mode,"cacheinfer-badstats")?
    "P100A Qwen27B Work:3 output tokens,30.0s; warm generation2 samples/18.293s (0.109 tok/s). CPU attention/state; CPU validation OFF; weights retained; experimental.":
    !strcmp(mode,"cacheinfer-nan")?
    "P100A Qwen27B Work:4 output tokens,30.0s; warm generation3 samples/18.293s (nan tok/s). CPU attention/state; CPU validation OFF; weights retained; experimental.":
    !strcmp(mode,"cacheinfer-badrate")?
    "P100A Qwen27B Work:4 output tokens,30.0s; warm generation3 samples/18.293s (1000.000 tok/s). CPU attention/state; CPU validation OFF; weights retained; experimental.":
    !strcmp(mode,"cacheinfer-validation")?
    "P100A Qwen27B Work:4 output tokens,30.0s; warm generation3 samples/18.293s (0.164 tok/s). CPU attention/state; CPU validation ON; weights retained; experimental.":
    !strcmp(mode,"cacheinfer-trailer")?
    "P100A Qwen27B Work:4 output tokens,30.0s; warm generation3 samples/18.293s (0.164 tok/s). CPU attention/state; CPU validation OFF; weights retained; experimental. unexpected trailer":
    !strcmp(mode,"cacheinfer-fast")?
    "P100A Qwen27B Work:4 output tokens,0.4s; warm generation3 samples/0.150s (20.000 tok/s). CPU attention/state; CPU validation OFF; weights retained; experimental.":
    !strcmp(mode,"cacheinfer-rounded")?
    "P100A Qwen27B Work:4 output tokens,0.4s; warm generation3 samples/0.150s (19.935 tok/s). CPU attention/state; CPU validation OFF; weights retained; experimental.":
    "P100A Qwen27B Work:4 output tokens,30.0s; warm generation3 samples/18.293s (0.164 tok/s). CPU attention/state; CPU validation OFF; weights retained; experimental.");
   continue;
  }
  if(!strcmp(mode,"eof")) return 70;
  if(!strcmp(mode,"fatal")) { Emit(A::Error,f.id,A::Fatal,"unknown execution"); return 70; }
  if(!strcmp(mode,"wrongid")) { Emit(A::Text,f.id+1,0,"wrong request"); return 70; }
  if(!strcmp(mode,"malformed")) { uint8_t header[24]={}; assert(write(1,header,sizeof(header))==24); return 70; }
  if(!strcmp(mode,"cancel")) {
   unsigned id=f.id; assert(Next(r,f) && f.kind==A::Cancel && f.id==id && !f.value && f.text.empty());
   Record("cancel\n"); Emit(A::Done,id,A::Cancelled,"complete token boundary"); continue;
  }
  Emit(A::Progress,f.id,1000,"weights retained"); Emit(A::Text,f.id,0,"public answer"); Emit(A::Done,f.id,A::Complete,"known complete");
 }
 Record("close-only\n"); return 70;
}
