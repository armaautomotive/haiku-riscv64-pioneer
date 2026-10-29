/* SPDX-License-Identifier: MIT */
// TEST-only opaque byte interface. Never a native/Work/source-ready grant.
#if !defined(PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE) || PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE != 1
#error Explicit offline embedding byte-bridge test role required
#endif
#if defined(_KERNEL_MODE) && (!defined(PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1)
#error Real kernel/driver code may not import this host-test byte transport
#endif
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#error Device/Work code may not import the offline raw byte bridge
#endif
#ifndef PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE_H
#define PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE_H
#include <cstdint>
class Q38EmbeddingRawTestBridge {
 struct Source;Source* source;
public:
 Q38EmbeddingRawTestBridge();~Q38EmbeddingRawTestBridge();
 Q38EmbeddingRawTestBridge(const Q38EmbeddingRawTestBridge&)=delete;
 Q38EmbeddingRawTestBridge& operator=(const Q38EmbeddingRawTestBridge&)=delete;
 bool Open(const char*);bool Ready()const;void Revoke();
 bool Read(unsigned route,uint32_t address,uint8_t* bytes,unsigned count);
 bool OriginalBlock(unsigned token,unsigned block,uint8_t* bytes,unsigned count);
}; // RAW bytes ONLY,no processor state,computed product or readiness setter
#endif
