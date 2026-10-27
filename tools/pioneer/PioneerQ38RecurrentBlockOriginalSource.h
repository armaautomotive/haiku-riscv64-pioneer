/* SPDX-License-Identifier: MIT */
// Repeat interlocks even after late role changes.
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_DECODER_RUNTIME) || defined(PIONEER_Q38_EMBEDDING_NATIVE) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE)
#error Recurrent original CPU source must stay outside native,kernel,image and Work roles,including defined0
#endif
#ifndef PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_SOURCE_H
#define PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_SOURCE_H
#include "PioneerQ38RecurrentOriginalWeightBytes.h"
#include "PioneerQ38RecurrentBlockComparison.h"
#include <memory>
// Private independent CPU products are never exposed. Only authenticated
// raw learned bytes and post-return comparisons cross this interface.
// Prepare+seal BEFORE creating any native/accelerator context.
struct PioneerRecurrentOriginalExtent {
 uint32_t bank=7,route=0,address=0,bytes=0,pages=0;
}; // immutable RAW byte geometry ONLY,never expected numerical products
class PioneerQ38RecurrentBlockOriginalSource:public RecurrentBlockRawLearnedBytes {
public:
 virtual bool Prepare()=0;virtual bool SealBeforeNativeContext()=0;
 virtual bool CompareStage(PioneerRecurrentOriginalStage,const uint32_t*,unsigned,PioneerRecurrentOriginalComparison*)const=0;
 virtual uint64_t ReferenceReads()const=0;virtual uint64_t ReferenceFingerprint()const=0;
 virtual uint64_t Bytes()const=0;virtual unsigned Pages()const=0;virtual unsigned Spans()const=0;
 virtual bool Extent(unsigned,PioneerRecurrentOriginalExtent&)const=0;
};
std::unique_ptr<PioneerQ38RecurrentBlockOriginalSource> OpenPioneerQ38RecurrentBlockOriginalSource(const char* path=nullptr);
#endif
