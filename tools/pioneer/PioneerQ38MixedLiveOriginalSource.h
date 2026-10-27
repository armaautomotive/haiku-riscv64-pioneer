/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE
#error Recurrent controller cannot contain CPU reference products
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_DECODER_RUNTIME) \
 || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE)
#error Original dense CPU source interface is validation-user-space only, not native/image/kernel/Work, including defined0
#endif
#ifndef PIONEER_Q38_MIXED_LIVE_ORIGINAL_SOURCE_H
#define PIONEER_Q38_MIXED_LIVE_ORIGINAL_SOURCE_H
#include "PioneerQ38MixedLiveComparison.h"
#include <memory>
class PioneerQwen38Gguf;
// Fixed original FILE/weight-byte source and post-return CPU observer only.
// Implementation is a SEPARATE exact CPU-reference TU. No expected numeric
// products, input vectors, native controller, source/owner/ready proof or
// accelerator FD can be supplied through this interface. Ready concerns
// only this authenticated file/reference, NEVER hardware permission.
class PioneerQ38MixedLiveOriginalSource {
public:
 virtual ~PioneerQ38MixedLiveOriginalSource()=default;
 virtual bool Prepare()=0;virtual bool SealBeforeNativeContext()=0;
 virtual bool Ready()const=0;
 virtual const PioneerQwen38Gguf* SealedModel()const=0;
 virtual bool ReadWeightPage(unsigned part,unsigned page,uint8_t*,unsigned capacity)=0;
 virtual bool CompareFinal(const uint32_t*,unsigned,PioneerQ38MixedLiveReferenceComparison*)const=0;
 virtual bool CompareStage(PioneerQ38MixedLiveReferenceStage,const uint32_t*,unsigned,PioneerQ38MixedLiveReferenceComparison*)const=0;
 virtual uint64_t ReferenceReads()const=0;virtual uint64_t ReferenceFingerprint()const=0;
};
// Haiku opens only the fixed authenticated model path. The path argument
// is a host-test adaptation only and is refused on Haiku unless null/fixed.
std::unique_ptr<PioneerQ38MixedLiveOriginalSource> OpenPioneerQ38MixedLiveOriginalSource(const char* path=nullptr);
#endif
