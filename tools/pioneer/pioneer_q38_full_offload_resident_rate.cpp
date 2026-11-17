/* SPDX-License-Identifier: MIT */
// Two matched requests, one worker and one explicitly verified model END.
// Only the external guarded parent may grant native trial authority.
#ifndef PIONEER_FULL_OFFLOAD_SMOKE_WORKER
#error The resident rate client requires its separately staged worker path
#endif
#define PIONEER_FULL_OFFLOAD_CHANNEL_ONLY 1
#include "pioneer_q38_full_offload_smoke.cpp"
#include "PioneerQ38FullOffloadRateData.h"
#include "PioneerQ38FullOffloadRateDeadline.h"

using Deadline = PioneerQ38FullOffloadRateDeadline;
using Rate = PioneerQ38FullOffloadRateData;
static const char Question[]="Count from 1 to 50, one number per line. Do not add explanations.";
static const char Answer[]="1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14";
static const char RawUpload[]="Full-card Qwen27B upload 18500.4/18500.4 MiB. No CPU inference.";

static int ResidentMetric(Rate& metric, const std::string& text)
{
 if(Rate::Prefix(text,"Full-card count "))return metric.Observe(text);
 const bool measured=Rate::Prefix(text,"Full-card observed count rate ");
 const bool unavailable=Rate::Prefix(text,"Full-card rate unavailable:");
 if(!measured && !unavailable)return 0;
 const char* expected=measured?";AFTER_CLEAN_REQUEST_CLOSE,model reservation retained;not isolated latency or broad quality.":
  ";AFTER_CLEAN_REQUEST_CLOSE,no model END or extrapolation.";
 const char* canonical=measured?";AFTER_CLEAN_END,post-prompt count window,not isolated latency or broad quality.":
  ";AFTER_CLEAN_END,no extrapolation.";
 const auto boundary=text.find(';');
 if(boundary==std::string::npos || text.compare(boundary,std::string::npos,expected))return -1;
 // This normalization reuses only the existing numeric parser. It never
 // converts request closure into authority to claim model END.
 return metric.Observe(text.substr(0,boundary)+canonical);
}

static bool RunRequest(Channel& channel, bool warm, std::string& answer)
{
 const double begin=Now();
 if(begin<0 || !channel.Send(A::Generate,32,Question))return false;
 Deadline cold(begin);
 double warmUntil=begin+Deadline::StartSeconds;
 A::Frame frame;Rate metric;bool running=false,uploaded=false,textSeen=false;
 while(channel.Next(frame,warm?warmUntil:cold.Until())){
  if(frame.kind==A::Error && frame.value==A::Fatal){
   printf("RESIDENT_WORKER_ERROR %s\n",frame.text.c_str());return false;
  }
  if(frame.kind==A::Progress){
   if(textSeen)return false;
   const bool uploadProgress=Rate::Prefix(frame.text,Deadline::UploadPrefix);
   if(uploadProgress!=(frame.value!=UINT32_MAX))return false;
   if(uploadProgress){
    if(warm || running || uploaded || frame.value>1000)return false;
    if(frame.value==1000){if(frame.text!=RawUpload)return false;uploaded=true;}
   }
   const double now=Now();
   if(now<begin || (!warm && !cold.Observe(frame.value,frame.text,now)))return false;
   if(frame.text==Deadline::Running){
    if(running || frame.value!=UINT32_MAX || (!warm && !uploaded))return false;
    if(warm)warmUntil=now+Deadline::RunSeconds;
    running=true;
   }
   const int row=ResidentMetric(metric,frame.text);
   if(row<0 || (row && (!running || frame.value!=UINT32_MAX)) || !Progress(frame))return false;
   continue;
  }
  if(frame.kind==A::Text && !frame.value && !textSeen && metric.rateSeen){answer=frame.text;textSeen=true;continue;}
  if(frame.kind!=A::Done || frame.value!=A::Complete || !textSeen || !running)return false;
  unsigned tokens=0;double compute=0,total=0;int end=-1;
  if(std::sscanf(frame.text.c_str(),"P100A full-offload resident candidate: %u output tokens; prompt+generation %lfs,total %lfs. All inference on card; CPU vocabulary/text only. Request closed cleanly; weights retained until explicit model END. Not a warm-token speed measurement.%n",
   &tokens,&compute,&total,&end)!=3 || end<0 || size_t(end)!=frame.text.size() || !metric.Complete(tokens)
   || tokens!=32 || metric.prompts!=31 || metric.lastCount!=62
   || !std::isfinite(compute) || !std::isfinite(total) || compute<=0 || total<compute)return false;
  const double done=Now();if(done<begin)return false;
  printf("RESIDENT_REQUEST_CLOSED request%u uploads%u tokens%u host_seconds%.3f rate_available%u %s\n",
   warm?2U:1U,unsigned(uploaded),tokens,done-begin,unsigned(metric.available),frame.text.c_str());
  printf("RESIDENT_ANSWER request%u bytes%zu hex=",warm?2U:1U,answer.size());
  for(unsigned char byte:answer)printf("%02x",unsigned(byte));
  putchar('\n');
  // A well-formed closed request can still fail the reference answer gate. Let
  // the caller cleanly unload a known closed model before rejecting it.
  return true;
 }
 return false;
}

int main(int argc,char** argv)
{
 if(argc!=2 || std::strcmp(argv[1],"--full-card-resident-rate-32"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 Channel channel;if(!channel.Open())return 1;
 const double begin=Now();if(begin<0 || !channel.Send(A::Load,64))return 1;
 A::Frame frame;bool ready=false;
 while(channel.Next(frame,begin+1800)){
  if(Progress(frame))continue;
  ready=frame.kind==A::Ready && !frame.value && frame.text=="QWEN27_FULL_OFFLOAD_RESIDENT_VOCAB_READY";
  break;
 }
 if(!ready){puts("FAIL_RESIDENT_READY;CLOSE_ONLY");return 1;}
 puts("RESIDENT_READY_NO_CPU_INFERENCE;ONE_WORKER_TWO_MATCHED_REQUESTS");
 std::string first,second;
 if(!RunRequest(channel,false,first) || !RunRequest(channel,true,second)){
  printf("FAIL_RESIDENT_REQUEST receive=%s;CLOSE_ONLY_NO_RETRY_OR_FALLBACK\n",channel.nextFailure);return 1;
 }
 const double unload=Now();
 if(unload<0 || !channel.Send(A::Unload) || !channel.Next(frame,unload+120)
  || frame.kind!=A::Closed || frame.value || frame.text!="QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END" || !channel.Exit()){
  puts("FAIL_RESIDENT_MODEL_END;CLOSE_ONLY_NO_RETRY");return 1;
 }
 if(first!=Answer || second!=Answer){puts("FAIL_RESIDENT_REFERENCE_ANSWER;MODEL_END_VERIFIED");return 1;}
 puts("PASS_RESIDENT_TWO_REQUEST one_upload1 model_end1 worker_exit0 CPU_INFERENCE_CONTEXTS0;NOT_BROAD_QUALITY");
 return 0;
}
