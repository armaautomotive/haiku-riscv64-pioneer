/* SPDX-License-Identifier: MIT */
// HOST instruction observations only. No processor/native-owner input API.
#if !defined(PIONEER_Q38_INSTRUCTION_STACK_OBSERVER_TEST) || PIONEER_Q38_INSTRUCTION_STACK_OBSERVER_TEST != 1
#error Explicit instruction-stack observation TEST required
#endif
#if defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#error Instruction-stack TEST cannot enter device or Work code,including defined0
#endif
#if defined(TT_PIONEER_Q38_WORK_MODE) && (defined(_KERNEL_MODE) \
 || !defined(PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE) || PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE != 1 \
 || !defined(PIONEER_Q38_TEST_SHARED_WORKER_STORAGE) || PIONEER_Q38_TEST_SHARED_WORKER_STORAGE != 1)
#error Legacy HOST instruction-model tuning flags require the exact isolated peer TEST,not Work authority
#endif
#if defined(_KERNEL_MODE) && (_KERNEL_MODE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE != 1)
#error Only the explicit emulated native-layer fixture may import this HOST observer
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) && !defined(_KERNEL_MODE)
#error Native-context macros require the exact emulated kernel-layer TEST
#endif
#ifndef PIONEER_Q38_INSTRUCTION_STACK_OBSERVER_H
#define PIONEER_Q38_INSTRUCTION_STACK_OBSERVER_H
#include <cstdint>
class Q38InstructionStackObserver {
 uint64_t instructions=0;
 uint32_t minimum=0x70000;
 bool initialized=false,invalid=false;
public:
 bool Retired(uint32_t sp){
  if(invalid || instructions==UINT64_MAX){invalid=true;return false;}
  instructions++;
  // Initial boot instructions may precede the real SP initializer. Once
  // nonzero SP was observed,zero cannot masquerade as a new virgin CPU.
  if(!sp){if(initialized)invalid=true;return !invalid;}
  if((sp&3U) || sp<0x6e000 || sp>0x70000){invalid=true;return false;}
  initialized=true;if(sp<minimum)minimum=sp;return true;
 }
 bool Valid()const{return !invalid;}
 bool Initialized()const{return initialized;}
 uint32_t MinimumSp()const{return minimum;}
 uint64_t Instructions()const{return instructions;}
}; // accumulated across HOST watchdog rebases;never a ready/completion proof
#endif
