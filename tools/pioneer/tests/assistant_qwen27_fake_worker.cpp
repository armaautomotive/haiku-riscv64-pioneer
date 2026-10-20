/* SPDX-License-Identifier: MIT */
// Offline protocol peer. No model, driver header, device file or accelerator IO.
#include "../../../src/apps/assistant/Qwen27Protocol.h"
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <assert.h>
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
int main(int argc,char** argv) {
 assert(argc==2 && !strcmp(argv[1],"--assistant-channel"));
 const char* mode=getenv("ASSISTANT27_TEST_MODE"); assert(mode);
 A::Receiver r; A::Frame f; assert(Next(r,f) && f.kind==A::Load && f.value==4 && f.text.empty());
 Record("load\n");
 if(!strcmp(mode,"reject")) { Emit(A::Error,f.id,A::Rejected,"host load rejected"); return 1; }
 if(!strcmp(mode,"badready")) { Emit(A::Ready,f.id,0,"wrong ready"); return 70; }
 Emit(A::Progress,f.id,500,"host metadata"); Emit(A::Ready,f.id,0,"QWEN27_WORK_HOST_READY_497");
 while(Next(r,f)) {
  if(f.kind==A::Threads) {
   Record("threads\n"); assert(f.text.empty());
   if(!strcmp(mode,"threadfault")) { Emit(A::Ready,f.id,f.value+1,"QWEN27_WORK_THREADS_READY"); return 70; }
   if(f.value==8) Emit(A::Error,f.id,A::Rejected,"pool allocation rejected");
   else Emit(A::Ready,f.id,f.value,"QWEN27_WORK_THREADS_READY");
   continue;
  }
  if(f.kind==A::Unload) {
   Record("unload\n"); Emit(A::Closed,f.id,0,!strcmp(mode,"badclose")?"wrong close":"QWEN27_KNOWN_CLEAN_END"); return 0;
  }
  assert(f.kind==A::Generate && A::Question(f.text) && f.value>=1 && f.value<=32); Record("generate\n");
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
