/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE
#error Recurrent BLOCK IMAGE is not a native driver or CPU reference
#endif
#if defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE)
#error New recurrent ROOT/capsule is not a CPU reference or validation client
#endif
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE
#error Recurrent projection image is separate from CPU validation
#endif
#if defined(PIONEER_Q38_DENSE_PREPARE_IMAGE) || defined(PIONEER_Q38_DENSE_ATTENTION_IMAGE) || defined(PIONEER_Q38_DENSE_PROJECTION_IMAGE) || defined(PIONEER_Q38_DENSE_PARTITION_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) || defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE)
#error Local dense-prepare IMAGE cannot contain independent CPU references
#endif
#ifdef PIONEER_Q38_RESIDENT_CONTINUE_IMAGE
#error Continuation IMAGE has no existing native/CPU/Work/component role
#endif
#ifdef PIONEER_Q38_RESIDENT_ADVANCE_IMAGE
#error On-card advance IMAGE cannot contain independent CPU reference products
#endif
// Gates precede the guard: late mixed-role inclusion is also forbidden.
#ifdef PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE
#error On-card link IMAGE cannot contain independent CPU reference products
#endif
#if !defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE) || PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE != 1 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1
#error Complete-layer ORIGINAL reference requires the separate exact CPU observer role
#endif
#if defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT) || defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_DECODER_RUNTIME) || defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || defined(PIONEER_Q38_RESIDENT_LAYER_OWNER) || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT) || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE)
#error Original complete-layer CPU observer never enters a device/native/Work role
#endif
#ifndef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE_H
#define PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE_H
#include "PioneerQ38NextGdnOutputValidationReference.h" // generic independent IEEE/half/Q8 DATA helpers ONLY
#include <array>
#include <vector>
#include "PioneerQ38ResidentLayerComparison.h"
// CPU-only reference PRODUCTS. Compute from ORIGINAL CPU embedding/GDN
// references BEFORE any accelerator FD. Never provide these to a native
// controller, byte endpoint, initialization, upload, owner or Work mode.
// The primary pipeline predicts ideal libm SiLU. A SECOND explicitly named
// reference predicts the frozen device polynomial after bounding every
// activation against libm. Their reports/fingerprints remain distinct.
// Real model instruction/readback comparison is required,never assumed.
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
// or card results. The frozen direct-SrcB peer explicitly FORBIDS GROUP32:
// apply affine/accumulation to each16-lane panel,not paired32-lane dots.
// Every integer dot is exact. Host IEEE/libm
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
  for(unsigned b=0;b<blocks;b++)for(unsigned panel=0;panel<16;panel++){
   const unsigned group=panel/2;
   int total=0;for(unsigned k=0;k<16;k++)total+=quant[b*256+panel*16+k];
   for(unsigned row=0;row<16;row++){
    const auto* p=bytes.data()+size_t(row)*rowBytes+b*144;const auto* s=p+4;
    uint16_t dh=uint16_t(p[0]|(unsigned(p[1])<<8)),mh=uint16_t(p[2]|(unsigned(p[3])<<8));
    if((dh&0x7c00)==0x7c00 || (mh&0x7c00)==0x7c00)return false;
    unsigned d=group<4?s[group]&63:(s[group+4]&15)|((s[group-4]>>6)<<4);
    unsigned m=group<4?s[group+4]&63:(s[group+4]>>4)|((s[group]>>6)<<4);
    int dot=0;for(unsigned k=0;k<16;k++){
     unsigned packed=p[16+(group/2)*32+(panel%2)*16+k],q=(packed>>((group%2)*4))&15;dot+=int(q)*quant[b*256+panel*16+k];
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
// Independent host description of the frozen degree8 SiLU contract.
// No production shader/math helper, software divider, native quantizer or
// observed output is used. Host IEEE division/ldexp/floor predict the
// specified rounded operations; the ideal libm pipeline stays separate.
inline bool SiluInstructionReference(uint32_t gate,uint32_t up,uint32_t& output){
 const unsigned g=gate&0x7fffffff,u=up&0x7fffffff;
 if(std::fegetround()!=FE_TONEAREST || (g && (g<0x2b800000 || g>0x42000000))
  || (u && (u<0x2b800000 || u>0x49800000)))return false;
 float x=Float(gate),argument=-x,t=std::fabs(argument)*1.4426950408889634f;
 int power=int(std::floor(double(t)+.5));if(std::signbit(argument))power=-power;
 float n=float(power),high=n*.693359375f,low=n*(-.00021219444005469058f);
 float reduced=(argument-high)-low;
 const float coefficients[]={1.f,1.f,.5f,1.f/6.f,1.f/24.f,1.f/120.f,1.f/720.f,1.f/5040.f,1.f/40320.f};
 float polynomial=coefficients[8];
 for(int i=7;i>=0;i--){float product=polynomial*reduced;polynomial=product+coefficients[i];}
 float exponential=std::ldexp(polynomial,power),denominator=1.f+exponential;
 float silu=std::copysign(std::fabs(x)/denominator,x),value=silu*Float(up);
 if(!Domain(Bits(value)) || std::fegetround()!=FE_TONEAREST)return false;
 output=Bits(value);return true;
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
// A SECOND named reference,not a replacement for the independent libm
// reference or its strict comparison. Both finish before any native FD.
// Only the activation policy differs; its per-value error MUST satisfy the
// already-qualified 3e-6 relative SiLU bound against libm before publication.
template<class Source>bool PioneerQ38ResidentLayerInstructionReference(Source& source,
 const PioneerQ38ResidentLayerReferenceProducts& ideal,PioneerQ38ResidentLayerReferenceProducts* output){
 using namespace PioneerQ38ResidentLayerReferenceDetail;
 const uintptr_t in=reinterpret_cast<uintptr_t>(&ideal),out=reinterpret_cast<uintptr_t>(output);
 if(!output || (out&3) || in>UINTPTR_MAX-sizeof(ideal) || out>UINTPTR_MAX-sizeof(*output)
  || (in<out+sizeof(*output) && out<in+sizeof(ideal)) || !source.Ready() || std::fegetround()!=FE_TONEAREST)return false;
 auto stage=std::make_unique<PioneerQ38ResidentLayerReferenceProducts>(ideal);
 for(unsigned i=0;i<17408;i++){
  if(!SiluInstructionReference(ideal.gate[i],ideal.up[i],stage->activated[i]) || !Domain(ideal.activated[i]))return false;
  double expected=Float(ideal.activated[i]),actual=Float(stage->activated[i]);
  if(std::fabs(actual-expected)>3e-6*std::fabs(expected))return false;
 }
 if(!MatrixK(source,2,stage->activated,stage->down))return false;
 for(unsigned i=0;i<5120;i++){
  if(!Domain(ideal.residual[i]))return false;
  float value=Float(ideal.residual[i])+Float(stage->down[i]);if(!Domain(Bits(value)))return false;stage->finalHidden[i]=Bits(value);
 }
 if(!source.Ready() || std::fegetround()!=FE_TONEAREST)return false;
 std::memcpy(output,stage.get(),sizeof(*output));return true;
}
// Post-return measurement of NEW fixed QLV1 bytes against references that
// ALREADY existed before the accelerator context. No actual card value
// seeds/recomputes an expected result. Strict projection/final bit equality
// is deliberately distinct from the previously qualified GDN error bounds.
// A successfully produced report is NOT a passing model comparison:inspect
// report.passed. Ideal libm vs device-polynomial rounding can fail it and
// must be diagnosed,not accepted by fitting a tolerance to observed values.
static inline bool PioneerQ38CompareResidentLayer(const uint32_t* values,unsigned words,
 const std::vector<uint32_t>& convolution,const std::vector<uint32_t>& gated,
 const std::vector<uint32_t>& recurrent,const PioneerQ38ResidentLayerReferenceProducts& expected,
 PioneerQ38ResidentLayerComparison* output){
 using namespace PioneerQ38ResidentLayerReferenceDetail;
 using C=PioneerQ38ResidentLayerComparison;
 if(!values || !output || words!=C::Words || convolution.size()!=40960 || gated.size()!=6144 || recurrent.size()!=96*8578
  || std::fegetround()!=FE_TONEAREST)return false;
 uintptr_t in=reinterpret_cast<uintptr_t>(values),out=reinterpret_cast<uintptr_t>(output);
 if((in&3) || (out&(alignof(C)-1)) || in>UINTPTR_MAX-uint64_t(words)*4 || out>UINTPTR_MAX-sizeof(C)
  || (in<out+sizeof(C) && out<in+uint64_t(words)*4))return false;
 for(auto v:convolution)if((v&0x7fffffff)>=0x7f800000)return false;
 for(auto v:gated)if((v&0x7fffffff)>=0x7f800000)return false;
 for(unsigned half=0;half<96;half++)for(unsigned i=0;i<8192;i++)if((recurrent[half*8578+322+i]&0x7fffffff)>=0x7f800000)return false;
 for(unsigned i=0;i<5120;i++)if((expected.projection[i]&0x7fffffff)>=0x7f800000 || (expected.finalHidden[i]&0x7fffffff)>=0x7f800000)return false;
 C r;double error2=0,reference2=0;
 for(unsigned i=0;i<words;i++)if((values[i]&0x7fffffff)>=0x7f800000)r.nonfinite++;
 for(unsigned i=0;i<10240;i++){
  double e=Float(convolution[i]),a=Float(values[C::Conv+i]);
  if(!std::isfinite(a) || std::fabs(a-e)>3e-6*(1+std::fabs(e)))r.convErrors++;
 }
 for(unsigned i=0;i<30720;i++)if(values[C::History+i]!=convolution[10240+i])r.historyErrors++;
 for(unsigned i=0;i<6144;i++){
  double e=Float(gated[i]),a=Float(values[C::Gated+i]),d=std::fabs(a-e);
  if(!std::isfinite(a) || d>3e-6*(1+std::fabs(e))
   || (std::fabs(e)>=double(Float(0x00800000))?d>3e-6*std::fabs(e):a!=0))r.gatedErrors++;
 }
 for(unsigned half=0;half<96;half++)for(unsigned i=0;i<8192;i++){
  unsigned at=half*8192+i;double e=Float(recurrent[half*8578+322+i]),a=Float(values[C::Next+at]);
  if(values[C::Current+at])r.currentErrors++;
  if(!std::isfinite(a) || std::fabs(a-e)>3e-6*(1+std::fabs(e)))r.nextErrors++;
 }
 for(unsigned i=0;i<5120;i++){
  double e=Float(expected.projection[i]),a=Float(values[C::Projection+i]);
  if(values[C::Projection+i]!=expected.projection[i])r.projectionDifferences++;
  if(std::isfinite(a))r.projectionMaxAbsolute=std::fmax(r.projectionMaxAbsolute,std::fabs(a-e));
  e=Float(expected.finalHidden[i]);a=Float(values[C::Final+i]);
  if(values[C::Final+i]!=expected.finalHidden[i])r.finalDifferences++;
  if(std::isfinite(a)){double d=a-e;r.finalMaxAbsolute=std::fmax(r.finalMaxAbsolute,std::fabs(d));error2+=d*d;reference2+=e*e;}
 }
 r.checked=words;r.finalRelativeL2=std::sqrt(error2/(reference2+1e-300));
 r.passed=!(r.nonfinite || r.convErrors || r.historyErrors || r.gatedErrors || r.projectionDifferences
  || r.currentErrors || r.nextErrors || r.finalDifferences);
 if(std::fegetround()!=FE_TONEAREST)return false;
 *output=r;return true; // report ONLY;no data uploaded,reference products exposed or native permission
}
#endif
