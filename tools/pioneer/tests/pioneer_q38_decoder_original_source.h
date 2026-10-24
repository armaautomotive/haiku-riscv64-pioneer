/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DECODER_ORIGINAL_SOURCE_H
#define PIONEER_Q38_DECODER_ORIGINAL_SOURCE_H
#include "pioneer_q38_projection_execution_machine.h"
#include <memory>
// TEST original-byte endpoints and independent CPU OBSERVER only. The
// implementation is compiled SEPARATELY without native adapter/execution
// or Work flags. The canonical mutable-DATA interlock grants NO device IO.
// Controllers receive only the existing weight-byte Read interface,never
// these precomputed numeric references,model/initialization/launch authority.
struct DecoderOriginalSource:ProjectionExecutionLearnedBytes {
 virtual bool Prepare(unsigned token)=0;
 virtual bool SealBeforeNativeContext()=0;
 virtual const uint8_t* EmbeddingTile()const=0;
 virtual const uint32_t* LearnedNorm()const=0;
 virtual bool CheckEmbeddingRms(const std::vector<uint32_t>& workspace)const=0;
 virtual bool CheckProjection(unsigned role,const uint32_t* values)const=0;
 virtual bool CheckConvolution(const uint32_t* values)const=0;
 virtual uint64_t ReferenceReads()const=0;
 virtual uint64_t ProjectionReads()const=0;
 virtual uint64_t ConvolutionReads()const=0;
 // Optional separate GDN-input observer. References never enter controllers.
 virtual bool CheckGdnInput(unsigned,const uint32_t*)const{return false;}
 virtual uint64_t GdnInputReads()const{return 0;}
 virtual bool CheckGdnRecurrent(unsigned,const uint32_t*,const uint32_t*,const uint32_t*)const{return false;}
 virtual const uint32_t* GdnLearnedNorm()const{return nullptr;} // ORIGINAL learned bytes only,not numeric reference products
 virtual bool CheckGdnGatedNorm(unsigned,const uint32_t*)const{return false;}
};
std::unique_ptr<DecoderOriginalSource> OpenDecoderOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenGdnInputOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenGdnRecurrentOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenGdnGatedNormOriginalSource(const char* path);
#endif
