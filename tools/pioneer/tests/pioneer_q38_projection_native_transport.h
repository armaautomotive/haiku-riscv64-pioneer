/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_NATIVE_TRANSPORT_H
#define PIONEER_Q38_PROJECTION_NATIVE_TRANSPORT_H
#include <stdint.h>
#include <vector>
struct PioneerQ38ProjectionGraphResult;
struct ProjectionExecutionLearnedBytes;
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
class PioneerQ38CanonicalModelPlan;struct DecoderHistoryBytes;
struct PioneerQ38ResidentDecoderResult;
#endif
// TEST PCI/L1/clock endpoints ONLY. No native context, capability, owner,
// scope, page ledger, private proof, or prepared lease is fabricated here.
// A separate TU avoids colliding with the original native source fixture.
class ProjectionNativeTransport {
 struct Impl;Impl* implementation;
public:
 enum Read {Source,Reset,ComputeNoc,Tlb,Local,Code,Control,Status,Noc,Panel,Pci,Pc};
 enum Write {SetTlb,SetLocal,SetCode,SetStatus,SetNoc,SetPanel,SetReset};
 ProjectionNativeTransport(const char* root,const char* peer,std::vector<uint32_t>& deviceWorkspace);
 ~ProjectionNativeTransport();
 ProjectionNativeTransport(const ProjectionNativeTransport&)=delete;
 ProjectionNativeTransport& operator=(const ProjectionNativeTransport&)=delete;
 void SeedRoot(const uint32_t* addresses,const uint32_t* values,unsigned count,
  const uint32_t* source,const uint32_t* command,const uint32_t* counters);
 bool Select(unsigned w);bool MapPanel();bool PanelStable();
 uint32_t Read32(Read kind,unsigned a=0,unsigned b=0,unsigned c=0);
 void Write32(Write kind,unsigned a,uint32_t value,unsigned b=0);
 uint32_t MappedRead(uint32_t address);void MappedWrite(uint32_t address,uint32_t value);
 uint32_t* PanelSaved();uint32_t* Backup(unsigned words);
 void Quarantine();void Pause();uint64_t Now()const;
 unsigned Boots()const;unsigned Starts()const;unsigned Stops()const;
 unsigned Matrices()const;unsigned Producers()const;unsigned Touches()const;
 void VerifyOutput();void Diagnostic()const;
 void Fault(unsigned kind); // simulated endpoint fault,never a production mode
 void Observe(const PioneerQ38ProjectionGraphResult& result); // TEST fault timing only
 void LearnedSource(ProjectionExecutionLearnedBytes& source); // TEST bytes,NO numerical products or authority
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
 void OutputSources(const char* root,const char* peer);void VerifyGdnOutput();
 void OutputFault(unsigned);bool OutputFaultInjected()const; // TEST endpoint failures only
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
 void ResidualRmsSource(const char*);void ResidualRmsFault(unsigned);bool ResidualRmsFaultInjected()const;
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
 void FfnSources(const char*,const char*);void VerifyFfn();
 void FfnFault(unsigned);bool FfnFaultInjected()const;
 void FfnRunTerminalCheckpoint(void (*callback)(void*),void* context); // fork actual in-flight stack AFTER emitted RUN terminal
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
 void NextRmsSource(const char*);void NextRmsFault(unsigned);bool NextRmsFaultInjected()const;void VerifyNextRms();
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
 void NextProjectionSources(const char*,const char*);void VerifyNextProjections();
 void NextProjectionFault(unsigned);bool NextProjectionFaultInjected()const;
 void NextProjectionRunTerminalCheckpoint(void (*callback)(void*),void* context); // TEST fork actual last RUN,not a ready/producer setter
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
 void VerifyNextConv();
 void NextConvFault(unsigned);bool NextConvFaultInjected()const;unsigned NextConvStarts()const;
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
 void VerifyNextGdn();
 void NextGdnFault(unsigned);bool NextGdnFaultInjected()const;unsigned NextGdnStarts()const;
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
 void NextOutputSources(const char*,const char*);void VerifyNextOutput();
 unsigned NextOutputStarts()const;
 void NextOutputFault(unsigned);bool NextOutputFaultInjected()const;
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 void NextResidualSource(const char*);unsigned NextResidualStarts()const;
 void NextResidualFault(unsigned);bool NextResidualFaultInjected()const;unsigned NextResidualFaultTouch()const;
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 void NextFfnSources(const char*,const char*);void VerifyNextFfn();unsigned NextFfnStarts()const;
 void NextFfnFault(unsigned);bool NextFfnFaultInjected()const;unsigned NextFfnFaultTouch()const;
 void NextFfnRunTerminalCheckpoint(void (*callback)(void*),void* context);
#endif
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
 void OutputValidationCheckpoint(void (*callback)(void*),void* context);
 void OutputReadbackFault(unsigned kind,unsigned word=0);bool OutputReadbackFaultInjected()const;
 unsigned OutputNumericReads()const;bool ValidationFaultInjected()const;
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 void DecoderSources(const char*,const PioneerQ38CanonicalModelPlan&,unsigned bank,DecoderHistoryBytes&); // TEST byte endpoints only
 unsigned DecoderStarts()const;
 void ObserveDecoder(const PioneerQ38ResidentDecoderResult&);
 void DecoderFault(unsigned); // TEST endpoint failure/snapshot only,never a production selector
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
 void GdnInputSource(const char*); // TEST exact emitted-byte image,not a native capability
 void GdnInputFault(unsigned);unsigned GdnInputStarts()const;
 bool GdnInputFaultInjected()const;
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
 void GatedLaunchDelay(unsigned worker,unsigned quanta,uint64_t hostMicroseconds=0); // TEST scheduling only,not readiness/producer authority
 bool GatedLaunchDelayInjected()const;
#endif
#if defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
 void GdnReadbackFault(unsigned word);unsigned GdnValidationReads()const;
#endif
#if defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
 void GdnValidationCheckpoint(void (*callback)(void*),void* context); // TEST fork ONLY after genuine initialized/held producer
 void GdnValidationRootFault(); // TEST endpoint mutation,not a readiness/owner setter
 void GdnValidationStatusFault(unsigned field);void GdnValidationRestoreFault();bool GdnValidationRestoreInjected()const;
#endif
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
 std::vector<uint32_t> DecoderCompletionReads()const; // TEST phase4 status/PC trace,NO device I/O
 unsigned DecoderFrontendSnapshots()const; // TEST observed speculative fallthroughs,NO device I/O
#endif
#endif
#if defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
 void ValidationFault(unsigned kind,unsigned page); // TEST endpoint fault,not production capability
 unsigned ValidationReads()const;
#endif
};
#endif
