/* SPDX-License-Identifier: MIT */
// Neutral USER validation boundary;no expected numerical vectors escape.
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE) || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RECURRENT_PREFIX_EMIT_CONTEXT_BODY) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE) || defined(PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) \
 || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_GDN_MUTABLE_STATE) \
 || defined(PIONEER_Q38_DECODER_RUNTIME) || defined(PIONEER_Q38_EMBEDDING_NATIVE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Prefix validation source interface cannot enter kernel,native,image or Work roles,including defined0
#endif
#ifndef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_SOURCE_H
#define PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_SOURCE_H
#include "PioneerQ38OriginalWeightBytes.h"
#include "PioneerQ38RecurrentBlockComparison.h"
#include <memory>
struct PioneerRecurrentPrefixValidationExtent {
 uint32_t bank=7,route=0,address=0,bytes=0,pages=0;
};
class PioneerQ38RecurrentPrefixValidationSource:public ProjectionExecutionLearnedBytes {
public:
 virtual bool Prepare()=0;virtual bool SealBeforeNativeContext()=0;
 virtual bool Contains(unsigned,uint32_t,unsigned)const=0;
 virtual bool CompareFinal(const uint32_t*,unsigned,PioneerRecurrentOriginalComparison*)const=0;
 virtual uint64_t ReferenceReads()const=0;virtual uint64_t ReferenceFingerprint(unsigned)const=0;
 virtual uint64_t Bytes()const=0;virtual unsigned Pages()const=0;virtual unsigned Spans()const=0;
 virtual bool Extent(unsigned,PioneerRecurrentPrefixValidationExtent&)const=0;
};
std::unique_ptr<PioneerQ38RecurrentPrefixValidationSource> OpenPioneerQ38RecurrentPrefixValidationSource(const char* path=nullptr);
// Concrete adapter and independent CPU oracle compile in UNFLAGGED TUs,
// outside the selected client/native role. No device operations here.
#endif
