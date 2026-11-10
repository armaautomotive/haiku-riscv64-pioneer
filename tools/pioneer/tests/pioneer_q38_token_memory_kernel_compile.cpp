/* SPDX-License-Identifier: MIT */
// Template/stack witness,NOT genuine native IO or a loadable driver.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenMemory.h"
#ifdef PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeOwner.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38QgwdNativeBinding.h"
struct QgwdCompileDram {tt_dram_samples_result dram;};
struct QgwdCompileBase2 {QgwdCompileDram baseline;};
struct QgwdCompileBase3 {QgwdCompileBase2 baseline;};
struct QgwdCompileBase4 {QgwdCompileBase3 baseline;};
struct QgwdCompileContext {
 PioneerQ38DenseQ8UploadReceiptInspection* q38QgwdReceipt;
 void* q38HybridReceipt;void* q38Qgw8Receipt;PioneerQ38SpanLease* q38Span;
 uint32_t* hashes;uint8_t* initialized;PioneerResidentBanks banks;QgwdCompileBase4 baseline;
};
struct QgwdCompileCookie {uint32_t magic;QgwdCompileContext* context;int64_t team;};
#endif
struct TokenMemoryKernelCompileIO {
 bool TokenMemoryScope(const PioneerQ38TokenMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38TokenStartupPlan&,
  const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool TokenMemoryMappingStable(unsigned);bool TokenMemoryReadChecked(unsigned,uint32_t,uint32_t&);
 bool TokenMemoryWrite(unsigned,uint32_t,uint32_t);void TokenMemoryRetainHostOwners();void QuarantineHostPage();
 bool SelectRuntimeCore(unsigned);bool Q38PanelMappingStable();
 uint32_t ResetRead();uint32_t SourceRead(unsigned);uint32_t ResidentNocRead(uint32_t);
 uint32_t HostPciRead(unsigned,uint16_t,uint8_t);uint32_t RuntimeCodeRead(unsigned);uint32_t Q38PanelRead(uint32_t);
#ifdef PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY
 bool TokenRuntimeScope(const PioneerQ38TokenNativeOwner&,const PioneerQ38TokenMemory&,const PioneerQ38CanonicalModelPlan&,
  const PioneerQ38TokenStartupPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 uint32_t* Q38PanelSavedWords();const uint32_t* BulkSavedWords();bool BulkMappingStable();
 uint64_t Q38CohortNow();void ResidentNocWrite(uint32_t,uint32_t);void Pause();
 void RuntimeResetWrite(uint32_t);uint32_t RuntimePcRead();
 bool TokenDispatchImageWrite(unsigned,uint32_t,uint32_t);bool TokenDispatchImageReadChecked(unsigned,uint32_t,uint32_t&);
 bool TokenPeerNiuWrite(unsigned,uint32_t,uint32_t);bool TokenPeerNiuReadChecked(unsigned,uint32_t,uint32_t&);
 bool TokenPeerScratchWrite(unsigned,uint32_t,uint32_t);bool TokenPeerScratchReadChecked(unsigned,uint32_t,uint32_t&);
 bool TokenRootNiuWrite(uint32_t,uint32_t);bool TokenRootNiuReadChecked(uint32_t,uint32_t&);
 bool TokenRootScratchWrite(uint32_t,uint32_t);bool TokenRootScratchReadChecked(uint32_t,uint32_t&);
 bool TokenSharedRootCommandReadChecked(unsigned,uint32_t&);bool TokenSharedTlbReadChecked(unsigned,unsigned,uint32_t&);
 bool TokenSharedTlbWrite(unsigned,unsigned,uint32_t);
 bool TokenPeerStoreSelect(unsigned);bool TokenFifoReadChecked(unsigned,bool,uint32_t&);
#endif
};
extern "C" bool pioneer_q38_token_memory_template_prepare(PioneerQ38TokenMemory& heap,TokenMemoryKernelCompileIO& io,
 const tt_dram_samples_result& dram,PioneerResidentBanks& banks,PioneerQ38DispatchLease& cohort,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38TokenStartupPlan& startup,const PioneerQ38GdnIdentity& id,const uint8_t* ledger,unsigned pages,PioneerQ38TokenMemoryResult& result){
 return heap.PrepareHeld(io,dram,banks,cohort,plan,startup,id,ledger,pages,result);
}
extern "C" bool pioneer_q38_token_memory_template_close(PioneerQ38TokenMemory& heap,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenMemoryResult& result){return heap.CloseHeldNoLaunch(io,result);}
#if defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) && !defined(PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY)
extern "C" bool pioneer_q38_hybrid_layout_template(PioneerQ38CanonicalModelPlan& heap){return heap.BuildHybridDiagnostic();}
#endif
#ifdef PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY
extern "C" bool pioneer_q38_hybrid_manifest_install_template(PioneerQ38TokenMemory& heap,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenMemoryResult& result){return heap.InstallHybridManifestHeld(io,result);}
extern "C" bool pioneer_q38_hybrid_manifest_verify_template(PioneerQ38TokenMemory& heap,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenMemoryResult& result){return heap.VerifyHybridManifestHeld(io,result);}
#endif
#ifdef PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY
extern "C" bool pioneer_q38_qgwd_bind_template(PioneerQ38TokenMemory& heap,PioneerQ38DenseQ8UploadReceiptInspection& receipt,
 const void* context,const PioneerQ38DispatchLease* cohort,const void* cookie,const uint32_t* hashes,int64_t team,uint64_t generation){
 return heap.BindDenseReceiptInspection(receipt,context,cohort,cookie,hashes,team,generation);
}
extern "C" bool pioneer_q38_qgwd_native_journal_bind_template(QgwdCompileContext& context,const QgwdCompileCookie* cookie,
 uint64_t generation,PioneerQ38DispatchLease* cohort,PioneerQ38TokenMemory& memory){
 return PioneerQ38QgwdNativeBinding::BindJournal(context,cookie,generation,cohort,memory);
}
extern "C" bool pioneer_q38_qgwd_manifest_install_template(PioneerQ38TokenMemory& heap,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenMemoryResult& result){return heap.InstallDenseManifestHeld(io,result);}
extern "C" bool pioneer_q38_qgwd_manifest_verify_template(PioneerQ38TokenMemory& heap,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenMemoryResult& result){return heap.VerifyDenseManifestHeld(io,result);}
extern "C" bool pioneer_q38_qgwd_runtime_template(PioneerQ38TokenNativeOwner& owner,TokenMemoryKernelCompileIO& io,
 const tt_dram_samples_result& dram,PioneerResidentBanks& banks,PioneerQ38DispatchLease& cohort,PioneerQ38TokenMemory& memory,
 const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38TokenStartupPlan& startup,const PioneerQ38GdnIdentity& id,
 const uint8_t* ledger,unsigned pages,PioneerQ38TokenRuntimeResult& result){
 return owner.PrepareHeldRuntime(io,dram,banks,cohort,memory,plan,startup,id,ledger,pages,result);
}
extern "C" bool pioneer_q38_qgwd_bootstrap_template(PioneerQ38TokenNativeOwner& owner,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenBootstrapResult& result){return owner.BootstrapWorkers(io,result);}
extern "C" bool pioneer_q38_qgwd_start_template(PioneerQ38TokenNativeOwner& owner,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenExecutionResult& result){return owner.StartRoot(io,result);}
extern "C" bool pioneer_q38_qgwd_hold_template(PioneerQ38TokenNativeOwner& owner,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenHoldResult& result){return owner.HoldStopped(io,result);}
extern "C" bool pioneer_q38_qgwd_restore_l1_template(PioneerQ38TokenNativeOwner& owner,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenRestoreResult& result){return owner.RestoreOriginalL1(io,result);}
extern "C" bool pioneer_q38_qgwd_restore_images_template(PioneerQ38TokenNativeOwner& owner,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenDispatchRestoreResult& result){return owner.RestoreDispatchImages(io,result);}
extern "C" bool pioneer_q38_qgwd_restore_peers_template(PioneerQ38TokenNativeOwner& owner,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenPeerRestoreResult& result){return owner.RestorePeerStores(io,result);}
extern "C" bool pioneer_q38_qgwd_restore_root_template(PioneerQ38TokenNativeOwner& owner,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenRootStoreRestoreResult& result){return owner.RestoreRootStore(io,result);}
extern "C" bool pioneer_q38_qgwd_restore_shared_template(PioneerQ38TokenNativeOwner& owner,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenSharedTlbRestoreResult& result){return owner.RestoreSharedMappings(io,result);}
extern "C" bool pioneer_q38_qgwd_read_template(PioneerQ38TokenNativeOwner& owner,TokenMemoryKernelCompileIO& io,
 unsigned count,uint32_t* words,PioneerQ38TokenReadResult& result){return owner.ReadCommitted(io,count,words,result);}
#endif
