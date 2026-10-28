/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#error Offline authenticated RAW bytes are not driver/image/Work authority
#endif
#ifndef PIONEER_Q38_WHOLE_ORIGINAL_BYTES_H
#define PIONEER_Q38_WHOLE_ORIGINAL_BYTES_H
#include <cstdint>
// Separate TEST-only source TU. Only losslessly permuted original learned
// bytes/padding; NO CPU model products, mutable state, native IO or ownership.
class WholeModelOriginalBytes {
 struct Original;Original* original;
public:
 WholeModelOriginalBytes();~WholeModelOriginalBytes();
 WholeModelOriginalBytes(const WholeModelOriginalBytes&)=delete;
 WholeModelOriginalBytes& operator=(const WholeModelOriginalBytes&)=delete;
 bool Open(const char*);bool Ready()const;bool PrepareFork();
 // One-way TEST byte-source revocation. No descriptor reopen/recovery or
 // device cleanup is implied; cached learned bytes become unreadable too.
 void Revoke();
 // Readiness/bounds query ONLY. A future fabric must classify canonical
 // learned coordinates independently and FAIL on any Read refusal; NEVER
 // fall back to synthetic bytes or CPU products when Contains/Ready fails.
 bool Contains(unsigned route,uint32_t address,unsigned bytes)const;
 bool Read(unsigned route,uint32_t address,uint8_t* out,unsigned bytes);
 // Narrow byte-source self-check only, not a model computation/reference.
 bool CheckSamples(unsigned& parts,unsigned& pages,uint64_t& padding,unsigned& mutableRefusals);
};
#endif
