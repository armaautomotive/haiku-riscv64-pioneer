/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RESIDENT_OPERATOR_IMAGE
#error Resident operator IMAGE never contains CPU reference observers
#endif
#ifdef PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE
#error Native66 never contains CPU reference observers
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
#error Native layer1 FFN never contains CPU reference observers
#endif
#ifdef PIONEER_Q38_CONV_SILU_PRECISION_NATIVE
#error Native precision diagnostic never contains CPU reference observers
#endif
#if defined(PIONEER_Q38_SILU_PRECISION_IMAGE) || defined(PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE)
#error Isolated SiLU diagnostic images never include CPU reference observers
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
#error SAME native63 residual RMS never includes CPU reference code
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_IMAGE
#error QRR2 device image never contains a CPU reference observer
#endif
#if !defined(PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE) || PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE != 1 || !defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT) || PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT != 1 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1
#error Exact separate original layer1 output CPU observer required
#endif
#if defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE) || defined(PIONEER_Q38_NEXT_GDN_NATIVE) || defined(PIONEER_Q38_NEXT_CONV_NATIVE) || defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE) || defined(PIONEER_Q38_NEXT_RMS_NATIVE) || defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE) || defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_GDN_GATED_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE)
#error Native ownership and original output CPU observer never coexist
#endif
#if defined(PIONEER_Q38_DECODER_RUNTIME) || defined(PIONEER_Q38_PROJECTION_RESULT_STASH)
#error Runtime/stash native roles never enter the original output CPU observer
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(__FAST_MATH__) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_ROOT) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_IMAGE) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GATHER_IMAGE)
#error Independent output observer never enters kernel/device-image/Work
#endif
// Gates precede include guard; later mixed-role reinclusion is refused.
#ifndef PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE_H
#define PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE_H
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

// DEBUG DATA only. No FD,route,ready flag,owner,upload or execution grant.
// The future client must FIRST validate the returned original GDN values
// against the independent GDN reference,then extract its 6144 gated words.
// Quantization of those validated ACTUAL values is intentional: small valid
// GDN differences can cross Q8 rounding boundaries. These post-return CPU
// products are NEVER uploaded/substituted,or compiled into Work/native.
struct PioneerQ38NextGdnOutputReferenceResult {
 uint32_t operand[3072],scale[192],values[5120];
};
static_assert(sizeof(PioneerQ38NextGdnOutputReferenceResult)==33536,"bounded CPU observer data only");

namespace PioneerQ38NextGdnOutputReferenceDetail {
static inline uint32_t Bits(float f){uint32_t v;std::memcpy(&v,&f,4);return v;}
static inline float Float(uint32_t v){float f;std::memcpy(&f,&v,4);return f;}
// Independent IEEE/libm binary16 conversion,not the card bit converter,
// integer divider,native quantizer,panel decoder or a device callback.
static inline bool HalfBits(float value,uint16_t& half){
 if(!std::isfinite(value) || value<0 || value>=65520.f)return false;
 if(value<0x1p-14f){half=uint16_t(std::nearbyint(double(value)*0x1p24));return true;}
 int exponent;double fraction=std::frexp(double(value),&exponent);
 unsigned mantissa=unsigned(std::nearbyint((fraction*2-1)*1024));
 half=uint16_t((unsigned(exponent+14)<<10)+mantissa);return half<0x7c00;
}
static inline float HalfFloat(uint16_t half){
 unsigned exponent=(half>>10)&31,mantissa=half&1023;
 float f=exponent?std::ldexp(float(1024+mantissa),int(exponent)-25):std::ldexp(float(mantissa),-24);
 return half&0x8000?-f:f;
}
static inline bool Input(const uint32_t* gated,PioneerQ38NextGdnOutputReferenceResult& stage){
 for(unsigned block=0;block<192;block++){
  float maximum=0;
  for(unsigned lane=0;lane<32;lane++){
   uint32_t bits=gated[block*32+lane],magnitude=bits&0x7fffffff;
   if(magnitude>=0x7f800000 || (magnitude && magnitude<0x00800000))return false;
   maximum=std::fmax(maximum,std::fabs(Float(bits)));
  }
  float step=maximum/127.f,inverse=maximum?1.f/step:0.f;uint16_t half;
  if(!std::isfinite(inverse) || !HalfBits(step,half))return false;
  float scale=HalfFloat(half);
  if(scale!=0 && (scale<0x1p-32f || scale>65536.f))return false;
  stage.scale[block]=Bits(scale);
  for(unsigned lane=0;lane<32;lane++){
   float rounded=std::round(Float(gated[block*32+lane])*inverse);
   if(!std::isfinite(rounded) || rounded<-127 || rounded>127)return false;
   int value=int(rounded);unsigned at=block*32+lane;
   unsigned native=0x4000U|(value<0?0x8000U|unsigned(-value):unsigned(value));
   if(!(at&1))stage.operand[at/2]=native;else stage.operand[at/2]|=native<<16;
  }
 }
 return true;
}
}

// Source is the SAME fully authenticated public-model source,with its
// CPU-only original-packed-row reader. Staged publication: any late source,
// scale,FP environment or arithmetic failure leaves caller output untouched.
template<class Source>bool PioneerQ38NextGdnOutputReference(Source& source,const uint32_t* gated,unsigned words,
 PioneerQ38NextGdnOutputReferenceResult* output){
 using namespace PioneerQ38NextGdnOutputReferenceDetail;
 static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,"IEEE binary32 observer");
 if(!gated || words!=6144 || !output || std::fegetround()!=FE_TONEAREST || !source.Ready())return false;
 uintptr_t in=reinterpret_cast<uintptr_t>(gated),out=reinterpret_cast<uintptr_t>(output);
 if((in&3) || (out&3) || in>UINTPTR_MAX-24576 || out>UINTPTR_MAX-sizeof(*output)
  || (in<out+sizeof(*output) && out<in+24576))return false;
 const auto* found=source.Model().Find("blk.1.ssm_out.weight");
 if(!found || found->type!=8 || found->rank!=2 || found->inputs!=6144 || found->outputs!=5120
  || found->bytes!=UINT64_C(5120)*6528)return false;
 const auto tensor=*found;const auto& tensors=source.Model().Tensors();unsigned index=0;
 while(index<tensors.size() && &tensors[index]!=found)index++;
 if(index==tensors.size())return false;
 auto current=[&](){
  if(!source.Ready() || index>=source.Model().Tensors().size() || std::fegetround()!=FE_TONEAREST)return false;
  const auto& t=source.Model().Tensors()[index];
  return t.name==tensor.name && t.type==tensor.type && t.rank==tensor.rank && t.inputs==tensor.inputs
   && t.outputs==tensor.outputs && t.bytes==tensor.bytes && t.offset==tensor.offset;
 };
 std::unique_ptr<PioneerQ38NextGdnOutputReferenceResult> stage(new(std::nothrow) PioneerQ38NextGdnOutputReferenceResult{});
 std::unique_ptr<uint8_t[]> packed(new(std::nothrow) uint8_t[16*6528]);
 if(!stage || !packed || !Input(gated,*stage))return false;
 for(unsigned first=0;first<5120;first+=16){
  if(!current() || !source.ReadReferencePacked(index,uint64_t(first)*6528,16*6528,packed.get(),16*6528) || !current())return false;
  float sum[16]={};
  for(unsigned block=0;block<192;block++){
   float scale=Float(stage->scale[block]),weights[16];
   for(unsigned row=0;row<16;row++){
    const auto* bytes=packed.get()+row*6528+block*34;uint16_t half=uint16_t(bytes[0]|(unsigned(bytes[1])<<8));
    if((half&0x7c00)==0x7c00)return false;
    weights[row]=HalfFloat(half);
   }
   for(unsigned piece=0;piece<2;piece++)for(unsigned row=0;row<16;row++){
    int dot=0;
    for(unsigned lane=0;lane<16;lane++){
     unsigned at=block*32+piece*16+lane;
     uint16_t operand=uint16_t(stage->operand[at/2]>>((at%2)*16));
     int activation=operand&0x8000?-int(operand&1023):int(operand&1023);
     unsigned byte=packed[row*6528+block*34+2+piece*16+lane];
     int weight=int(byte)-int((byte&128)*2);dot+=weight*activation;
    }
    float product=(weights[row]*scale)*float(dot);sum[row]+=product;
    if(!std::isfinite(sum[row]))return false;
   }
  }
  for(unsigned row=0;row<16;row++)stage->values[first+row]=Bits(sum[row]);
 }
 if(!current())return false;
 std::memcpy(output,stage.get(),sizeof(*output));return true;
}
#endif
