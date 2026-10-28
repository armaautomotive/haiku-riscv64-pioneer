/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_TRANSPORT_H
#define PIONEER_Q38_RECURRENT_PREFIX_NATIVE_TRANSPORT_H
#include <stdint.h>
class PioneerQ38RecurrentPrefixPlan;struct PioneerQ38RecurrentPrefixResult;
struct RecurrentBlockRawLearnedBytes;struct PrefixOriginalStageSink;
// TEST raw endpoints ONLY. Native FD/source/clock/producer/init permissions
// and private phases/ready records NEVER cross this translation boundary.
struct PrefixInitializedByteSource {
 virtual ~PrefixInitializedByteSource()=default;
 virtual bool Recurrent(unsigned,uint32_t,uint32_t*,unsigned)const=0;
 virtual bool History(unsigned,uint32_t,uint32_t*,unsigned)const=0;
};
class RecurrentPrefixNativeTransport {
 struct Impl;Impl* implementation;
public:
 enum Read {Source,Reset,ComputeNoc,Tlb,Local,Code,Control,Status,Noc,Panel,Pci,Pc};
 enum Write {SetTlb,SetLocal,SetCode,SetStatus,SetNoc,SetPanel,SetReset};
 RecurrentPrefixNativeTransport(const char*,const char*);~RecurrentPrefixNativeTransport();
 RecurrentPrefixNativeTransport(const RecurrentPrefixNativeTransport&)=delete;
 RecurrentPrefixNativeTransport& operator=(const RecurrentPrefixNativeTransport&)=delete;
 void SeedRoot(const uint32_t*,const uint32_t*,unsigned,const uint32_t*,const uint32_t*,const uint32_t*);
 bool Select(unsigned);bool MapPanel();bool PanelStable();
 uint32_t Read32(Read,unsigned=0,unsigned=0,unsigned=0);void Write32(Write,unsigned,uint32_t,unsigned=0);
 uint32_t MappedRead(uint32_t);void MappedWrite(uint32_t,uint32_t);
 uint32_t* PanelSaved();uint32_t* Backup(unsigned);
 void Quarantine();void Pause();uint64_t Now()const;unsigned Touches()const;unsigned Resets()const;uint64_t Fingerprint();
 bool Configure(const PioneerQ38RecurrentPrefixPlan&,const uint32_t*,unsigned,const PrefixInitializedByteSource&);
 bool AttachOriginalLearned(RecurrentBlockRawLearnedBytes&);
 bool CaptureOriginalOutput(uint32_t*,unsigned);bool PublishOriginalStages(PrefixOriginalStageSink&);
 void EnableValidationReadback(unsigned=0);unsigned ReadbackFaultFence()const;unsigned ReadbackPackets()const;
 void VerifyReadback(const uint32_t*,unsigned);
 void PreserveClosedValidationObservation(const uint32_t*,unsigned);
 void Observe(const PioneerQ38RecurrentPrefixResult&); // read-only ordering observer,not a native phase setter
 void VerifyRun();void VerifyClosed();void Diagnostic();
};
#endif
