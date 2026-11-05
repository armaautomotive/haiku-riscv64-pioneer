/* SPDX-License-Identifier: MIT */
// Arithmetic words only;no Dst,worker,source,device or model integration.
#define PIONEER_Q38_Q4_BLOCK_INTEGER_AFFINE_OFFBOARD 1
#include "../PioneerQ38Q4BlockIntegerAffine.h"
#define PIONEER_Q38_VECTOR_AFFINE_TEST_MODEL_ONLY
#include "pioneer_q38_vector_affine_test.cpp"
#include <array>
#include <climits>
#ifdef NDEBUG
#error Assertions required
#endif
using IntegerAffine=PioneerQ38Q4BlockIntegerAffine;
struct IntegerVector:AffineVector {
 void Op(uint32_t word){
  if((word>>24)!=0x84){AffineVector::Op(word);return;}
  if(!okay)return;
  const unsigned a=(word>>16)&15,b=(word>>12)&15,c=(word>>8)&15,d=(word>>4)&15;
  assert(!(word&0x00f0000f) && a<8 && b<8 && c<8 && d<8);
  uint32_t result[32];
  for(unsigned lane=0;lane<32;lane++)result[lane]=PioneerBlackholeFmaBits(Read(a,lane),Read(b,lane),Read(c,lane));
  for(unsigned lane=0;lane<32;lane++)lr[d][lane]=result[lane];
  ready[d]=cycle+2;cycle++;
 }
};
static uint32_t Bits(float value){return PioneerFmaBits(value);}
static float Float(uint32_t value){return PioneerFmaFloat(value);}
static float Half(unsigned value){
 const int exponent=int((value>>10)&31),mantissa=int(value&1023);
 return std::ldexp(float(mantissa+(exponent?1024:0)),(exponent?exponent:1)-25)*((value&0x8000)?-1.f:1.f);
}
#ifndef PIONEER_Q38_Q4_BLOCK_INTEGER_AFFINE_TEST_MODEL_ONLY
int main(){
 assert(SM(-1)==0x80000001);
 AffineWords panel[2],finish[2];
 IntegerAffine::Panel<0>(panel[0]);IntegerAffine::Panel<2>(panel[1]);
 IntegerAffine::Finish<0>(finish[0]);IntegerAffine::Finish<2>(finish[1]);
 assert(panel[0].words.size()==3 && panel[1].words.size()==3);
 assert(finish[0].words.size()==12 && finish[1].words.size()==12);
 uint32_t seed=0x38b10c25;unsigned outputs=0,partialChecks=0,refusals=0,roundings=0,unsplitDifferences=0;
 auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
 const unsigned halfCases[]={0,0x8000,1,0x8001,0x3ff,0x83ff,0x400,0x8400,0x3c00,0xbc00,0x7bff,0xfbff};
 for(unsigned trial=0;trial<2048;trial++){
  IntegerVector m;std::array<std::array<int64_t,32>,2> dot{},minimum{},first{};
  std::array<std::array<float,32>,2> unsplit{};
  std::array<std::array<uint32_t,32>,2> second{},savedMinimum{};
  for(unsigned p=0;p<16;p++){
   if(p==8)for(unsigned parity=0;parity<2;parity++)for(unsigned lane=0;lane<32;lane++){
    first[parity][lane]=dot[parity][lane];
    assert(m.lr[4+parity*2][lane]==Bits(float(dot[parity][lane])));
    m.lr[4+parity*2][lane]=0;dot[parity][lane]=0;
   }
   for(unsigned parity=0;parity<2;parity++){
    uint32_t inputs[4][32],other[2][32];
    for(unsigned lane=0;lane<32;lane++){
     int d=int(random()%61441)-30720,s=int(random()%4097)-2048;
     unsigned scale=random()%64,min=random()%64;
     if(trial<16){
      d=(trial&1)?-30720:30720;s=(trial&2)?-2048:2048;scale=(trial&4)?0:63;min=(trial&8)?0:63;
      if(trial==3 && p>=8){d=-d;s=-s;}
      if(trial==1 && p==15)d++;
      if(trial==2 && p==8)d--;
      if(trial==2 && p>8)d=-d;
     }
     assert(IntegerAffine::PanelDomain(d,s,scale,min));
     m.lr[0][lane]=Bits(float(d));m.lr[1][lane]=Bits(float(scale));
     m.lr[2][lane]=Bits(float(min));m.lr[3][lane]=Bits(float(s));
     for(unsigned r=0;r<4;r++)inputs[r][lane]=m.lr[r][lane];
     other[0][lane]=m.lr[6-parity*2][lane];other[1][lane]=m.lr[7-parity*2][lane];
     dot[parity][lane]+=int64_t(d)*scale;minimum[parity][lane]+=int64_t(s)*min;
     volatile float term=float(d)*float(scale);
     unsplit[parity][lane]+=term;
    }
    for(auto word:panel[parity].words)m.Op(word);
    assert(m.okay);
    for(unsigned lane=0;lane<32;lane++){
     assert(std::abs(dot[parity][lane])<=IntegerAffine::PartialDotLimit);
     assert(std::abs(minimum[parity][lane])<=IntegerAffine::MinimumLimit);
     assert(m.Read(4+parity*2,lane)==Bits(float(dot[parity][lane])));
     assert(m.Read(5+parity*2,lane)==Bits(float(minimum[parity][lane])));
     for(unsigned r=0;r<4;r++)assert(inputs[r][lane]==m.lr[r][lane]);
     assert(other[0][lane]==m.lr[6-parity*2][lane] && other[1][lane]==m.lr[7-parity*2][lane]);
     partialChecks+=2;
    }
   }
  }
  for(unsigned parity=0;parity<2;parity++)for(unsigned lane=0;lane<32;lane++){
   second[parity][lane]=m.lr[4+parity*2][lane];savedMinimum[parity][lane]=m.lr[5+parity*2][lane];
  }
  for(unsigned parity=0;parity<2;parity++){
   uint32_t expected[32],untouched[2][32];
   for(unsigned lane=0;lane<32;lane++){
    // Test the documented standalone add against a separate int64->FP32 conversion.
    const uint32_t total=PioneerBlackholeFmaBits(Bits(float(first[parity][lane])),0x3f800000,second[parity][lane]);
    assert(total==Bits(float(first[parity][lane]+dot[parity][lane])));
    roundings+=int64_t(Float(total))!=first[parity][lane]+dot[parity][lane];
    unsplitDifferences+=Bits(unsplit[parity][lane])!=total;
    const float d=Half(halfCases[(trial+lane)%12]),dm=Half(halfCases[(trial*3+lane+1)%12]);
    const unsigned exp=95+random()%49;
    float scale=Float((exp<<23)|(exp==143?0:random()&0x7fffff)|(random()&0x80000000));
    if(trial%31==0)scale=Float(random()&0x80000000);
    float old=Float(((47+random()%149)<<23)|(random()&0x807fffff));
    if(trial%29==0)old=Float(random()&0x80000000);
    m.lr[0][lane]=total;m.lr[1][lane]=Bits(d);m.lr[2][lane]=Bits(-dm);
    m.lr[3][lane]=Bits(scale);m.lr[4][lane]=Bits(old);
    // Negating dmin changes zero signs too;this is deliberately a new contract.
    volatile float coefficient=d*scale,negativeMinimum=(-dm)*scale;
    volatile float product=coefficient*Float(total),correction=negativeMinimum*float(minimum[parity][lane]);
    volatile float value=product+correction;
    expected[lane]=Bits(old+value);
    assert(std::isfinite(Float(expected[lane])));
    untouched[0][lane]=m.lr[6][lane];untouched[1][lane]=m.lr[7][lane];
    assert(m.lr[5+parity*2][lane]==savedMinimum[parity][lane]);
   }
   for(auto word:finish[parity].words)m.Op(word);
   assert(m.okay);
   for(unsigned lane=0;lane<32;lane++){
    assert(m.Read(0,lane)==expected[lane]);
    assert(m.lr[6][lane]==untouched[0][lane] && m.lr[7][lane]==untouched[1][lane]);outputs++;
   }
  }
 }
 for(int d:{INT_MIN,-30721,30721,INT_MAX}){assert(!IntegerAffine::PanelDomain(d,0,0,0));refusals++;}
 for(int s:{INT_MIN,-2049,2049,INT_MAX}){assert(!IntegerAffine::PanelDomain(0,s,0,0));refusals++;}
 for(unsigned f:{64U,65535U,UINT_MAX}){
  assert(!IntegerAffine::PanelDomain(0,0,f,0) && !IntegerAffine::PanelDomain(0,0,0,f));refusals+=2;
 }
 unsigned timingFaults=0;
 for(unsigned parity=0;parity<2;parity++){
  IntegerVector m;for(unsigned lane=0;lane<32;lane++)for(unsigned r=0;r<8;r++)m.lr[r][lane]=Bits(1.f);
  for(unsigned i=0;i+1<panel[parity].words.size();i++)m.Op(panel[parity].words[i]);
  (void)m.Read(5+parity*2,0);assert(!m.okay);timingFaults++;
 }
 assert(roundings && unsplitDifferences);
 printf("PASS OFFBOARD block-integer arithmetic outputs%u exactPartialIntegerChecks%u roundedFullDotChecks%u unsplitControlDifferences%u domainRefusals%u timingFaults%u;ALL32lanes bothParities preservedOtherAccumulators;panel3_finish12words;NEW_ROUNDING_NOT_OLD_K16;NO_DST_WORKER_NATIVE_MODEL_QUALITY_OR_SPEED_QUALIFICATION\n",outputs,partialChecks,roundings,unsplitDifferences,refusals,timingFaults);
}
#endif
