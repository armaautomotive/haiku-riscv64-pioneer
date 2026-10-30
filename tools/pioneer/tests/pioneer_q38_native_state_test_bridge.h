/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE) || PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE != 1
#error Explicit native state DATA TEST interface required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)
#include "pioneer_q38_token_native_full_end_role.h"
#if !defined(PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE != 1
#error Explicit native state snapshot TEST required
#endif
#endif
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#error State DATA TEST interface is not image,CPU-oracle or Work authority
#endif
#ifndef PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE_H
#define PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE_H
#include <cstdint>
// Output-only const DATA;no processor/memory/owner callbacks,expected CPU
// products,ready setters or mutable pointers are accepted or returned.
class Q38NativeStateTest {
 struct Impl;Impl* impl;
public:
 Q38NativeStateTest();~Q38NativeStateTest();
 Q38NativeStateTest(const Q38NativeStateTest&)=delete;
 Q38NativeStateTest& operator=(const Q38NativeStateTest&)=delete;
 bool Record(const char*,unsigned completed,unsigned layer,unsigned kind,const uint32_t*,unsigned);
 bool Finish(const char*); // caller must separately prove genuine full generation+END
};
#endif
