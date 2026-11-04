/* SPDX-License-Identifier: MIT */
// Same real IPC/process lifecycle as the four-token client. A longer public
// request to the full-card worker;NO model/ggml/CPU inference API or fallback.
#define PIONEER_FULL_OFFLOAD_CHANNEL_ONLY 1
#include "pioneer_q38_full_offload_smoke.cpp"
#include "PioneerQ38FullOffloadRateData.h"
static const char Question[]="Count from 1 to 50, one number per line. Do not add explanations.";
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--full-card-rate-32"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 Channel channel;if(!channel.Open())return 1;
 const double began=Now();if(began<0 || !channel.Send(A::Load,64))return 1;
 A::Frame frame;bool ready=false;
 while(channel.Next(frame,began+1800)){
  if(Progress(frame))continue;
  ready=frame.kind==A::Ready && !frame.value && frame.text=="QWEN27_FULL_OFFLOAD_VOCAB_READY";break;
 }
 if(!ready){puts("FAIL_FULL_OFFLOAD_RATE_READY;CLOSE_ONLY_NO_FALLBACK");return 1;}
 puts("FULL_OFFLOAD_RATE_READY_NO_CPU_CONTEXT;REQUEST_MAX32_PUBLIC_COUNTING");
 if(!channel.Send(A::Generate,32,Question))return 1;
 const double generate=Now();bool complete=false,textSeen=false;std::string answer,status;
 PioneerQ38FullOffloadRateData metric;
 while(generate>=0 && channel.Next(frame,generate+7200)){
  if(frame.kind==A::Progress){
   if(textSeen || metric.Observe(frame.text)<0 || !Progress(frame))return 1;
   continue;
  }
  if(frame.kind==A::Text && !frame.value && !textSeen && metric.rateSeen){answer=frame.text;textSeen=true;continue;}
  if(frame.kind==A::Done && frame.value==A::Complete && textSeen){status=frame.text;complete=true;}
  break;
 }
 if(!complete){puts("FAIL_FULL_OFFLOAD_RATE_GENERATION;CLOSE_ONLY_NO_END_RETRY_RESET_OR_CPU_FALLBACK");return 1;}
 unsigned tokens=0;double compute=0,total=0;int end=-1;
 if(std::sscanf(status.c_str(),"P100A full-offload candidate: %u output tokens; prompt+generation %lfs,total %lfs. All inference on card; CPU vocabulary/text only. Clean END; weights released. Not a warm-token speed measurement.%n",
  &tokens,&compute,&total,&end)!=3 || end<0 || size_t(end)!=status.size() || !metric.Complete(tokens)
  || !std::isfinite(compute) || !std::isfinite(total) || compute<=0 || total<compute)return 1;
 printf("FULL_OFFLOAD_RATE_DONE %s\n",status.c_str());
 printf("FULL_OFFLOAD_RATE_ANSWER bytes%zu hex=",answer.size());
 for(unsigned char byte:answer){printf("%02x",unsigned(byte));}
 putchar('\n');
 if(!channel.Send(A::Unload) || !channel.Next(frame,Now()+120) || frame.kind!=A::Closed || frame.value
  || frame.text!="QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END" || !channel.Exit()){
  puts("FAIL_FULL_OFFLOAD_RATE_WORKER_CLEAN_CLOSE;NO_RETRY");return 1;
 }
 // A clean job can have an insufficient timing window;that is NOT a
 // hardware failure or permission to invent a throughput figure.
 printf("PASS_FULL_OFFLOAD_RATE_JOB clean_worker_exit1 rate_available%u output_tokens%u;CPU_INFERENCE_CONTEXTS0;POST_PROMPT_COUNT_WINDOW_NOT_BROAD_QUALITY_OR_ISOLATED_LATENCY\n",
  unsigned(metric.available),tokens);
 return 0;
}
