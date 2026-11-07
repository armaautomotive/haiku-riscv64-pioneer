/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_BFP4_FFN_QUALITY_H
#define PIONEER_BFP4_FFN_QUALITY_H
#include "PioneerBfp4PromptQuality.h"
// CPU-only streaming diagnostic. Original model is authenticated separately;
// no runtime selection, GPU access, fallback or user-visible quality claim.
namespace PioneerBfp4FfnQuality {
inline float Float(uint32_t bits){float value;std::memcpy(&value,&bits,4);return value;}
inline uint32_t Bits(float value){uint32_t bits;std::memcpy(&bits,&value,4);return bits;}
template<class Source>bool PackBand(Source& source,unsigned index,unsigned first,std::vector<uint8_t>& output){
 if(!source.Ready() || !source.Stable() || index>=source.Tensors().size())return false;
 const auto tensor=source.Tensors()[index];
 if(tensor.type!=12 || tensor.rank!=2 || !tensor.inputs || tensor.inputs>17408 || tensor.inputs%32
  || !tensor.outputs || tensor.outputs>17408 || tensor.outputs%32 || first%32 || first>=tensor.outputs)return false;
 const unsigned width=unsigned(tensor.inputs);const uint64_t epoch=source.Epoch();std::vector<float> block(size_t(32)*width);
 std::vector<uint8_t> packed(size_t(width/32)*576);std::array<float,1024> tile={};
 for(unsigned row=0;row<32;row++)if(!source.ReadRow(index,first+row,block.data()+size_t(row)*width,width)
  || !source.Ready() || !source.Stable() || source.Epoch()!=epoch)return false;
 for(unsigned column=0;column<width/32;column++){
  for(unsigned row=0;row<32;row++)for(unsigned k=0;k<32;k++)tile[row*32+k]=block[size_t(row)*width+column*32+k];
  if(!PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,tile.data(),1024,packed.data()+column*576,576))return false;
 }
 if(!source.Ready() || !source.Stable() || source.Epoch()!=epoch)return false;
 output=std::move(packed);return true;
}
template<class Source>bool Project(Source& source,unsigned index,const std::vector<uint32_t>& input,std::vector<uint32_t>& output,
 bool (*progress)(unsigned,unsigned)=nullptr){
 if(!source.Ready() || !source.Stable() || index>=source.Tensors().size())return false;
 const auto tensor=source.Tensors()[index];
 if(tensor.type!=12 || tensor.rank!=2 || !tensor.inputs || tensor.inputs>17408 || tensor.inputs%32
  || !tensor.outputs || tensor.outputs>17408 || tensor.outputs%32 || input.size()!=tensor.inputs)return false;
 const unsigned width=unsigned(tensor.inputs),rows=unsigned(tensor.outputs);const uint64_t epoch=source.Epoch();
 std::vector<uint32_t> predicted(rows);std::vector<uint8_t> packed;
 for(unsigned first=0;first<rows;first+=32){
  if(!PackBand(source,index,first,packed) || source.Epoch()!=epoch)return false;
  std::array<uint32_t,32> band={};if(!PioneerBfp4PromptQuality::Predict(packed.data(),packed.size(),input.data(),width,band))return false;
  std::copy(band.begin(),band.end(),predicted.begin()+first);
  if(progress && ((first+32)%1024==0 || first+32==rows) && !progress(first+32,rows))return false;
 }
 if(!source.Ready() || !source.Stable() || source.Epoch()!=epoch)return false;
 output=std::move(predicted);return true;
}
inline bool Activate(const std::vector<uint32_t>& gate,const std::vector<uint32_t>& up,std::vector<uint32_t>& output){
 if(gate.size()!=17408 || up.size()!=gate.size())return false;
 std::vector<uint32_t> next(gate.size());
 for(unsigned i=0;i<gate.size();i++){
  float g=Float(gate[i]),u=Float(up[i]);if(!std::isfinite(g) || !std::isfinite(u))return false;
  double e=std::exp(-std::fabs(double(g)));float sigmoid=float(g>=0?1/(1+e):e/(1+e));
  float value=(g*sigmoid)*u;if(!std::isfinite(value))return false;next[i]=Bits(value);
 }
 output=std::move(next);return true;
}
inline bool Compare(const std::vector<uint32_t>& observed,const std::vector<uint32_t>& baseline,PioneerBfp4PromptQuality::Result& result){
 if(observed.empty() || observed.size()!=baseline.size())return false;
 long double norm=0,error=0;double maximum=0;
 for(size_t i=0;i<observed.size();i++){
  float a=Float(observed[i]),b=Float(baseline[i]);if(!std::isfinite(a) || !std::isfinite(b))return false;
  long double d=static_cast<long double>(a)-b;norm+=static_cast<long double>(b)*b;error+=d*d;maximum=std::max(maximum,double(std::fabs(d)));
 }
 if(!(norm>0))return false;
 PioneerBfp4PromptQuality::Result next={double(std::sqrt(error/norm)),maximum};
 if(!std::isfinite(next.relativeL2) || !std::isfinite(next.maxAbsolute))return false;
 result=next;return true;
}
}
#endif
