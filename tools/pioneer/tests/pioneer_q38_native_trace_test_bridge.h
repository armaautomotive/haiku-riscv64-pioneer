/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_NATIVE_TRACE_TEST_BRIDGE) || PIONEER_Q38_NATIVE_TRACE_TEST_BRIDGE != 1
#error Explicit native trace TEST interface required
#endif
#if defined(_KERNEL_MODE) && (!defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1)
#error Trace interface is only for the explicitly emulated native TEST fixture
#endif
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Trace TEST interface is not Work,image or CPU-oracle authority
#endif
#ifndef PIONEER_Q38_NATIVE_TRACE_TEST_BRIDGE_H
#define PIONEER_Q38_NATIVE_TRACE_TEST_BRIDGE_H
#include <cstdint>
// Const DATA observations only. No owner,processor,memory endpoint,expected
// CPU value source or accelerator callback is accepted or returned.
class Q38NativeTraceTest {
 struct Impl;Impl* impl;
public:
 static constexpr uint32_t TokenMagic=0x31435451;
 Q38NativeTraceTest();~Q38NativeTraceTest();
 Q38NativeTraceTest(const Q38NativeTraceTest&)=delete;
 Q38NativeTraceTest& operator=(const Q38NativeTraceTest&)=delete;
 bool Embedding(unsigned,unsigned,const uint32_t*,unsigned);
 bool Layer(unsigned,unsigned,const uint32_t*,unsigned);
 bool Vocabulary(unsigned,unsigned,const uint32_t*,unsigned);
 bool Commit(unsigned,const uint32_t*,unsigned);
 bool Complete()const;bool SavedComplete()const;unsigned Embeddings()const;unsigned Layers(unsigned)const;
 bool Save(const char*);bool SaveFirstLayerDiagnostic(const char*);
 bool SnapshotFirstLayerDiagnostic(const char*); // deliberately incomplete,does NOT seal/stop the run
}; // Save still requires the caller's separate genuine terminal verification
#endif
