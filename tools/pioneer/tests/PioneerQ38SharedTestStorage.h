/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TEST_SHARED_WORKER_STORAGE) || PIONEER_Q38_TEST_SHARED_WORKER_STORAGE != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Shared storage is an explicit HOST-only instruction test,never a driver/Work grant
#endif
#ifndef PIONEER_Q38_SHARED_TEST_STORAGE_H
#define PIONEER_Q38_SHARED_TEST_STORAGE_H
#include "pioneer_q38_native_peer_test_bridge.h"
#include <cassert>
#include <map>
struct PioneerQ38SharedTestBinding {
 const Q38NativePeerTestBytes& bytes;const unsigned worker;bool executing=false;
 uint32_t& Word(uint32_t address)const{
  assert(executing && bytes.context && bytes.word && worker && worker<120 && !(address&3) && address<0x180000);
  return bytes.word(bytes.context,worker,address);
 }
};
class PioneerQ38SharedTestConstruction {
 static inline thread_local PioneerQ38SharedTestBinding* current=nullptr;
 PioneerQ38SharedTestBinding* previous;
public:
 explicit PioneerQ38SharedTestConstruction(PioneerQ38SharedTestBinding& binding):previous(current){assert(!current);current=&binding;}
 ~PioneerQ38SharedTestConstruction(){current=previous;}
 static PioneerQ38SharedTestBinding& Binding(){assert(current && !current->executing);return *current;}
};
// Generic legacy fixture constructors initialize synthetic mailboxes/code/
// operands. Keep those writes in DISCARDED constructor scratch,NEVER in
// the native backing. No instruction executes in that phase. The SAME
// storage object is bound from construction;every executed access uses
// the actual byte endpoint,with NO constructor data/history copied into it.
class PioneerQ38SharedTestMap {
 PioneerQ38SharedTestBinding& binding=PioneerQ38SharedTestConstruction::Binding();
 std::map<uint32_t,uint32_t> construction;
public:
 uint32_t& operator[](uint32_t address){return binding.executing?binding.Word(address):construction[address];}
 uint32_t& at(uint32_t address){return binding.executing?binding.Word(address):construction.at(address);}
 const uint32_t& at(uint32_t address)const{return binding.executing?binding.Word(address):construction.at(address);}
 unsigned count(uint32_t address)const{return binding.executing?unsigned(!(address&3) && address<0x180000):unsigned(construction.count(address));}
 void ForgetConstruction(){assert(!binding.executing);construction.clear();}
};
template<unsigned Base,unsigned Words> class PioneerQ38SharedTestArray {
 PioneerQ38SharedTestMap words;
public:
 uint32_t& operator[](unsigned i){assert(i<Words);return words[Base+i*4];}
 const uint32_t& operator[](unsigned i)const{assert(i<Words);return words.at(Base+i*4);}
 void ForgetConstruction(){words.ForgetConstruction();}
};
#endif
