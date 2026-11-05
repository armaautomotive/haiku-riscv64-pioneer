/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EXACT_Q4_FIXED_DATA_H
#define PIONEER_Q38_EXACT_Q4_FIXED_DATA_H
// Fixed SYNTHETIC qualification DATA ONLY. No learned weights or dot/affine
// calculation. Integer bit construction is exact;no RV floating helpers.
struct PioneerQ38ExactQ4FixedData {
 static unsigned Quant(unsigned seed,unsigned p,unsigned k,unsigned n){return (seed+p*11+k*17+n*3)&15;}
 static int Input(unsigned seed,unsigned p,unsigned k){return int((seed*29+p*13+k*37)&255)-128;}
 static unsigned Dyadic(unsigned magnitude,bool negative,unsigned fraction){
  if(!magnitude)return negative?0x80000000U:0;
  unsigned top=0;while(magnitude>>(top+1))top++;
  return (negative?0x80000000U:0)|((127+top-fraction)<<23)|((magnitude<<(23-top))&0x7fffff);
 }
 static unsigned InputBits(unsigned seed,unsigned p,unsigned k){const int x=Input(seed,p,k);return Dyadic(x<0?unsigned(-x):unsigned(x),x<0,0)>>16;}
 static unsigned Coefficient(unsigned seed,unsigned p,unsigned n){return Dyadic((seed+(p/2)*3+n)&63,bool((seed+p/2+n)&1),10);}
 static unsigned Minimum(unsigned seed,unsigned p,unsigned n){return Dyadic((seed*7+p/2+n*3)&63,true,12);}
 static unsigned Scale(unsigned seed,unsigned p){
  return 0x3b800000U|(((seed+p/16)*7919)&0x7fffff)|(((seed+p/16)&1)?0x80000000U:0);
 }
};
#endif
