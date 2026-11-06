/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Startup timing is host observation DATA,not inference or native authority
#endif
#ifndef PIONEER_Q38_STARTUP_TIMING_H
#define PIONEER_Q38_STARTUP_TIMING_H
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
// All stamps MUST use one monotonic microsecond clock. Invalid/missing DATA
// does not change model execution or grant cleanup/publication authority.
// The caller publishes only AFTER independently proven clean END+Close.
class PioneerQ38StartupTiming {
 int64_t loadBegin=-1,loadEnd=-1,request=-1,ready=-1;
 int64_t uploadBegin=-1,uploadEnd=-1,uploadSum=0,previousEnd=-1,below=-1,firstLow=0,firstHigh=0;
 uint64_t bytes=0;uint32_t calls=0,prompts=0,limit=0,previousCount=0;
 bool valid=true,observed=false,firstSeen=false,firstAvailable=false;
 void Invalidate(){valid=false;}
public:
 static constexpr const char* Unavailable="Full-card startup timing unavailable: invalid clock or incomplete samples;AFTER_CLEAN_END,no extrapolation.";
 struct Record {
  long long vocab=0,setup=0,window=0,ioctl=0,firstMin=0,firstMax=0,answerReady=0;
  unsigned calls=0,firstValid=0;unsigned long long bytes=0;bool available=false;
 };
 static bool Parse(const std::string& text,Record& output){
  Record r;
  if(text==Unavailable){output=r;return true;}
  const char* fields[]={"Full-card startup timing vocab_us"," setup_us"," upload_calls"," upload_bytes",
   " upload_window_us"," upload_ioctl_us"," first_commit_valid"," first_commit_min_us"," first_commit_max_us"," answer_ready_us"};
  uint64_t values[10]={};size_t cursor=0;
  for(unsigned field=0;field<10;field++){
   const size_t length=std::strlen(fields[field]);
   if(text.compare(cursor,length,fields[field]))return false;
   cursor+=length;const size_t begin=cursor;
   const uint64_t maximum=field==2 || field==6?UINT32_MAX:INT64_MAX;
   while(cursor<text.size() && text[cursor]>='0' && text[cursor]<='9'){
    const unsigned digit=unsigned(text[cursor]-'0');
    if(values[field]>(maximum-digit)/10)return false;
    values[field]=values[field]*10+digit;cursor++;
   }
   if(cursor==begin || (cursor-begin>1 && text[begin]=='0'))return false;
  }
  if(text.compare(cursor,std::string::npos,";AFTER_CLEAN_END,host_us,first_commit_not_streamed_text,upload_window_includes_between_call_preparation."))return false;
  r.vocab=values[0];r.setup=values[1];r.calls=unsigned(values[2]);r.bytes=values[3];
  r.window=values[4];r.ioctl=values[5];r.firstValid=unsigned(values[6]);
  r.firstMin=values[7];r.firstMax=values[8];r.answerReady=values[9];
  if(!r.calls || !r.bytes || r.bytes%4096
   || r.bytes<uint64_t(r.calls)*4096 || r.bytes>uint64_t(r.calls)*1048576 || r.window<0 || r.ioctl<0
   || r.ioctl>r.window || r.window>r.setup || r.firstValid>1 || r.firstMin<0 || r.firstMax<r.firstMin
   || r.firstMax>r.answerReady || r.answerReady<r.setup
   || (!r.firstValid && (r.firstMin || r.firstMax))
   || (r.firstValid && (r.firstMax<r.setup || (r.firstMin && r.firstMin<r.setup))))return false;
  r.available=true;output=r;return true;
 }
 void LoadBegin(int64_t now){if(loadBegin>=0 || now<0)Invalidate();else loadBegin=now;}
 void LoadReady(int64_t now){if(loadBegin<0 || loadEnd>=0 || now<loadBegin)Invalidate();else loadEnd=now;}
 void Begin(int64_t now,uint32_t promptCount,uint32_t positionLimit){
  if(!valid || loadEnd<0 || now<loadEnd || !promptCount || promptCount>96
   || positionLimit<promptCount || positionLimit>promptCount+31 || positionLimit>128){Invalidate();return;}
  request=now;ready=-1;uploadBegin=uploadEnd=previousEnd=-1;uploadSum=0;bytes=0;calls=0;
  prompts=promptCount;limit=positionLimit;previousCount=0;below=now;firstLow=firstHigh=0;
  observed=firstSeen=firstAvailable=false;
 }
 void Upload(int64_t before,int64_t after,uint64_t count){
  if(!valid || request<0 || ready>=0 || before<request || after<before
   || (calls && before<uploadEnd) || !count || count>1048576 || count%4096
   || calls==UINT32_MAX || bytes>UINT64_MAX-count || uploadSum>INT64_MAX-(after-before)){Invalidate();return;}
  if(!calls)uploadBegin=before;
  uploadEnd=after;uploadSum+=after-before;bytes+=count;calls++;
 }
 void Ready(int64_t now){
  if(!valid || request<0 || ready>=0 || !calls || now<uploadEnd){Invalidate();return;}
  ready=now;
 }
 void Observe(uint32_t count,uint32_t promptCount,uint32_t positionLimit,int64_t before,int64_t after){
  if(!valid || ready<0 || before<ready || after<before || promptCount!=prompts || positionLimit!=limit
   || count>limit || (observed && (before<previousEnd || count<previousCount))){Invalidate();return;}
  observed=true;previousEnd=after;previousCount=count;
  if(count<prompts){below=before;return;}
  if(!firstSeen){
   firstSeen=true;firstAvailable=count==prompts;
   if(firstAvailable){firstLow=below-request;firstHigh=after-request;}
  }
 } // first generated commit is position prompts,NOT the final prompt-1.
 bool Format(int64_t answerReady,char* text,unsigned capacity)const{
  const bool complete=valid && ready>=0 && observed && firstSeen && previousCount>=prompts
   && answerReady>=previousEnd && answerReady>=ready && uploadSum<=uploadEnd-uploadBegin;
  int n;
  if(!complete)n=std::snprintf(text,capacity,"%s",Unavailable);
  else n=std::snprintf(text,capacity,
   "Full-card startup timing vocab_us%lld setup_us%lld upload_calls%u upload_bytes%llu upload_window_us%lld upload_ioctl_us%lld first_commit_valid%u first_commit_min_us%lld first_commit_max_us%lld answer_ready_us%lld;AFTER_CLEAN_END,host_us,first_commit_not_streamed_text,upload_window_includes_between_call_preparation.",
   (long long)(loadEnd-loadBegin),(long long)(ready-request),calls,(unsigned long long)bytes,
   (long long)(uploadEnd-uploadBegin),(long long)uploadSum,unsigned(firstAvailable),
   (long long)firstLow,(long long)firstHigh,(long long)(answerReady-request));
  return n>=0 && unsigned(n)<capacity;
 }
};
#endif
