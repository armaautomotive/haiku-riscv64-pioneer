/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE) || PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE != 1 \
 || !defined(PIONEER_Q38_NATIVE_BFP4_PEER_TEST) || PIONEER_Q38_NATIVE_BFP4_PEER_TEST != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Native BFP4 peer backend requires an isolated host test
#endif
#ifndef PIONEER_Q38_NATIVE_BFP4_PEER_BRIDGE_H
#define PIONEER_Q38_NATIVE_BFP4_PEER_BRIDGE_H
#include "pioneer_q38_native_peer_test_bridge.h"
#include <cstdint>
class Q38NativeBfp4PeerBridge {
 struct Impl;Impl* impl;
public:
 Q38NativeBfp4PeerBridge(const uint32_t job[16],unsigned worker,const Q38NativePeerTestBytes& bytes);
 ~Q38NativeBfp4PeerBridge();
 Q38NativeBfp4PeerBridge(const Q38NativeBfp4PeerBridge&)=delete;
 Q38NativeBfp4PeerBridge& operator=(const Q38NativeBfp4PeerBridge&)=delete;
 bool Handles(uint32_t address)const;
 uint32_t Read(uint32_t address);
 void Write(uint32_t address,uint32_t value);
 unsigned MatrixInstructions()const;
 bool Clean()const;
 uint32_t CounterDelta(unsigned index)const;
 uint32_t LastCommand(unsigned index)const;
};
#endif
