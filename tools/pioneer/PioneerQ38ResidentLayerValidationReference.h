/* SPDX-License-Identifier: MIT */
// Gates precede the guard: late mixed-role inclusion is also forbidden.
#if !defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE) || PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE != 1 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1
#error Complete-layer ORIGINAL reference requires the separate exact CPU observer role
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_DECODER_RUNTIME) || defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || defined(PIONEER_Q38_RESIDENT_LAYER_OWNER) || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT) || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE)
#error Original complete-layer CPU observer never enters a device/native/Work role
#endif
#ifndef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE_H
#define PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE_H
#include "PioneerQ38NextGdnOutputValidationReference.h" // generic independent IEEE/half/Q8 DATA helpers ONLY
#include <array>
#include <vector>
// CPU-only reference PRODUCTS. Compute from ORIGINAL CPU embedding/GDN
// references BEFORE any accelerator FD. Never provide these to a native
// controller, byte endpoint, initialization, upload, owner or Work mode.
// This predicts the ideal libm-SiLU quantized pipeline, not a claim that
// the separately bounded device polynomial is bit-identical. Real model
// instruction/readback comparison is still required, never assumed.
struct PioneerQ38ResidentLayerReferenceProducts {
 uint32_t projection[5120],residual[5120],normalized[5120];
 uint32_t gate[17408],up[17408],activated[17408],down[5120],finalHidden[5120];
};
static_assert(sizeof(PioneerQ38ResidentLayerReferenceProducts)==311296,"reference products HEAP ONLY");
namespace PioneerQ38ResidentLayerReferenceDetail {
using PioneerQ38NextGdnOutputReferenceDetail::Bits;
using PioneerQ38NextGdnOutputReferenceDetail::Float;
using PioneerQ38NextGdnOutputReferenceDetail::HalfFloat;
inline bool Domain(uint32_t v){unsigned m=v&0x7fffffff;return !m || (m>=0x00800000 && m<0x7f800000);}
inline bool InputK(const uint32_t* x,unsigned words,std::vector<int>& quant,std::vector<float>& scales){
 if(!x || (words!=5120 && words!=17408) || std::fegetround()!=FE_TONEAREST)return false;
 quant.assign(words,0);scales.assign(words/256,0);
 for(unsigned b=0;b<words/256;b++){
  float maximum=0,selected=0;
  for(unsigned i=0;i<256;i++){
   uint32_t v=x[b*256+i];if(!Domain(v))return false;float f=Float(v);
   if(std::fabs(f)>maximum){maximum=std::fabs(f);selected=f;}
  }
  if(!maximum)continue;
  float inverse=127.f/maximum,scale=1.f/inverse;
  if(!Domain(Bits(inverse)) || !inverse || !Domain(Bits(scale)) || scale<0x1p-32f || scale>65536.f)return false;
  inverse=std::copysign(inverse,-selected);scales[b]=std::copysign(scale,-selected);
  for(unsigned i=0;i<256;i++){
   float rounded=std::nearbyint(Float(x[b*256+i])*inverse);
   if(!std::isfinite(rounded) || rounded<-127 || rounded>127)return false;
   quant[b*256+i]=int(rounded);
  }
 }return true;
}
// ORIGINAL row-major Q4_K bytes, NOT tiled panel unpack/native quantizer
// or card results. Explicit32-lane affine grouping matches the qualified
// matrix arithmetic contract; every integer dot is exact. Host IEEE/libm
// conversion/division/rounding is independent of the device bit policies.
template<class Source>bool MatrixK(Source& source,unsigned role,const uint32_t* input,uint32_t* output){
 if(role>=3 || !input || !output || !source.Ready() || std::fegetround()!=FE_TONEAREST)return false;
 const char* names[]={"blk.0.ffn_gate.weight","blk.0.ffn_up.weight","blk.0.ffn_down.weight"};
 const unsigned inputs=role==2?17408:5120,rows=role==2?5120:17408,blocks=inputs/256,rowBytes=blocks*144;
 const auto* found=source.Model().Find(names[role]);
 if(!found || found->type!=12 || found->rank!=2 || found->inputs!=inputs || found->outputs!=rows || found->bytes!=uint64_t(rows)*rowBytes)return false;
 const auto tensor=*found;const unsigned index=unsigned(found-source.Model().Tensors().data());
 auto current=[&](){
  if(!source.Ready() || std::fegetround()!=FE_TONEAREST || index>=source.Model().Tensors().size())return false;
  const auto& t=source.Model().Tensors()[index];
  return t.name==tensor.name && t.type==tensor.type && t.rank==tensor.rank && t.inputs==tensor.inputs
   && t.outputs==tensor.outputs && t.bytes==tensor.bytes && t.offset==tensor.offset;
 };
 std::vector<int> quant;std::vector<float> scales;if(!InputK(input,inputs,quant,scales))return false;
 std::vector<uint8_t> bytes(size_t(16)*rowBytes);std::vector<uint32_t> stage(rows);
 for(unsigned first=0;first<rows;first+=16){
  if(!current() || !source.ReadReferencePacked(index,uint64_t(first)*rowBytes,unsigned(bytes.size()),bytes.data(),unsigned(bytes.size())) || !current())return false;
  float sums[16]={};
  for(unsigned b=0;b<blocks;b++)for(unsigned group=0;group<8;group++){
   int total=0;for(unsigned k=0;k<32;k++)total+=quant[b*256+group*32+k];
   for(unsigned row=0;row<16;row++){
    const auto* p=bytes.data()+size_t(row)*rowBytes+b*144;const auto* s=p+4;
    uint16_t dh=uint16_t(p[0]|(unsigned(p[1])<<8)),mh=uint16_t(p[2]|(unsigned(p[3])<<8));
    if((dh&0x7c00)==0x7c00 || (mh&0x7c00)==0x7c00)return false;
    unsigned d=group<4?s[group]&63:(s[group+4]&15)|((s[group-4]>>6)<<4);
    unsigned m=group<4?s[group+4]&63:(s[group+4]>>4)|((s[group]>>6)<<4);
    int dot=0;for(unsigned k=0;k<32;k++){
     unsigned packed=p[16+(group/2)*32+k],q=(packed>>((group%2)*4))&15;dot+=int(q)*quant[b*256+group*32+k];
    }
    float coefficient=HalfFloat(dh)*float(d),minimum=HalfFloat(mh)*-float(m);
    float c=coefficient*scales[b],v=minimum*scales[b],product=c*float(dot),correction=v*float(total);
    sums[row]+=product+correction;if(!std::isfinite(sums[row]))return false;
   }
  }
  for(unsigned row=0;row<16;row++)stage[first+row]=Bits(sums[row]);
 }
 if(!current())return false;
 std::memcpy(output,stage.data(),rows*4);return true;
}
template<class Source>bool OutputProjection(Source& source,const uint32_t* gated,uint32_t* output){
 auto stage=std::make_unique<PioneerQ38NextGdnOutputReferenceResult>();
 if(!gated || !output || !source.Ready() || std::fegetround()!=FE_TONEAREST
  || !PioneerQ38NextGdnOutputReferenceDetail::Input(gated,*stage))return false;
 const auto* found=source.Model().Find("blk.0.ssm_out.weight");
 if(!found || found->type!=8 || found->rank!=2 || found->inputs!=6144 || found->outputs!=5120 || found->bytes!=uint64_t(5120)*6528)return false;
 const auto tensor=*found;unsigned index=unsigned(found-source.Model().Tensors().data());
 auto current=[&](){
  if(!source.Ready() || std::fegetround()!=FE_TONEAREST || index>=source.Model().Tensors().size())return false;
  const auto& t=source.Model().Tensors()[index];
  return t.name==tensor.name && t.type==tensor.type && t.rank==tensor.rank && t.inputs==tensor.inputs
   && t.outputs==tensor.outputs && t.bytes==tensor.bytes && t.offset==tensor.offset;
 };
 std::vector<uint8_t> bytes(16*6528);
 for(unsigned first=0;first<5120;first+=16){
  if(!current() || !source.ReadReferencePacked(index,uint64_t(first)*6528,unsigned(bytes.size()),bytes.data(),unsigned(bytes.size())) || !current())return false;
  float sum[16]={};
  for(unsigned block=0;block<192;block++)for(unsigned piece=0;piece<2;piece++)for(unsigned row=0;row<16;row++){
   const auto* p=bytes.data()+row*6528+block*34;uint16_t half=uint16_t(p[0]|(unsigned(p[1])<<8));if((half&0x7c00)==0x7c00)return false;
   int dot=0;for(unsigned k=0;k<16;k++){
    unsigned at=block*32+piece*16+k,raw=p[2+piece*16+k];uint16_t operand=uint16_t(stage->operand[at/2]>>((at%2)*16));
    int x=int(operand&1023)*(operand&0x8000?-1:1),w=int(raw)-int((raw&128)*2);dot+=x*w;
   }
   sum[row]+=(HalfFloat(half)*Float(stage->scale[block]))*float(dot);if(!std::isfinite(sum[row]))return false;
  }
  for(unsigned row=0;row<16;row++)stage->values[first+row]=Bits(sum[row]);
 }
 if(!current())return false;
 std::memcpy(output,stage->values,5120*4);return true;
}
}
template<class Source>bool PioneerQ38ResidentLayerOriginalReference(Source& source,const uint32_t* raw,const uint32_t* gated,
 PioneerQ38ResidentLayerReferenceProducts* output){
 using namespace PioneerQ38ResidentLayerReferenceDetail;
 static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,"IEEE binary32 reference");
 if(!raw || !gated || !output || !source.Ready() || std::fegetround()!=FE_TONEAREST)return false;
 uintptr_t r=reinterpret_cast<uintptr_t>(raw),g=reinterpret_cast<uintptr_t>(gated),o=reinterpret_cast<uintptr_t>(output);
 if((r&3) || (g&3) || (o&3) || r>UINTPTR_MAX-20480 || g>UINTPTR_MAX-24576 || o>UINTPTR_MAX-sizeof(*output)
  || (r<o+sizeof(*output) && o<r+20480) || (g<o+sizeof(*output) && o<g+24576))return false;
 for(unsigned i=0;i<5120;i++)if(!Domain(raw[i]))return false;
 auto stage=std::make_unique<PioneerQ38ResidentLayerReferenceProducts>();
 if(!OutputProjection(source,gated,stage->projection))return false;
 const auto* found=source.Model().Find("blk.0.post_attention_norm.weight");
 if(!found || found->type || found->rank!=1 || found->inputs!=5120 || found->outputs!=1 || found->bytes!=20480)return false;
 const auto tensor=*found;unsigned index=unsigned(found-source.Model().Tensors().data());std::array<uint8_t,20480> norm;
 if(!source.ReadReferencePacked(index,0,20480,norm.data(),20480) || !source.Ready() || std::fegetround()!=FE_TONEAREST
  || index>=source.Model().Tensors().size())return false;
 const auto& again=source.Model().Tensors()[index];
 if(again.name!=tensor.name || again.type!=tensor.type || again.rank!=tensor.rank || again.inputs!=tensor.inputs
  || again.outputs!=tensor.outputs || again.bytes!=tensor.bytes || again.offset!=tensor.offset)return false;
 double squares=0;
 for(unsigned i=0;i<5120;i++){
  float x=Float(raw[i])+Float(stage->projection[i]);if(!Domain(Bits(x)))return false;
  stage->residual[i]=Bits(x);float square=x*x;if(!std::isfinite(square))return false;squares+=double(square);
 }
 float scale=1.f/std::sqrt(float(squares/5120)+1e-6f);
 for(unsigned i=0;i<5120;i++){
  const auto* p=norm.data()+i*4;uint32_t v=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
  if(!Domain(v))return false;
  stage->normalized[i]=Bits((Float(stage->residual[i])*scale)*Float(v));
 }
 if(!MatrixK(source,0,stage->normalized,stage->gate) || !MatrixK(source,1,stage->normalized,stage->up))return false;
 for(unsigned i=0;i<17408;i++){
  double x=Float(stage->gate[i]),e=std::exp(-std::fabs(x));
  float silu=float(x*(x>=0?1/(1+e):e/(1+e))),value=silu*Float(stage->up[i]);
  if(!Domain(Bits(value)))return false;
  stage->activated[i]=Bits(value);
 }
 if(!MatrixK(source,2,stage->activated,stage->down))return false;
 for(unsigned i=0;i<5120;i++){
  float value=Float(stage->residual[i])+Float(stage->down[i]);if(!Domain(Bits(value)))return false;stage->finalHidden[i]=Bits(value);
 }
 if(!source.Ready() || std::fegetround()!=FE_TONEAREST)return false;
 std::memcpy(output,stage.get(),sizeof(*output));return true; // atomic CPU-only publication,NEVER a device input
}
#endif
