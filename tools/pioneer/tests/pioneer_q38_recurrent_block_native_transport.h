/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_TRANSPORT_H
#define PIONEER_Q38_RECURRENT_BLOCK_NATIVE_TRANSPORT_H
#include <stdint.h>
class PioneerQ38MixedRecurrentBlockPlan;
struct PioneerQ38RecurrentBlockResult;
// TEST raw byte boundary ONLY. Never transfers native context,permission,
// initialized/complete seals,clock,leases or processor ready/progress bits.
struct RecurrentInitializedByteSource {
 virtual ~RecurrentInitializedByteSource()=default;
 virtual bool Recurrent(unsigned,uint32_t,uint32_t*,unsigned)const=0;
 virtual bool History(unsigned,uint32_t,uint32_t*,unsigned)const=0;
};
class RecurrentBlockNativeTransport {
 struct Impl;Impl* implementation;
public:
 enum Read {Source,Reset,ComputeNoc,Tlb,Local,Code,Control,Status,Noc,Panel,Pci,Pc};
 enum Write {SetTlb,SetLocal,SetCode,SetStatus,SetNoc,SetPanel,SetReset};
 RecurrentBlockNativeTransport(const char*,const char*);~RecurrentBlockNativeTransport();
 RecurrentBlockNativeTransport(const RecurrentBlockNativeTransport&)=delete;
 RecurrentBlockNativeTransport& operator=(const RecurrentBlockNativeTransport&)=delete;
 void SeedRoot(const uint32_t*,const uint32_t*,unsigned,const uint32_t*,const uint32_t*,const uint32_t*);
 bool Select(unsigned);bool MapPanel();bool PanelStable();
 uint32_t Read32(Read,unsigned=0,unsigned=0,unsigned=0);
 void Write32(Write,unsigned,uint32_t,unsigned=0);
 uint32_t MappedRead(uint32_t);void MappedWrite(uint32_t,uint32_t);
 uint32_t* PanelSaved();uint32_t* Backup(unsigned);
 void Quarantine();void Pause();uint64_t Now()const;
 unsigned Touches()const;unsigned Resets()const;uint64_t Fingerprint();
 bool Configure(const PioneerQ38MixedRecurrentBlockPlan&,const uint32_t*,unsigned,const RecurrentInitializedByteSource&);
 void Observe(const PioneerQ38RecurrentBlockResult&); // asserts reset/hold ordering,NEVER sets private progress
 void VerifyRun();void VerifyClosed();void Diagnostic();
};
#endif
