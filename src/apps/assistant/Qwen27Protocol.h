/* SPDX-License-Identifier: MIT */
#ifndef ASSISTANT_QWEN27_PROTOCOL_H
#define ASSISTANT_QWEN27_PROTOCOL_H
#include <stdint.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <errno.h>
namespace Assistant27 {
enum { Load=1,Generate=2,Cancel=3,Unload=4,Threads=5,Progress=10,Ready=11,Text=12,Done=13,Error=14,Closed=15 };
enum { Complete=0,Cancelled=1,Rejected=2,Fatal=3,MaxQuestion=256,MaxPromptTokens=96,MaxTokens=32,MaxAnswer=8192 };
struct Frame { uint32_t kind=0,id=0,value=0; std::string text; };
static inline bool Question(const std::string& q) { return !q.empty() && q.size()<=MaxQuestion && q.find('\0')==std::string::npos; }
static inline std::string Prompt(const std::string& q) {
 return "<|im_start|>user\n"+q+"<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
}
static inline void Put(uint8_t* p,uint32_t n) { for (unsigned i=0;i<4;i++) p[i]=uint8_t(n>>(8*i)); }
static inline uint32_t Get(const uint8_t* p) { uint32_t n=0; for (unsigned i=0;i<4;i++) n|=uint32_t(p[i])<<(8*i); return n; }
static inline bool Send(int fd,const Frame& f) {
 if (!f.id || f.text.size()>MaxAnswer || f.text.find('\0')!=std::string::npos) return false;
 uint8_t h[24]; Put(h,0x31413851); Put(h+4,1); Put(h+8,f.kind); Put(h+12,f.id); Put(h+16,f.value); Put(h+20,uint32_t(f.text.size()));
 for (unsigned part=0;part<2;part++) {
  const void* p=part?static_cast<const void*>(f.text.data()):h; size_t bytes=part?f.text.size():sizeof(h),at=0;
  while (at<bytes) {
   int flags=0;
#ifdef MSG_NOSIGNAL
   flags=MSG_NOSIGNAL;
#else
   int one=1; if (setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE,&one,sizeof(one))) return false; // Darwin offline tests
#endif
   ssize_t n=send(fd,static_cast<const char*>(p)+at,bytes-at,flags);
   if (n<0 && errno==EINTR) continue;
   if (n<=0) return false;
   at+=size_t(n);
  }
 }
 return true;
}
// Incremental nonblocking parser: fragmented header/body never blocks the
// service looper or loses bytes across polls. No shell, pointers, or paths.
class Receiver {
 uint8_t header[24]={}; size_t used=0,body=0; Frame pending;
public:
 enum Result { Waiting,Received,Lost,Invalid };
 Result Read(int fd,Frame& out) {
  for (;;) {
   if (used==24 && body==pending.text.size()) {
    if (pending.text.find('\0')!=std::string::npos) return Invalid;
    out=pending; pending=Frame(); used=body=0; return Received;
   }
   void* target=used<24?static_cast<void*>(header+used):static_cast<void*>(&pending.text[body]);
   size_t bytes=used<24?24-used:pending.text.size()-body;
   ssize_t n=recv(fd,target,bytes,MSG_DONTWAIT);
   if (n<0 && errno==EINTR) continue;
   if (n<0 && (errno==EAGAIN || errno==EWOULDBLOCK)) return Waiting;
   if (n<=0) return Lost;
   if (used<24) {
    used+=size_t(n); if (used!=24) continue;
    if (Get(header)!=0x31413851 || Get(header+4)!=1 || !Get(header+12) || Get(header+20)>MaxAnswer) return Invalid;
    pending.kind=Get(header+8); pending.id=Get(header+12); pending.value=Get(header+16); pending.text.resize(Get(header+20));
   } else body+=size_t(n);
  }
 }
};
}
#endif
