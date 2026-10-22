/* SPDX-License-Identifier: MIT */
// RV64 kernel compile/stack witness ONLY,NOT a native adapter. Intentionally
// NO PCI-write,NoC-command-write,host numeric load/store,STOP/END/release APIs.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnMatrixProof.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnMatrixController.h"
struct FfnMatrixCompileIO {
 bool FfnMatrixScope(const PioneerQ38FfnMatrixLease&,const PioneerQ38FfnBanks&,const PioneerQ38DispatchLease&,const PioneerQ38FfnIdentity&);
 bool FfnMatrixInputReady(const PioneerQ38FfnMatrixLease&,const PioneerQ38FfnMatrixJob&);
 void FfnMatrixRetainHostOwners();void QuarantineHostPage();
 bool SelectRuntimeCore(unsigned);bool FfnMatrixPrefixStable();bool Q38PanelMappingStable();
 uint32_t FfnMatrixPrefixRead(unsigned);void FfnMatrixPrefixWrite(unsigned,uint32_t);
 uint32_t Q38PanelRead(uint32_t);void Q38PanelWrite(uint32_t,uint32_t);
 uint32_t RuntimeCodeRead(unsigned);uint32_t ResetRead();uint32_t SourceRead(unsigned);
 uint32_t HostPciRead(unsigned,uint16_t,uint8_t);uint32_t ResidentNocRead(uint32_t);
 uint32_t RuntimePcRead();uint32_t RuntimeStatusRead(unsigned);void RuntimeStatusWrite(unsigned,uint32_t);
 void RuntimeResetWrite(uint32_t);void Pause();uint64_t Q38CohortNow();
};
extern "C" bool pioneer_ffn_matrix_prepare_compile(FfnMatrixCompileIO& io,const tt_dram_samples_result& d,PioneerResidentBanks& b,
 PioneerQ38FfnBanks& s,PioneerQ38DispatchLease& c,const PioneerQ38FfnIdentity& id,PioneerQ38FfnMatrixLease& l)
{return PioneerQ38FfnMatrixPrepare(io,d,b,s,c,id,l);}
extern "C" bool pioneer_ffn_matrix_run_compile(FfnMatrixCompileIO& io,const tt_dram_samples_result& d,PioneerResidentBanks& b,
 PioneerQ38FfnBanks& s,PioneerQ38DispatchLease& c,PioneerQ38FfnMatrixLease& l,const uint8_t* initialized,unsigned pages,
 const PioneerQ38FfnMatrixJob& j,PioneerQ38FfnMatrixResult& r)
{return PioneerQ38FfnMatrixRun(io,d,b,s,c,l,initialized,pages,j,r);}
extern "C" bool pioneer_ffn_matrix_close_compile(FfnMatrixCompileIO& io,const tt_dram_samples_result& d,PioneerResidentBanks& b,
 PioneerQ38FfnBanks& s,PioneerQ38DispatchLease& c,PioneerQ38FfnMatrixLease& l,bool clean)
{return PioneerQ38FfnMatrixClose(io,d,b,s,c,l,clean);}
