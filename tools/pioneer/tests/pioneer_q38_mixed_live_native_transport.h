/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_NATIVE_TRANSPORT_H
#define PIONEER_Q38_MIXED_LIVE_NATIVE_TRANSPORT_H
#include <stdint.h>
struct PioneerQ38MixedLiveResult;
class PioneerQ38MixedLiveBlockPlan;
struct ProjectionExecutionLearnedBytes;
// TEST physical byte endpoints only. No native context, phase, permission,
// completed producer or prepared lease is supplied by this transport.
class MixedLiveNativeTransport {
 struct Impl;Impl* implementation;
public:
 enum Read {Source,Reset,ComputeNoc,Tlb,Local,Code,Control,Status,Noc,Panel,Pci,Pc};
 enum Write {SetTlb,SetLocal,SetCode,SetStatus,SetNoc,SetPanel,SetReset};
 MixedLiveNativeTransport(const char*,const char*,const char*,const char*);~MixedLiveNativeTransport();
 MixedLiveNativeTransport(const MixedLiveNativeTransport&)=delete;
 MixedLiveNativeTransport& operator=(const MixedLiveNativeTransport&)=delete;
 void SeedRoot(const uint32_t*,const uint32_t*,unsigned,const uint32_t*,const uint32_t*,const uint32_t*);
 bool Select(unsigned);bool MapPanel();bool PanelStable();
 uint32_t Read32(Read,unsigned=0,unsigned=0,unsigned=0);
 void Write32(Write,unsigned,uint32_t,unsigned=0);
 uint32_t MappedRead(uint32_t);void MappedWrite(uint32_t,uint32_t);
 uint32_t* PanelSaved();uint32_t* Backup(unsigned);
 void Quarantine();void Pause();uint64_t Now()const;
 unsigned Touches()const;unsigned Resets()const;
 uint64_t Fingerprint();
 void Configure(const PioneerQ38MixedLiveBlockPlan&,const uint32_t*,unsigned);
#ifdef PIONEER_Q38_MIXED_LIVE_ORIGINAL_NATIVE_TEST
 bool AttachOriginalLearned(ProjectionExecutionLearnedBytes&); // RAW test weight endpoint ONLY,not a model/native-ready setter
#endif
 void Observe(const PioneerQ38MixedLiveResult&); // TEST reset/terminal assertions only
 void VerifyRun();void VerifyClosed();void Diagnostic();
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK
 void VerifyReadback(const uint32_t*,unsigned); // observer ONLY after complete capture+restore
#endif
};
#endif
