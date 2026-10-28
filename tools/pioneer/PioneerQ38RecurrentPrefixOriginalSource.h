/* SPDX-License-Identifier: MIT */
// CPU-only original-file validation boundary. No reference vectors escape.
#if defined(PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE) || defined(PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST)
#error Prefix original source must remain outside image,native and client roles,including defined0
#endif
#include "PioneerQ38RecurrentBlockOriginalSource.h"
#ifndef PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_SOURCE_H
#define PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_SOURCE_H
// Same authenticated open file supplies independently decoded CPU reference
// and tiled RAW learned bytes for layers0..2. Prepare/seal before any context.
// CompareStage accepts returned observations, NEVER yields an expected input.
class PioneerQ38RecurrentPrefixOriginalSource:public RecurrentBlockRawLearnedBytes {
public:
 virtual bool Prepare()=0;virtual bool SealBeforeNativeContext()=0;
 virtual bool CompareStage(unsigned,PioneerRecurrentOriginalStage,const uint32_t*,unsigned,PioneerRecurrentOriginalComparison*)const=0;
 virtual uint64_t ReferenceReads()const=0;virtual uint64_t ReferenceFingerprint(unsigned)const=0;
 virtual uint64_t Bytes()const=0;virtual unsigned Pages()const=0;virtual unsigned Spans()const=0;
 virtual bool Extent(unsigned,PioneerRecurrentOriginalExtent&)const=0;
};
std::unique_ptr<PioneerQ38RecurrentPrefixOriginalSource> OpenPioneerQ38RecurrentPrefixOriginalSource(const char* path=nullptr);
#endif
