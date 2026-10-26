/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DECODER_ORIGINAL_SOURCE_H
#define PIONEER_Q38_DECODER_ORIGINAL_SOURCE_H
#include "PioneerQ38OriginalWeightBytes.h"
#include <vector>
#include <memory>
struct tt_q38_resident_output_validation;
struct PioneerQ38ResidentLayerComparison;
class PioneerQwen38Gguf;
class PioneerQwen38PackedPlan;
// Original-byte endpoints and independent CPU OBSERVER only. The
// implementation is compiled SEPARATELY without native adapter/execution
// or Work flags. The canonical mutable-DATA interlock grants NO device IO.
// Controllers receive only the existing weight-byte Read interface,never
// these precomputed numeric references,model/initialization/launch authority.
struct DecoderOriginalSource:ProjectionExecutionLearnedBytes {
 virtual bool Prepare(unsigned token)=0;
 virtual bool SealBeforeNativeContext()=0;
 // Read-only metadata and original weight pages from the SAME sealed,
 // fully authenticated file description used by the CPU observer. No
 // reopen/CPU numeric input/FD/launch authority is exposed to the client.
 virtual const PioneerQwen38Gguf* SealedModel()const{return nullptr;}
 virtual bool ReadWeightPage(const PioneerQwen38PackedPlan&,unsigned,unsigned,uint8_t*,unsigned){return false;}
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
 virtual uint64_t GdnNormReads()const{return 0;}
 virtual bool GdnOutputWeightsReady()const{return false;} // TEST learned byte endpoints only
 virtual uint64_t GdnOutputReads()const{return 0;}
 // Post-return CPU observer only. No references/products are exposed as
 // input to the native controller or to its instruction-machine endpoints.
 virtual bool CheckResidentOutput(const tt_q38_resident_output_validation&){return false;}
 virtual bool ResidualRmsWeightsReady()const{return false;}
 virtual uint64_t ResidualRmsReads()const{return 0;}
 virtual bool CheckResidualRms(const std::vector<uint32_t>&,const std::vector<uint32_t>&)const{return false;}
 virtual bool FfnWeightsReady()const{return false;}
 virtual uint64_t FfnReads()const{return 0;}
 virtual bool CheckFfnResidual(const std::vector<uint32_t>&,const std::vector<uint32_t>&)const{return false;}
 virtual bool NextRmsWeightsReady()const{return false;}
 virtual uint64_t NextRmsReads()const{return 0;}
 virtual bool CheckNextRms(const std::vector<uint32_t>&,const std::vector<uint32_t>&)const{return false;}
 virtual bool NextProjectionWeightsReady()const{return false;}
 virtual uint64_t NextProjectionReads()const{return 0;}
 virtual bool CheckNextProjections(const std::vector<uint32_t>&,const std::vector<uint32_t>&){return false;}
 virtual bool NextConvWeightsReady()const{return false;}
 virtual uint64_t NextConvReads()const{return 0;}
 virtual bool CheckNextConv(const std::vector<uint32_t>&,const std::vector<uint32_t>&){return false;}
 virtual bool NextGdnWeightsReady()const{return false;}
 virtual uint64_t NextGdnReads()const{return 0;}
 virtual bool CheckNextGdn(unsigned,const std::vector<uint32_t>&,const uint32_t*,const uint32_t*,const uint32_t*,const uint32_t*)const{return false;}
 virtual bool NextGdnOutputWeightsReady()const{return false;}
 virtual uint64_t NextGdnOutputReads()const{return 0;}
 virtual bool CheckNextGdnOutput(const std::vector<uint32_t>&,const std::vector<uint32_t>&){return false;}
 virtual bool NextResidualRmsWeightsReady()const{return false;}
 virtual uint64_t NextResidualRmsReads()const{return 0;}
 virtual bool CheckNextResidualRms(const std::vector<uint32_t>&,const std::vector<uint32_t>&)const{return false;}
 virtual bool NextFfnWeightsReady()const{return false;}
 virtual uint64_t NextFfnReads()const{return 0;}
 virtual bool CheckNextFfnResidual(const std::vector<uint32_t>&,const std::vector<uint32_t>&)const{return false;}
 // Append new metadata methods: preserve every existing observer vtable
 // slot. Private CPU products never become native inputs or Work fallback.
 virtual bool ResidentLayerReferenceReady()const{return false;}
 virtual uint64_t ResidentLayerReferenceFingerprint()const{return 0;}
 // true means a report was produced;caller MUST separately inspect its
 // passed field. Neither result grants native execution/cleanup authority.
 virtual bool CompareResidentLayer(const uint32_t*,unsigned,PioneerQ38ResidentLayerComparison*)const{return false;}
 virtual bool ResidentLayerStageFingerprint(unsigned,uint64_t&)const{return false;}
 // Explicit SECOND observer for the frozen SiLU instruction policy.
 // Never aliases/relabels the ideal-libm comparison or native permission.
 virtual uint64_t ResidentLayerInstructionReferenceFingerprint()const{return 0;}
 virtual bool CompareResidentLayerInstruction(const uint32_t*,unsigned,PioneerQ38ResidentLayerComparison*)const{return false;}
 virtual bool ResidentLayerInstructionStageFingerprint(unsigned,uint64_t&)const{return false;}
};
std::unique_ptr<DecoderOriginalSource> OpenDecoderOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenGdnInputOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenGdnRecurrentOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenGdnGatedNormOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenGdnGatedNativeOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenGdnOutputOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenResidualRmsOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenFfnResidualOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenNextRmsOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenNextProjectionOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenNextConvOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenNextGdnOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenNextGdnOutputOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenNextResidualRmsOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenNextFfnResidualOriginalSource(const char* path);
// Fixed PUBLIC model only;caller prepares+seals BEFORE any accelerator FD.
std::unique_ptr<DecoderOriginalSource> OpenResidentOutputOriginalSource();
std::unique_ptr<DecoderOriginalSource> OpenResidentLayerOriginalSource(const char* path);
std::unique_ptr<DecoderOriginalSource> OpenResidentLayerOriginalSource();
#endif
