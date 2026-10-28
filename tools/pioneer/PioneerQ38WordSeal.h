/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_WORD_SEAL_H
#define PIONEER_Q38_WORD_SEAL_H
// Code-generation helpers ONLY, not ownership, initialization or completion
// evidence. Each comparison still faults immediately. Four-word unrolling
// removes loop/address overhead without hashing, caching or skipping a word.
// The caller retains all pointer, lifetime, phase, NoC and source checks.
extern "C" void scalar_fault_park() __attribute__((noreturn));
template<unsigned Count>
static inline __attribute__((always_inline)) void PioneerQ38WordSeal(
 const volatile uint32_t* actual,const volatile uint32_t* expected){
 static_assert(Count && !(Count&3),"exact four-word groups, no unexamined tail");
 for(unsigned i=0;i<Count;i+=4){
  if(actual[i]!=expected[i])scalar_fault_park();
  if(actual[i+1]!=expected[i+1])scalar_fault_park();
  if(actual[i+2]!=expected[i+2])scalar_fault_park();
  if(actual[i+3]!=expected[i+3])scalar_fault_park();
 }
}
template<unsigned Count>
static inline __attribute__((always_inline)) void PioneerQ38WordSealPairs(
 const volatile uint32_t* first,const volatile uint32_t* firstCopy,
 const volatile uint32_t* second,const volatile uint32_t* secondCopy){
 static_assert(Count && !(Count&3),"exact four-word groups, no unexamined tail");
 for(unsigned i=0;i<Count;i+=4){
  if(first[i]!=firstCopy[i] || second[i]!=secondCopy[i])scalar_fault_park();
  if(first[i+1]!=firstCopy[i+1] || second[i+1]!=secondCopy[i+1])scalar_fault_park();
  if(first[i+2]!=firstCopy[i+2] || second[i+2]!=secondCopy[i+2])scalar_fault_park();
  if(first[i+3]!=firstCopy[i+3] || second[i+3]!=secondCopy[i+3])scalar_fault_park();
 }
}
static inline __attribute__((always_inline)) void PioneerQ38WordSealSources(const uint32_t* expected){
 // The SAME sixteen live MMIO reads, in the SAME order as Source(i). Fixed
 // addresses avoid recomputing the piecewise Source() mapping on every word.
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20100U)!=expected[0])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20118U)!=expected[1])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb2011cU)!=expected[2])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20120U)!=expected[3])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20124U)!=expected[4])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20128U)!=expected[5])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb2012cU)!=expected[6])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20130U)!=expected[7])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20134U)!=expected[8])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20138U)!=expected[9])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb2013cU)!=expected[10])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20140U)!=expected[11])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20144U)!=expected[12])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20150U)!=expected[13])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb20154U)!=expected[14])scalar_fault_park();
 if(*reinterpret_cast<const volatile uint32_t*>(0xffb2016cU)!=expected[15])scalar_fault_park();
}
#endif
