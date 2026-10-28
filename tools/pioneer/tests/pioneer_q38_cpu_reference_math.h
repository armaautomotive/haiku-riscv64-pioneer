/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_cpu_model_reference.h"
#ifndef PIONEER_Q38_CPU_REFERENCE_MATH_H
#define PIONEER_Q38_CPU_REFERENCE_MATH_H
#include <algorithm>
#include <array>
#include <cfenv>
#include <cmath>
#include <cstring>
#include <limits>
// Independent HOST row-major/libm arithmetic. No device quantizer,tile,
// SFPU implementation,readback or model-product input callback is included.
namespace Q38CpuReferenceMath {
inline uint32_t Bits(float x){uint32_t b;std::memcpy(&b,&x,4);return b;}
inline float Float(uint32_t b){float x;std::memcpy(&x,&b,4);return x;}
inline bool Normal(float x){unsigned a=Bits(x)&0x7fffffff;return !a || (a>=0x00800000 && a<0x7f800000);}
inline bool Bounded(float x,unsigned max=0x477fe000){unsigned a=Bits(x)&0x7fffffff;return !a || (a>=0x26800000 && a<=max);}
inline float Flush(float x){unsigned b=Bits(x);return b&0x7f800000?x:Float(b&0x80000000);}
inline unsigned U16(const uint8_t* p){return unsigned(p[0])|unsigned(p[1])<<8;}
inline int Signed(unsigned b){return int(b)-int((b&128)*2);}
inline float Half(unsigned h){
 unsigned e=h>>10&31,m=h&1023;
 float x=e==31?std::numeric_limits<float>::infinity():std::ldexp(float(e?1024+m:m),e?int(e)-25:-24);
 return std::copysign(x,h&0x8000?-1.f:1.f);
}
inline uint16_t HalfRne(float x){
 unsigned sign=Bits(x)>>16&0x8000;float a=std::fabs(x);if(!a)return uint16_t(sign);
 if(!std::isfinite(a))return uint16_t(sign|0x7c00);
 int e=0;std::frexp(a,&e);if(e< -13)return uint16_t(sign|unsigned(std::nearbyint(std::ldexp(a,24))));
 if(e>16)return uint16_t(sign|0x7c00);
 unsigned q=unsigned(std::nearbyint(std::ldexp(a,11-e)));if(q==2048){q=1024;e++;}
 return e>16?uint16_t(sign|0x7c00):uint16_t(sign|unsigned(e+14)*1024|(q-1024));
}
inline unsigned Scale4(const uint8_t* p,unsigned g){return g<4?p[4+g]&63:(p[8+g]&15)|((p[g]>>6)<<4);}
inline unsigned Min4(const uint8_t* p,unsigned g){return g<4?p[8+g]&63:(p[8+g]>>4)|((p[4+g]>>6)<<4);}
inline int Quant(unsigned type,const uint8_t* p,unsigned i){
 if(type==8)return Signed(p[2+i]);
 if(type==12)return int(p[16+i/64*32+i%32]>>((i/32%2)*4)&15);
 unsigned h=i/128,g=i%128/32,l=i%32;
 return int((p[h*64+(g%2)*32+l]>>((g/2)*4)&15)|((p[128+h*32+l]>>(g*2)&3)<<4))-32;
}
inline double SigmoidDouble(float x){double e=std::exp(-std::fabs(double(x)));return x>=0?1/(1+e):e/(1+e);}
inline float Sigmoid(float x){return float(SigmoidDouble(x));}
inline bool Rms(const std::vector<float>& input,const std::vector<float>& weight,std::vector<float>& out,bool hidden=true){
 unsigned n=unsigned(input.size());if((n!=5120 && n!=256) || weight.size()!=n || std::fegetround()!=FE_TONEAREST)return false;
 double sum=0;for(unsigned i=0;i<n;i++){if(!Bounded(input[i],hidden?0x4c800000:0x477fe000) || !Bounded(weight[i]))return false;sum+=double(input[i]*input[i]);}
 float scale=1.f/std::sqrt(float(sum/n)+1e-6f);if(!Bounded(scale) || !(scale>0))return false;
 std::vector<float> stage(n);for(unsigned i=0;i<n;i++){stage[i]=(input[i]*scale)*weight[i];if(!Normal(stage[i]))return false;}
 out=std::move(stage);return true;
}
inline bool Input(unsigned type,const std::vector<float>& input,std::vector<int>& q,std::vector<float>& scale){
 unsigned n=unsigned(input.size());if((type!=8 && type!=12 && type!=14) || !n || n>17408 || n%256 || std::fegetround()!=FE_TONEAREST)return false;
 unsigned width=type==8?32:256;q.assign(n,0);scale.assign(n/width,0);
 for(unsigned at=0;at<n;at+=width){
  float maximum=0,selected=0;
  for(unsigned i=0;i<width;i++){float x=input[at+i];if(!Normal(x))return false;if(std::fabs(x)>maximum){maximum=std::fabs(x);selected=x;}}
  if(!maximum)continue;
  float inverse=type==8?1.f/(maximum/127.f):127.f/maximum,s=type==8?Half(HalfRne(maximum/127.f)):1.f/inverse;
  if(!Normal(inverse) || !inverse || !Normal(s) || s>65536.f || (type!=8 && s<0x1p-32f))return false;
  if(type!=8){inverse=std::copysign(inverse,-selected);s=std::copysign(s,-selected);}scale[at/width]=s;
  for(unsigned i=0;i<width;i++){float x=input[at+i]*inverse,v=type==8?std::round(x):std::nearbyint(x);if(!std::isfinite(v) || v< -127 || v>127)return false;q[at+i]=int(v);}
 }
 return true;
}
template<class Read>bool Matrix(unsigned type,unsigned inputs,unsigned outputs,const std::vector<float>& input,std::vector<float>& output,Read read){
 if(input.size()!=inputs || !outputs || outputs%16 || (type!=8 && type!=12 && type!=14))return false;
 unsigned width=type==8?32:256,packed=type==8?34:type==12?144:210,blocks=inputs/width,rowBytes=blocks*packed;
 std::vector<int> q;std::vector<float> scales;if(!Input(type,input,q,scales))return false;
 std::vector<uint8_t> raw(16*rowBytes);std::vector<float> stage(outputs);
 for(unsigned first=0;first<outputs;first+=16){
  if(!read(uint64_t(first)*rowBytes,unsigned(raw.size()),raw.data()))return false;
  float sums[16]={};
  for(unsigned block=0;block<blocks;block++)for(unsigned panel=0;panel<width/16;panel++){
   int total=0;for(unsigned lane=0;lane<16;lane++)total+=q[block*width+panel*16+lane];
   for(unsigned row=0;row<16;row++){
    const auto* p=raw.data()+row*rowBytes+block*packed;float d=Half(U16(p+(type==14?208:0))),minimum=0;
    int factor=type==12?int(Scale4(p,panel/2)):type==14?Signed(p[192+panel]):1;
    if(!std::isfinite(d))return false;
    if(type==12){float m=Half(U16(p+2));if(!std::isfinite(m))return false;minimum=m*float(-int(Min4(p,panel/2)));}
    float coefficient=d*float(factor),c=coefficient*scales[block],v=minimum*scales[block];int dot=0;
    if(c!=0)for(unsigned lane=0;lane<16;lane++)dot+=Quant(type,p,panel*16+lane)*q[block*width+panel*16+lane];
    sums[row]+=(c*float(dot))+(v*float(total));if(!Normal(sums[row]))return false;
   }
  }
  for(unsigned row=0;row<16;row++)stage[first+row]=sums[row];
 }
 output=std::move(stage);return true;
}
inline bool Gdn(const float* q,const float* k,const float* value,float decay,float beta,float* state,float* out){
 if(!q || !k || !value || !state || !out || !Normal(decay) || !Normal(beta) || decay<0 || decay>1 || beta<0 || beta>1)return false;
 for(unsigned v=0;v<128;v++){
  double prediction=0;for(unsigned key=0;key<128;key++)prediction+=double(state[key*128+v]*decay)*k[key];
  float delta=(value[v]-float(prediction))*beta;double sum=0;
  for(unsigned key=0;key<128;key++){unsigned at=key*128+v;float updated=state[at]*decay+k[key]*delta;if(!Normal(updated))return false;state[at]=updated;sum+=double(updated)*q[key];}
  out[v]=float(sum)*0.0883883461356163f;if(!Normal(out[v]))return false;
 }
 return true;
}
inline void Rope(std::vector<float>& row,unsigned position){
 for(unsigned i=0;i<32;i++){
  float angle=float(position)*std::pow(10000000.f,-float(i)/32),c=std::cos(angle),s=std::sin(angle),a=row[i],b=row[i+32];
  row[i]=std::fma(a,c,-(b*s));row[i+32]=std::fma(a,s,b*c);
 }
}
inline bool Attention(const std::vector<float>& qg,const std::vector<float>& key,const std::vector<float>& value,
 const std::vector<float>& qnorm,const std::vector<float>& knorm,unsigned position,std::vector<float>& cacheK,std::vector<float>& cacheV,std::vector<float>& output){
 if(qg.size()!=12288 || key.size()!=1024 || value.size()!=1024 || qnorm.size()!=256 || knorm.size()!=256 || position>=8192 || cacheK.size()!=size_t(position)*1024 || cacheV.size()!=cacheK.size())return false;
 std::array<float,1024> nextK,nextV;std::vector<float> stage(6144);
 for(unsigned g=0;g<4;g++){
  std::vector<float> in(key.begin()+g*256,key.begin()+(g+1)*256),k;
  if(!Rms(in,knorm,k,false))return false;Rope(k,position);
  for(unsigned i=0;i<256;i++){if(!Bounded(k[i]) || !Bounded(value[g*256+i]))return false;nextK[g*256+i]=Half(HalfRne(k[i]));nextV[g*256+i]=Half(HalfRne(value[g*256+i]));}
 }
 // CPU-private provisional cache only. A later failure poisons the entire
 // reference instance; no retry or partial trace publication is allowed.
 cacheK.insert(cacheK.end(),nextK.begin(),nextK.end());cacheV.insert(cacheV.end(),nextV.begin(),nextV.end());
 for(unsigned h=0;h<24;h++){
  unsigned g=h/6;std::vector<float> in(qg.begin()+h*512,qg.begin()+h*512+256),query;
  if(!Rms(in,qnorm,query,false))return false;Rope(query,position);
  for(auto& x:query){if(!Bounded(x))return false;x=Half(HalfRne(x));}
  std::vector<float> probabilities(position+1);float maximum=-std::numeric_limits<float>::infinity();
  for(unsigned t=0;t<=position;t++){double sum=0;for(unsigned i=0;i<256;i++)sum+=double(query[i])*cacheK[t*1024+g*256+i];probabilities[t]=float(sum)*.0625f;maximum=std::max(maximum,probabilities[t]);}
  double denominator=0;for(auto& p:probabilities){p=std::exp(p-maximum);denominator+=p;}
  float inverse=float(1./denominator);if(!Normal(inverse) || !(inverse>0))return false;
  for(auto& p:probabilities)p=Half(HalfRne(p*inverse));
  for(unsigned i=0;i<256;i++){
   double sum=0;for(unsigned t=0;t<=position;t++)sum+=double(probabilities[t])*cacheV[t*1024+g*256+i];
   float gate=qg[h*512+256+i];if(!Bounded(gate))return false;stage[h*256+i]=Flush(float(sum)*Sigmoid(gate));if(!Normal(stage[h*256+i]))return false;
  }
 }
 output=std::move(stage);return true;
}
}
#endif
