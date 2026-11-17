/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FULL_OFFLOAD_RATE_DEADLINE_H
#define PIONEER_Q38_FULL_OFFLOAD_RATE_DEADLINE_H
#include <cmath>
#include <cstdint>
#include <string>

// Host benchmark watchdog only. Progress is not a native cleanup receipt.
// Arbitrary progress, counters, and repeated milestones never buy more time.
class PioneerQ38FullOffloadRateDeadline {
public:
 static constexpr unsigned UploadSeconds=7200,StartSeconds=1200,RunSeconds=2100;
 static constexpr const char* Running="Qwen27B running entirely on P100A: embeddings,matrices,attention,KV,recurrent state,norms and greedy selection. CPU inference OFF.";
 static constexpr char UploadPrefix[]="Full-card Qwen27B upload ";

 explicit PioneerQ38FullOffloadRateDeadline(double now):entered(now){}
 bool Valid()const{return std::isfinite(entered) && entered>=0;}
 double Until()const{return entered+Budget();}
 unsigned Budget()const{return phase==Upload?UploadSeconds:phase==Start?StartSeconds:RunSeconds;}
 const char* Name()const{return phase==Upload?"upload_or_setup":phase==Start?"post_upload_start":"on_card_or_cleanup";}
 bool Started()const{return phase==Run;}

 bool Observe(unsigned value,const std::string& text,double now){
  const bool upload=value==1000 && text.compare(0,sizeof(UploadPrefix)-1,UploadPrefix)==0;
  const bool running=text==Running;
  if(!upload && !running)return true;
  if(!Valid() || !std::isfinite(now) || now<entered || now>Until())return false;
  if(upload){
   if(phase!=Upload || (text!="Full-card Qwen27B upload 18500.4/18500.4 MiB. No CPU inference."
    && text!="Full-card Qwen27B upload 19522.4/19522.4 MiB. No CPU inference."))return false;
   phase=Start;
  }else{
   if(value!=UINT32_MAX || phase!=Start)return false;
   phase=Run;
  }
  entered=now;return true;
 }

private:
 enum Phase {Upload,Start,Run} phase=Upload;
 double entered;
};
#endif
