/* SPDX-License-Identifier: MIT */
#include "../../../src/apps/assistant/Qwen27Protocol.h"
#include "../../../src/apps/assistant/P100AQwen27.h"
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
namespace A=Assistant27;
static const char* const Model="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf";
static void Protocol() {
 assert(!A::Question("") && A::Question(std::string(256,'x')) && !A::Question(std::string(257,'x')));
 assert(!A::Question(std::string("a\0b",3)));
 assert(A::Prompt("PUBLIC")=="<|im_start|>user\nPUBLIC<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n");
 int s[2]; assert(!socketpair(AF_UNIX,SOCK_STREAM,0,s)); A::Receiver r; A::Frame f;
 uint8_t header[24]; A::Put(header,0x31413851); A::Put(header+4,1); A::Put(header+8,A::Text); A::Put(header+12,9); A::Put(header+16,0); A::Put(header+20,3);
 for(unsigned i=0;i<24;i++) { assert(write(s[0],header+i,1)==1); assert(r.Read(s[1],f)==A::Receiver::Waiting); }
 assert(write(s[0],"ab",2)==2 && r.Read(s[1],f)==A::Receiver::Waiting);
 assert(write(s[0],"c",1)==1 && r.Read(s[1],f)==A::Receiver::Received && f.id==9 && f.text=="abc");
 f.kind=A::Done; f.id=10; f.text=""; assert(A::Send(s[0],f) && r.Read(s[1],f)==A::Receiver::Received && f.id==10);
 close(s[1]); assert(!A::Send(s[0],f)); close(s[0]); // no SIGPIPE
 for(unsigned fault=0;fault<5;fault++) {
  assert(!socketpair(AF_UNIX,SOCK_STREAM,0,s)); A::Receiver bad;
  A::Put(header,0x31413851); A::Put(header+4,1); A::Put(header+12,9); A::Put(header+20,0);
  if(fault==0)A::Put(header,0);
  if(fault==1)A::Put(header+4,2);
  if(fault==2)A::Put(header+12,0);
  if(fault==3)A::Put(header+20,A::MaxAnswer+1);
  if(fault==4)A::Put(header+20,1);
  assert(write(s[0],header,24)==24); if(fault==4)assert(write(s[0],"",1)==1);
  assert(bad.Read(s[1],f)==A::Receiver::Invalid); close(s[0]); close(s[1]);
 }
}
static std::string Events(const char* path) {
 int fd=open(path,O_RDONLY); assert(fd>=0); char b[2048]; ssize_t n=read(fd,b,sizeof(b)); assert(n>=0 && n<ssize_t(sizeof(b))); close(fd); return std::string(b,n);
}
int main() {
 Protocol(); assert(AssistantP100AQwen27::Available());
 for(const char* mode:{"normal","cancel","reject","badready","eof","fatal","wrongid","malformed","badclose"}) {
  char log[]="/tmp/assistant27-test-XXXXXX"; int fd=mkstemp(log); assert(fd>=0); close(fd);
  assert(!setenv("ASSISTANT27_TEST_LOG",log,1) && !setenv("ASSISTANT27_TEST_MODE",mode,1));
  {
   AssistantP100AQwen27 backend; std::string error,answer; unsigned loads=0,updates=0,memory=0;
   auto load=[&](float p,const char*) { assert(p==.5f); loads++; };
   auto progress=[&](const std::string&,const char*) { updates++; };
   auto memoryProgress=[&](float p,const char*) { assert(p==1); memory++; };
   assert(!backend.Load("/tmp/not-allowlisted",4,error,load));
   assert(!backend.Load(Model,0,error,load) && !backend.Load(Model,65,error,load));
   assert(Events(log).empty()); error.clear();
   bool ready=backend.Load(Model,4,error,load);
   if(!strcmp(mode,"reject") || !strcmp(mode,"badready")) {
    assert(!ready && backend.Faulted()==(strcmp(mode,"reject")!=0));
    assert(!backend.Load(Model,4,error,load)); // never auto-retry this instance
   } else {
    assert(ready && loads==1);
    assert(!backend.Complete(std::string(257,'x'),1,answer,error,[] {return false;},progress,memoryProgress));
    assert(!backend.Complete("public",33,answer,error,[] {return false;},progress,memoryProgress)); error.clear();
    bool cancel=!strcmp(mode,"cancel");
    bool success=backend.Complete("Public test",2,answer,error,[&] {return cancel;},progress,memoryProgress);
    if(!strcmp(mode,"normal") || !strcmp(mode,"badclose")) {
     assert(success && answer=="public answer" && !backend.Faulted());
     assert(backend.Complete("Second public test",2,answer,error,[] {return false;},progress,memoryProgress));
     assert(memory==2 && updates>=4);
    } else if(cancel) { assert(!success && !backend.Faulted() && error=="complete token boundary"); }
    else {
     assert(!success && backend.Faulted());
     assert(!backend.Complete("No retry",1,answer,error,[] {return false;},progress,memoryProgress));
    }
    bool clean=backend.Unload(error);
    assert(clean==(!strcmp(mode,"normal") || cancel));
    if(!strcmp(mode,"badclose"))assert(backend.Faulted());
   }
  }
  std::string events=Events(log); assert(events.find("load\n")==0);
  if(!strcmp(mode,"normal") || !strcmp(mode,"badclose"))assert(events=="load\ngenerate\ngenerate\nunload\n");
  else if(!strcmp(mode,"cancel"))assert(events=="load\ngenerate\ncancel\nunload\n");
  else assert(events.find("unload")==std::string::npos);
  unlink(log);
 }
 puts("PASS: fragmented/bounded framing, closed-channel SIGPIPE safety, pinned-path/thread/question limits, isolated worker warm reuse, token-boundary cancel, known-clean unload, rejection and malformed/EOF/fatal/wrong-ID fail closed with no unload/retry/reset/fallback; NO DEVICE IO");
}
