/* SPDX-License-Identifier: MIT */
// Synthetic IPC peer, never a model/device worker.
#include "../../../src/apps/assistant/Qwen27Protocol.h"
#include <cstdio>
#include <cstdlib>
#include <poll.h>
namespace A=Assistant27;
static FILE* trace;
static bool Read(A::Frame& frame){
 A::Receiver receiver;
 for(;;){
  auto result=receiver.Read(0,frame);
  if(result==A::Receiver::Received){std::fprintf(trace,"%u %u\n",frame.kind,frame.id);std::fflush(trace);return true;}
  if(result!=A::Receiver::Waiting)return false;
  pollfd p={0,POLLIN,0};if(poll(&p,1,1000)<0 && errno!=EINTR)return false;
 }
}
static bool Send(unsigned kind,unsigned id,unsigned value,const char* text){
 A::Frame f;f.kind=kind;f.id=id;f.value=value;f.text=text;return A::Send(1,f);
}
static bool Count(unsigned id,unsigned n){char text[180];std::snprintf(text,sizeof(text),
 "Full-card count committed%u prompts31 limit62;read-only DATA,not a terminal or cleanup receipt.",n);
 return Send(A::Progress,id,UINT32_MAX,text);}
int main(){
 const char* path=std::getenv("RESIDENT_RATE_TRACE"),*selected=std::getenv("RESIDENT_RATE_CASE");
 if(!path || !selected || !(trace=std::fopen(path,"w")))return 2;
 const unsigned mode=std::atoi(selected);A::Frame frame;
 const auto refused=[&]{return Read(frame)?3:0;};
 if(!Read(frame) || frame.id!=1 || frame.kind!=A::Load || frame.value!=64 || !frame.text.empty())return 2;
 if(!Send(A::Ready,1,0,mode==2?"QWEN27_FULL_OFFLOAD_VOCAB_READY":"QWEN27_FULL_OFFLOAD_RESIDENT_VOCAB_READY"))return 2;
 if(mode==2)return refused();
 for(unsigned id=2;id<=3;id++){
  if(!Read(frame) || frame.id!=id || frame.kind!=A::Generate || frame.value!=32
   || frame.text!="Count from 1 to 50, one number per line. Do not add explanations.")return 2;
  const char* upload="Full-card Qwen27B upload 18500.4/18500.4 MiB. No CPU inference.";
  const char* running="Qwen27B running entirely on P100A: embeddings,matrices,attention,KV,recurrent state,norms and greedy selection. CPU inference OFF.";
  if(id==2 || mode==4){if(!Send(A::Progress,id,1000,upload))return 2;if(mode==4 && id==3)return refused();}
  if(mode==3){Send(A::Progress,id,1000,upload);return refused();}
  if(mode==13 && id==3){Send(A::Progress,id,1000,"Unexpected second upload progress");return refused();}
  if(!Send(A::Progress,id,UINT32_MAX,running))return 2;
  if(id==3 && mode==12){Send(A::Error,id,A::Fatal,"Synthetic unknown warm start");return refused();}
  if(mode==7){Send(A::Progress,id,UINT32_MAX,running);return refused();}
  if(!Count(id,0) || !Count(id,31) || !Count(id,mode==5?30:62))return 2;
  if(mode==5)return refused();
  char rate[768];std::snprintf(rate,sizeof(rate),
   "Full-card observed count rate sampled31 first31 last62 elapsed_min_us999990 elapsed_max_us1000010 lower_tps%.9f upper_tps%.9f observations100 first_begin_us1000 first_end_us1010 last_begin_us1001000 last_end_us1001010;%s",
   mode==8?99.0:31000000.0/1000010,31000000.0/999990,mode==6?
   "AFTER_CLEAN_END,post-prompt count window,not isolated latency or broad quality.":
   "AFTER_CLEAN_REQUEST_CLOSE,model reservation retained;not isolated latency or broad quality.");
  if(!Send(A::Progress,id,UINT32_MAX,mode==1?
    "Full-card rate unavailable: insufficient post-prompt window or invalid timing;AFTER_CLEAN_REQUEST_CLOSE,no model END or extrapolation.":rate))return 2;
  if(mode==6 || mode==8)return refused();
  if(!Send(A::Text,id,0,id==3 && mode==9?"incorrect":"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14"))return 2;
  if(!Send(A::Done,id,A::Complete,
   "P100A full-offload resident candidate: 32 output tokens; prompt+generation 2.000s,total 4.000s. All inference on card; CPU vocabulary/text only. Request closed cleanly; weights retained until explicit model END. Not a warm-token speed measurement."))return 2;
 }
 if(!Read(frame) || frame.id!=4 || frame.kind!=A::Unload || frame.value || !frame.text.empty())return 2;
 if(!Send(A::Closed,4,0,mode==11?"Unverified close":"QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END"))return 2;
 std::fclose(trace);return mode==10?1:0;
}
