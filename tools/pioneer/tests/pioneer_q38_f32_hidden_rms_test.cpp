/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38F32HiddenRms.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
static uint32_t randomState=0x712ac39b;
static uint32_t Next(){randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;return randomState;}
static void Reference(const std::vector<float>& x,const std::vector<float>& w,const std::vector<float>& actual){
 double sum=0;for(float v:x)sum+=double(v*v);float scale=1.f/sqrtf(float(sum/5120)+1e-6f);
 for(unsigned i=0;i<5120;i++)assert(PioneerAttentionBits(actual[i])==PioneerAttentionBits((x[i]*scale)*w[i]));
}
int main(){
 std::vector<float> x(5120),w(5120),out(5120),old(5120),projected(5120),residual(5120);
 unsigned rows=0,equivalent=0,refusals=0;
 for(unsigned trial=0;trial<512;trial++){
  for(unsigned i=0;i<5120;i++){
   uint32_t b=(Next()&0x807fffff)|((77+Next()%76)<<23);x[i]=PioneerAttentionFloat(b);w[i]=float(int(Next()%2049)-1024)*.001f;
   if(trial<8)x[i]=trial==0?67108864.f:trial==1?PioneerAttentionFloat(0x47816cda):trial==2?PioneerAttentionFloat(0x26800000)
    :trial==3?-0.f:trial==4?(i?67108864.f:PioneerAttentionFloat(0x26800000)):trial==5?(i?0.f:67108864.f):trial==6?-67108864.f:65504.f;
   if(trial<8)w[i]=trial==4?PioneerAttentionFloat(0x26800000):trial==5?65504.f:1.f;
  }
  assert(PioneerQ38F32HiddenRms(x.data(),w.data(),out.data()));Reference(x,w,out);rows++;
  if(trial>=8 && trial%2){
   for(unsigned i=0;i<5120;i++)x[i]=float(int(Next()%200001)-100000)*.13f;
   assert(PioneerQ38LearnedRms(x.data(),w.data(),old.data()) && PioneerQ38F32HiddenRms(x.data(),w.data(),out.data()));
   for(unsigned i=0;i<5120;i++)assert(PioneerAttentionBits(old[i])==PioneerAttentionBits(out[i]));equivalent++;
  }
 }
 std::fill(x.begin(),x.end(),PioneerAttentionFloat(0x47816cda));std::fill(w.begin(),w.end(),1);std::fill(old.begin(),old.end(),-37);
 assert(!PioneerQ38LearnedRms(x.data(),w.data(),old.data()) && PioneerQ38F32HiddenRms(x.data(),w.data(),out.data()));Reference(x,w,out);
 for(unsigned index:{0U,5119U})for(uint32_t bits:{1U,0x267fffffU,0x4c800001U,0x7f800000U,0xff800000U,0x7fc00000U}){
  std::fill(x.begin(),x.end(),1);std::fill(w.begin(),w.end(),1);std::fill(out.begin(),out.end(),-37);auto before=out;
  x[index]=PioneerAttentionFloat(bits);assert(!PioneerQ38F32HiddenRms(x.data(),w.data(),out.data()) && out==before);refusals++;
 }
 for(uint32_t bits:{1U,0x267fffffU,0x47800000U,0x47816cdaU,0x7f800000U,0x7fc00000U}){
  std::fill(x.begin(),x.end(),67108864.f);std::fill(w.begin(),w.end(),1);std::fill(out.begin(),out.end(),-37);auto before=out;
  w[5119]=PioneerAttentionFloat(bits);assert(!PioneerQ38F32HiddenRms(x.data(),w.data(),out.data()) && out==before);refusals++;
 }
 std::fill(x.begin(),x.end(),PioneerAttentionFloat(0x47719ad5));std::fill(projected.begin(),projected.end(),PioneerAttentionFloat(0x4589f6f1));std::fill(w.begin(),w.end(),1);
 assert(PioneerQ38F32HiddenResidualRms(projected.data(),x.data(),w.data(),residual.data(),out.data()));
 for(float v:residual)assert(PioneerAttentionBits(v)==0x47816cda);Reference(residual,w,out);
 std::fill(x.begin(),x.end(),67108864.f);std::fill(projected.begin(),projected.end(),67108864.f);std::fill(out.begin(),out.end(),-37);auto before=out;
 assert(!PioneerQ38F32HiddenResidualRms(projected.data(),x.data(),w.data(),residual.data(),out.data()) && out==before);refusals++;
 assert(!PioneerQ38F32HiddenRms(x.data(),w.data(),x.data()) && !PioneerQ38F32HiddenResidualRms(x.data(),x.data(),w.data(),residual.data(),out.data()));refusals+=2;
 printf("PASS NEW bounded FP32 hidden RMS rows%u/legacy-domain bitexact%u/refusals%u/captured4414.86767578125+61850.83203125=66265.703125/2^26 maximum+2^-126 product edge/UNCHANGED65504 learned-weight domain/independent double+sqrtf reference;NO controller/native/physical/Work/TPS authority\n",rows,equivalent,refusals);
}
