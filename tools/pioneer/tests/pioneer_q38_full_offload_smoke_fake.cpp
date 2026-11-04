/* SPDX-License-Identifier: MIT */
// Framed transport fixture ONLY,not a model or a device worker.
#include "../../../src/apps/assistant/Qwen27Protocol.h"
#include <cassert>
#include <cstdlib>
#include <poll.h>
namespace A=Assistant27;
static A::Frame Read(){
 A::Receiver receiver;A::Frame frame;
 for(;;){auto result=receiver.Read(0,frame);if(result==A::Receiver::Received)return frame;
  assert(result==A::Receiver::Waiting);pollfd p={0,POLLIN,0};assert(poll(&p,1,1000)>=0);}
}
static void Reply(unsigned kind,unsigned id,unsigned value=0,const char* text=""){
 A::Frame frame;frame.kind=kind;frame.id=id;frame.value=value;frame.text=text;assert(A::Send(1,frame));
}
int main(){
 const char* value=getenv("FULL_OFFLOAD_SMOKE_CASE");assert(value);const int mode=atoi(value);
 auto load=Read();assert(load.kind==A::Load && load.id==1 && load.value==64 && load.text.empty());
 Reply(A::Progress,1,UINT32_MAX,"Fixture vocabulary only");
 if(mode==1){Reply(A::Ready,1,0,"QWEN27_WORK_HOST_READY_497");return 0;}
 if(mode==2){Reply(A::Ready,2,0,"QWEN27_FULL_OFFLOAD_VOCAB_READY");return 0;}
 if(mode==3){Reply(A::Ready,1,1,"QWEN27_FULL_OFFLOAD_VOCAB_READY");return 0;}
 Reply(A::Ready,1,0,"QWEN27_FULL_OFFLOAD_VOCAB_READY");
 const auto generate=Read();assert(generate.kind==A::Generate && generate.id==2 && generate.value==4 && generate.text=="Hello.");
 if(mode==4){Reply(A::Error,2,A::Fatal,"Synthetic unknown device result");return 0;}
 if(mode==5){Reply(A::Done,2,A::Cancelled,"Synthetic cancel");return 0;}
 if(mode==6){Reply(A::Progress,2,1001,"Invalid progress");return 0;}
 Reply(A::Progress,2,1000,"Synthetic upload");Reply(A::Text,2,0,mode==7?"Wrong answer":"Hello! How can");
 Reply(A::Done,2,0,"Synthetic full-offload completion");
 const auto unload=Read();assert(unload.kind==A::Unload && unload.id==3 && !unload.value && unload.text.empty());
 Reply(A::Closed,3,0,mode==8?"QWEN27_KNOWN_CLEAN_END":"QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END");
 return mode==9?1:0;
}
