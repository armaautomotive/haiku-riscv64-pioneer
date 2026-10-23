/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_NATIVE_TRANSPORT_H
#define PIONEER_Q38_PROJECTION_NATIVE_TRANSPORT_H
#include <stdint.h>
#include <vector>
struct PioneerQ38ProjectionGraphResult;
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
};
#endif
