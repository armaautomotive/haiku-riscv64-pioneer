/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE) || PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || (defined(TT_PIONEER_Q38_WORK_MODE) && (!defined(PIONEER_Q38_NATIVE_PEER_RECURRENT_MATRIX_TEST) \
  || PIONEER_Q38_NATIVE_PEER_RECURRENT_MATRIX_TEST != 1 || !defined(PIONEER_Q38_NATIVE_PEER_RECURRENT_TEST) \
  || PIONEER_Q38_NATIVE_PEER_RECURRENT_TEST != 1 || !defined(PIONEER_Q38_TEST_BF16_CONNECTED_PREPARE) \
  || PIONEER_Q38_TEST_BF16_CONNECTED_PREPARE != 1)) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Native BFP peer backend is an isolated host instruction test
#endif
#ifndef PIONEER_Q38_NATIVE_BFP_PEER_BRIDGE_H
#define PIONEER_Q38_NATIVE_BFP_PEER_BRIDGE_H
#include "pioneer_q38_native_peer_test_bridge.h"
#include <cstdint>
// Offline tensor/queued-read endpoints only. No CPU/worker state setter,
// completion supplier, model loader, device handle or execution authority.
class Q38NativeBfpPeerBridge {
 struct Impl;Impl* impl;
public:
 Q38NativeBfpPeerBridge(const uint32_t job[16],unsigned worker,const Q38NativePeerTestBytes& bytes);
 ~Q38NativeBfpPeerBridge();
 Q38NativeBfpPeerBridge(const Q38NativeBfpPeerBridge&)=delete;
 Q38NativeBfpPeerBridge& operator=(const Q38NativeBfpPeerBridge&)=delete;
 bool Handles(uint32_t address)const;
 uint32_t Read(uint32_t address);
 void Write(uint32_t address,uint32_t value);
 unsigned MatrixInstructions()const;
 bool Clean()const;
 uint32_t CounterDelta(unsigned index)const;
 uint32_t LastCommand(unsigned index)const;
};
#endif
