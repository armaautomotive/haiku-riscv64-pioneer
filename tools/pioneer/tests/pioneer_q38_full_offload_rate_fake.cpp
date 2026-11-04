/* SPDX-License-Identifier: MIT */
// FAKE framed worker only;not model inference or device cleanup evidence.
#include "../../../src/apps/assistant/Qwen27Protocol.h"
#include <cstdlib>
#include <cstdio>
#include <poll.h>
namespace A=Assistant27;
static bool Read(A::Frame& frame){
 A::Receiver receiver;
 for(;;){auto r=receiver.Read(0,frame);if(r==A::Receiver::Received)return true;
  if(r!=A::Receiver::Waiting)return false;
  pollfd p={0,POLLIN,0};if(poll(&p,1,1000)<0 && errno!=EINTR)return false;}
}
static bool Reply(unsigned kind,unsigned id,unsigned value=0,const char* text=""){
 A::Frame f;f.kind=kind;f.id=id;f.value=value;f.text=text;return A::Send(1,f);
}
static bool Count(unsigned n){char text[192];snprintf(text,sizeof(text),
 "Full-card count committed%u prompts20 limit51;read-only DATA,not a terminal or cleanup receipt.",n);
 return Reply(A::Progress,2,UINT32_MAX,text);}
int main(){
 const char* value=getenv("FULL_OFFLOAD_RATE_CASE");if(!value)return 2;const int mode=atoi(value);
 A::Frame f;if(!Read(f) || f.kind!=A::Load || f.id!=1 || f.value!=64 || !f.text.empty())return 2;
 if(mode==2){Reply(A::Ready,1,0,"QWEN27_WORK_HOST_READY_497");return 0;}
 if(!Reply(A::Ready,mode==3?2:1,0,"QWEN27_FULL_OFFLOAD_VOCAB_READY"))return 0;
 if(mode==3)return 0;
 if(!Read(f) || f.kind!=A::Generate || f.id!=2 || f.value!=32
  || f.text!="Count from 1 to 50, one number per line. Do not add explanations.")return 2;
 if(mode==4){Reply(A::Error,2,A::Fatal,"Synthetic unknown");return 0;}
 if(mode==5){Reply(A::Done,2,A::Cancelled,"Synthetic cancelled");return 0;}
 if(!Count(0) || !Count(20))return 0;
 if(mode==6){Count(19);return 0;}
 if(!Count(mode==1?28:51))return 0;
 char metric[768];snprintf(metric,sizeof(metric),
 "Full-card observed count rate sampled31 first20 last51 elapsed_min_us999990 elapsed_max_us1000010 lower_tps%.9f upper_tps%.9f observations100 first_begin_us1000 first_end_us1010 last_begin_us1001000 last_end_us1001010;AFTER_CLEAN_END,post-prompt count window,not isolated latency or broad quality.",
 mode==7?99.0:31000000.0/1000010,31000000.0/999990);
 const char* unavailable="Full-card rate unavailable: insufficient post-prompt window or invalid timing;AFTER_CLEAN_END,no extrapolation.";
 if(mode!=8 && !Reply(A::Progress,2,UINT32_MAX,mode==1?unavailable:metric))return 0;
 if(mode==7)return 0;
 if(!Reply(A::Text,2,0,"1\n2\n3\n4\n5\n6\n7\n8"))return 0;
 if(mode==8)return 0;
 if(mode==9){Reply(A::Text,2,0,"duplicate");return 0;}
 if(mode==10){Count(51);return 0;}
 if(!Reply(A::Done,2,A::Complete,mode==1?
  "P100A full-offload candidate: 8 output tokens; prompt+generation 2.000s,total 4.000s. All inference on card; CPU vocabulary/text only. Clean END; weights released. Not a warm-token speed measurement.":
  "P100A full-offload candidate: 32 output tokens; prompt+generation 2.000s,total 4.000s. All inference on card; CPU vocabulary/text only. Clean END; weights released. Not a warm-token speed measurement."))return 0;
 if(!Read(f))return 0;
 if(f.kind!=A::Unload || f.id!=3 || f.value || !f.text.empty())return 2;
 Reply(A::Closed,3,0,mode==11?"QWEN27_KNOWN_CLEAN_END":"QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END");
 return mode==12?1:0;
}
