/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE)
#error SAME native62 next-layer output never includes CPU reference code
#endif
#if defined(PIONEER_Q38_NEXT_GDN_NATIVE)
#error SAME native next-layer GDN never includes CPU reference code
#endif
#if defined(PIONEER_Q38_NEXT_CONV_NATIVE)
#error SAME native next convolution never includes CPU reference code
#endif
#if defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE)
#error SAME native next-layer projections never include CPU reference code
#endif
#if defined(PIONEER_Q38_PROJECTION_NEXT_ROOT)
#error QPS2 device images never include independent CPU reference code
#endif
#if defined(PIONEER_Q38_NEXT_RMS_NATIVE)
#error SAME native next-layer RMS never includes independent CPU reference code
#endif
#if defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE)
#error SAME native FFN execution must NEVER include independent CPU reference code
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
#error Native residual RMS never compiles CPU reference observers
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
#error Native output DEBUG never compiles CPU reference observers
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
#error Native output execution never compiles CPU reference observers
#endif
#ifndef PIONEER_Q38_PROJECTION_VALIDATION_REFERENCE_H
#define PIONEER_Q38_PROJECTION_VALIDATION_REFERENCE_H
#ifdef PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE
#error Offline gated input image never compiles CPU references
#endif
#if defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_GATED_NATIVE)
#error Recurrent native DEBUG never compiles CPU references
#endif
#if (defined(PIONEER_Q38_PROJECTION_VALIDATION_CLIENT)+defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT)+defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT)+defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT)+defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT)+defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT)) != 1 || (defined(PIONEER_Q38_PROJECTION_VALIDATION_CLIENT) && PIONEER_Q38_PROJECTION_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT) && PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT) && PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT) && PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT) && PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT) && PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT != 1) || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1 || !defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE) || PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE != 1 || defined(PIONEER_Q38_EMBEDDING_CLIENT) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT) || defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(_KERNEL_MODE)
#error Independent ORIGINAL first-layer projection reference is source-bound DEBUG ONLY,never Work/native/kernel
#endif
#include "PioneerQwen38Gguf.h"
#include "PioneerQwen38Panel.h"
#include "PioneerQwen38CohortHost.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionResultLayout.h"
#include <vector>
// ORIGINAL packed Q8_0 rows + independently computed CPU RMS input,
// BEFORE GPU FD. Never consults card/tiled-output/proof/ready data and never
// uploads these numeric products. FP32 accumulation order matches the
// independently qualified panel arithmetic,not a library/device callback.
template<class Source>bool PioneerQ38ProjectionReference(Source& source,unsigned role,const uint32_t* normalized,unsigned inputs,
 uint32_t* output,unsigned capacity){
 unsigned rows=PioneerQ38ProjectionResultRows(role);
 if(!normalized || !output || inputs!=5120 || !rows || capacity!=rows || !source.Ready())return false;
 uintptr_t in=reinterpret_cast<uintptr_t>(normalized),out=reinterpret_cast<uintptr_t>(output);
 if((in&3) || (out&3) || in>UINTPTR_MAX-20480 || out>UINTPTR_MAX-rows*4 || (in<out+rows*4 && out<in+20480))return false;
 PioneerQ38Tensor expected;if(!PioneerQ38Expected(0,role,expected))return false;
 const auto* tensor=source.Model().Find(expected.name.c_str());
 if(!tensor || tensor->type!=8 || tensor->rank!=2 || tensor->inputs!=5120 || tensor->outputs!=rows || tensor->bytes!=uint64_t(rows)*5440)return false;
 unsigned index=unsigned(tensor-source.Model().Tensors().data());
 std::vector<float> x(5120);std::vector<uint32_t> staged(rows);
 for(unsigned i=0;i<5120;i++){
  if((normalized[i]&0x7fffffff)>=0x7f800000)return false;
  x[i]=PioneerAttentionFloat(normalized[i]);
 }
 auto input=std::make_unique<PioneerQ38CohortInput>();
 if(!PioneerQ38CohortQuantize(8,5120,x.data(),*input))return false;
 std::vector<uint8_t> packed(5440*16),tile(576);
 for(unsigned first=0;first<rows;first+=16){
  if(!source.ReadReferencePacked(index,uint64_t(first)*5440,unsigned(packed.size()),packed.data(),unsigned(packed.size())) || !source.Ready())return false;
  float sum[16]={};
  for(unsigned block=0;block<160;block++){
   for(unsigned byte=0;byte<34;byte++)for(unsigned row=0;row<16;row++)tile[byte*16+row]=packed[row*5440+block*34+byte];
   for(unsigned piece=0;piece<2;piece++){
    PioneerQ38Panel panel;if(!PioneerQ38UnpackPanel(8,tile.data(),576,piece,&panel))return false;
    int activation[16],total=0;
    for(unsigned lane=0;lane<16;lane++){
     unsigned at=block*32+piece*16+lane;uint16_t bits=uint16_t(input->activation[at/2]>>(16*(at%2)));
     activation[lane]=(bits&0x8000)?-int(bits&0x3ff):int(bits&0x3ff);total+=activation[lane];
    }
    float scale=PioneerAttentionFloat(input->scales[block]);
    for(unsigned row=0;row<16;row++){
     int dot=0;
     for(unsigned lane=0;lane<16;lane++){
      unsigned at=lane*16+row;uint16_t bits=uint16_t(panel.operand[at/2]>>(16*(at%2)));
      int weight=(bits&0x8000)?-int(bits&0x3ff):int(bits&0x3ff);dot+=weight*activation[lane];
     }
     float product=(PioneerAttentionFloat(panel.coefficient[row])*scale)*float(dot);
     float correction=(PioneerAttentionFloat(panel.minimum[row])*scale)*float(total);
     sum[row]+=product+correction;
    }
   }
  }
  for(unsigned row=0;row<16;row++){
   uint32_t bits=PioneerAttentionBits(sum[row]);if((bits&0x7fffffff)>=0x7f800000)return false;staged[first+row]=bits;
  }
 }
 if(!source.Ready())return false;
 memcpy(output,staged.data(),rows*4);return true; // publish only complete independent reference
}
#endif
