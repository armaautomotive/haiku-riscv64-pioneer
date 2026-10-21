/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_AFFINE_TEST_MODEL_ONLY
#include "pioneer_q38_vector_affine_test.cpp"
#include "../PioneerQ38ZeroMinimumAffine.h"
// Independent Blackhole SFPCAST_IntFloat:sign is retained EVEN for zero.
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/SFPCAST_IntFloat.md
struct RawDotVector:AffineVector {
 void Op(uint32_t word) {
  if(word>>24!=0x90){AffineVector::Op(word);return;}
  unsigned d=(word>>4)&15,s=(word>>8)&15;assert(d<8 && s<8 && !(word&0x00fff00f));
  uint32_t value[32];
  for(unsigned lane=0;lane<32;lane++) {
   uint32_t raw=Read(s,lane),mag=raw&0x7fffffff;assert(mag<=(s==0?262144U:2048U));
   value[lane]=PioneerFmaBits(float(mag))|(raw&0x80000000U);
  }
  for(unsigned lane=0;lane<32;lane++)lr[d][lane]=value[lane];ready[d]=cycle+1;cycle++;
 }
};
int main() {
 AffineWords full,shorter;PioneerQ38VectorAffine(full);PioneerQ38ZeroMinimumAffine(shorter);
 uint32_t seed=0x73e9a1d5;unsigned compared=0,negativeZeros=0;
 auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
 for(bool zeroMinimum:{false,true})for(unsigned trial=0;trial<512;trial++) {
  uint32_t accumulator[32]={};
  for(unsigned step=0;step<32;step++) {
   RawDotVector old,next;
   for(unsigned lane=0;lane<32;lane++) {
    uint32_t coefficient,minimum;
    assert(PioneerQ38BinaryScale(uint16_t((random()%0x7c00)|(random()&0x8000)),zeroMinimum?int(random()%256)-128:int(random()%64),coefficient));
    assert(PioneerQ38BinaryScale(uint16_t((random()%0x7c00)|(random()&0x8000)),-int(random()%64),minimum));
    if(zeroMinimum)minimum=0;
    unsigned exp=95+random()%49;uint32_t scale=(exp<<23)|(exp==143?0:random()&0x7fffff)|(random()&0x80000000);
    if(!(step%7))scale=random()&0x80000000;
    if(!(step%5))coefficient=random()&0x80000000;
    if(!(step%3))minimum=zeroMinimum?0:random()&0x80000000;
    int dot=int(random()%524289)-262144,sum=int(random()%4097)-2048;
    if(!(step%2))dot=0;if(!(step%4))sum=0;
    uint32_t dotBits=SM(dot);if(!dot && (random()&1)){dotBits=0x80000000;negativeZeros++;}
    next.lr[0][lane]=dotBits;old.lr[0][lane]=(dotBits&0x7fffffff)?dotBits:0;
    for(auto* v:{&old,&next}) {
     v->lr[1][lane]=coefficient;v->lr[2][lane]=minimum;v->lr[3][lane]=scale;v->lr[4][lane]=SM(sum);v->lr[5][lane]=accumulator[lane];
    }
   }
   const auto& ops=zeroMinimum?shorter.words:full.words;
   for(auto word:ops){old.Op(word);next.Op(word);}assert(old.okay && next.okay);
   for(unsigned lane=0;lane<32;lane++) {
    assert(old.lr[0][lane]==next.lr[0][lane] && next.lr[0][lane]!=0x80000000
      && std::isfinite(PioneerFmaFloat(next.lr[0][lane])));
    accumulator[lane]=next.lr[0][lane];compared++;
   }
  }
 }
 // Necessity of +0 initialization:an arbitrary -0 accumulator is NOT
 // covered by this Work-specific induction or a blanket zero relaxation.
 for(bool zeroMinimum:{false,true}) {
  RawDotVector old,next;
  for(unsigned lane=0;lane<32;lane++) {
   old.lr[0][lane]=0;next.lr[0][lane]=0x80000000;
   for(auto* v:{&old,&next}) {
    v->lr[1][lane]=v->lr[3][lane]=0x3f800000;v->lr[2][lane]=0x80000000;
    v->lr[4][lane]=SM(1);v->lr[5][lane]=0x80000000;
   }
  }
  for(auto word:(zeroMinimum?shorter.words:full.words)){old.Op(word);next.Op(word);}
  assert(old.okay && next.okay && old.lr[0][0]==0 && next.lr[0][0]==0x80000000);
 }
 printf("PASS OFFLINE Work dot-zero proof:%u exact lanewise recurrent comparisons,%u raw negative-zero dots,SFPCAST preserves sign,ALL original FP32 rounding/MAD waits,positive-zero accumulator induction;negative-zero accumulator counterexample retained;NO native/kernel change\n",compared,negativeZeros);
}
