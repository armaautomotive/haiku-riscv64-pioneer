/* SPDX-License-Identifier: MIT */
#ifdef _KERNEL_MODE
#error Host observation arithmetic is NOT kernel code or a native authority
#endif
#ifndef PIONEER_Q38_TOKEN_RATE_SAMPLES_H
#define PIONEER_Q38_TOKEN_RATE_SAMPLES_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenFrontierPacket.h"
// Host DATA arithmetic only. The caller MUST separately prove SAME source/FD,
// genuine terminal/HOLD/drain/END+Close and record/count consistency BEFORE
// publishing a benchmark. This object cannot authorize any of those events.
class PioneerQ38TokenRateSamples {
 struct Sample {int64_t begin=0,end=0;uint32_t count=0;};
 uint32_t prompts=0,limit=0,previousCount=0;
 int64_t previousEnd=0;bool valid=false,observed=false,haveFirst=false,haveLast=false;
 Sample first,last;
 bool Fail(){valid=false;return false;}
public:
 struct Bounds {
  uint32_t prompts=0,limit=0,firstCommitted=0,lastCommitted=0,sampledTokens=0;
  int64_t minElapsedUs=0,maxElapsedUs=0;
  double lowerTokensPerSecond=0,upperTokensPerSecond=0;
 };
 struct Window {
  int64_t firstBegin=0,firstEnd=0,lastBegin=0,lastEnd=0;
  uint32_t firstCount=0,lastCount=0;
 };
 explicit PioneerQ38TokenRateSamples(uint32_t promptCount,uint32_t positionLimit):prompts(promptCount),limit(positionLimit){
  valid=prompts && prompts<limit && limit<=8192;
 }
 bool Observe(const PioneerQ38TokenFrontierControl::Packet& packet,int64_t begin,int64_t end){
  if(!valid)return false;
  if(!PioneerQ38TokenFrontierControl::ReplyShape(packet) || !packet.reply.ready
   || packet.reply.detail[8]!=limit || packet.reply.detail[9]!=prompts || begin<0 || end<begin
   || (observed && (begin<previousEnd || packet.reply.detail[7]<previousCount)))return Fail();
  const Sample sample={begin,end,packet.reply.detail[7]};
  observed=true;previousEnd=end;previousCount=sample.count;
  if(sample.count<prompts)return true; // never count a prompt position as decode
  if(!haveFirst){first=last=sample;haveFirst=true;return true;}
  if(sample.count>last.count){last=sample;haveLast=true;}
  return true; // repeated counts MUST NOT move endpoints into later idle time
 }
 bool ObservedBounds(Bounds& output)const{
  if(!valid || !haveFirst || !haveLast || last.count!=limit || first.count>=last.count)return false;
  // Each aligned count was read at an unknown instant INSIDE its call.
  // Bound elapsed time instead of pretending both were read at call return.
  const int64_t shortest=last.begin-first.end,longest=last.end-first.begin;
  if(shortest<=0 || longest<shortest)return false;
  Bounds result;result.prompts=prompts;result.limit=limit;result.firstCommitted=first.count;
  result.lastCommitted=last.count;result.sampledTokens=last.count-first.count;
  result.minElapsedUs=shortest;result.maxElapsedUs=longest;
  const double numerator=double(result.sampledTokens)*1000000.0;
  result.lowerTokensPerSecond=numerator/double(longest);result.upperTokensPerSecond=numerator/double(shortest);
  output=result;return true;
 } // observed commit-count rate bounds ONLY,not isolated token latency or a clean-END/model-quality certificate
 bool ObservedWindow(Window& output)const{
  Bounds ignored;if(!ObservedBounds(ignored))return false;
  output={first.begin,first.end,last.begin,last.end,first.count,last.count};return true;
 } // raw SAME observations for independent arithmetic;still no lifecycle/quality authority
};
static_assert(sizeof(PioneerQ38TokenRateSamples)<=128,"small host sampler,not an8192-position kernel stack array");
#endif
