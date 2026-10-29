/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE) || PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE != 1
#error Explicit offline native peer byte-bridge role required
#endif
#if defined(_KERNEL_MODE) && (!defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1)
#error A real kernel/driver may not import the offline peer engine
#endif
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#error Device and Work code may not import the offline peer engine
#endif
#ifndef PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE_H
#define PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE_H
#include <cstdint>
struct Q38NativePeerTestBytes {
 void* context=nullptr;
 uint32_t& (*word)(void*,unsigned,uint32_t)=nullptr;
 uint32_t (*noc)(void*,unsigned,unsigned)=nullptr;
 void (*events)(void*,unsigned,const uint32_t*,const uint32_t*,unsigned,bool,bool)=nullptr;
 bool (*read)(void*,unsigned,uint32_t,uint8_t*,unsigned)=nullptr;
 bool (*put)(void*,unsigned,uint32_t,uint32_t)=nullptr;
 uint64_t (*clock)(void*)=nullptr;
}; // RAW L1/CSR/GDDR byte endpoints ONLY;no processor/owner/ready/product callbacks
struct Q38NativePeerTestStats {
 uint32_t pc,steps,matrixInstructions,moveInstructions,floatOps,requests,responses,pending;
 uint32_t deviceReads,deviceWrites,statusWrites,minStack,sequence,active;
}; // observations ONLY;never accepted as a processor or native-owner input
class Q38NativePeerTestCpu {
 struct Impl;Impl* impl;
public:
 Q38NativePeerTestCpu(const char*,const char*,unsigned,const Q38NativePeerTestBytes&);
 ~Q38NativePeerTestCpu();
 Q38NativePeerTestCpu(const Q38NativePeerTestCpu&)=delete;
 Q38NativePeerTestCpu& operator=(const Q38NativePeerTestCpu&)=delete;
 void Link(Q38NativePeerTestCpu&);void Advance(unsigned);Q38NativePeerTestStats Stats()const;
 bool ObserveMatrixOutput()const; // independent arithmetic observer ONLY,never a ready/completion setter
}; // virgin construction + execution ONLY;NO PC/register/history/ready setters
#endif
