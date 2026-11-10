/* SPDX-License-Identifier: MIT */
// Compile the exact QGWD receipt/control against Haiku kernel-shaped types.
// No driver entry points, ioctl dispatch, device access or inference.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_QGW8_SPAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION 1
#define PIONEER_Q38_QGWD_SPAN_INSPECTION 1
#define PIONEER_Q38_QGWD_CONTROL_INSPECTION 1
#define PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION 1
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#define PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define B_DEVICE_OP_CODES_END 1000
#include <stddef.h>
#include <new>
#include "../PioneerQ38DenseQ8UploadReceiptInspection.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerResidentSession.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanService.h"
static_assert(PioneerQ38SpanReceiptKind<PioneerQ38DenseQ8UploadReceiptInspection>::Value==3,"distinct QGWD receipt");
using status_t=int;using uint32=uint32_t;
enum {B_OK=0,B_NOT_ALLOWED=-1,B_BAD_VALUE=-2,B_DEV_NOT_READY=-3};
struct thread_info {int team;};
extern int geteuid();extern int find_thread(void*);extern status_t get_thread_info(int,thread_info*);
extern status_t user_memcpy(void*,const void*,size_t);extern void release_sem(int);
struct ProbeIO {bool SpanBeforeCohort();};
struct DramRef {tt_dram_samples_result dram;};
struct Baseline2 {DramRef baseline;};struct Baseline3 {Baseline2 baseline;};struct Baseline4 {Baseline3 baseline;};
class PioneerQ38HybridUploadReceipt;class PioneerQ38Qgw8UploadReceipt;
struct ProbeContext {
 PioneerResidentBanks banks;PioneerQ38DenseQ8UploadReceiptInspection* q38QgwdReceipt;
 PioneerQ38Qgw8UploadReceipt* q38Qgw8Receipt;PioneerQ38HybridUploadReceipt* q38HybridReceipt;
 PioneerQ38SpanLease* q38Span;void* q38TokenNative;uint32_t* hashes;uint8_t* initialized;
 uint64_t bytes;unsigned uploadedPages;ProbeIO io;Baseline4 baseline;bool Unsafe();
};
struct PioneerProbeCookie {uint32 magic;int team;ProbeContext* context;};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38QgwdNativeBinding.h"
extern PioneerProbeCookie* sRuntimeOwner;extern bool sBankStorageOnly,sColdRecoveryRequired,sReloadQuiesced;
extern uint64_t sReloadGeneration;extern int sLock;
extern status_t AcquireProbeLock(PioneerProbeCookie*,bool);
extern bool PioneerQ38SpanOwner(const ProbeContext&);
extern void RetainQ38DispatchOwner(ProbeContext&,tt_resident_session_result&);
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38QgwdUploadControlInspection.inc"
extern "C" status_t qgwd_upload_control_compile(PioneerProbeCookie* cookie,void* data,size_t length){
 return Q38QgwdUploadControlInspection(cookie,data,length);
}
extern "C" bool qgwd_dense_manifest_compile(PioneerQ38DenseQ8UploadReceiptInspection* receipt,uint32_t* output,
 const void* context,const void* cookie,const uint32_t* hashes,const uint8_t* ledger,int64_t team,uint64_t generation){
 return receipt->DenseManifest(output,PioneerQ38DenseQ8UploadReceiptInspection::DenseManifestWords,
  context,cookie,hashes,ledger,team,generation);
}
extern "C" bool qgwd_bound_canonical_compile(PioneerQ38DenseQ8UploadReceiptInspection* receipt,
 PioneerQ38CanonicalModelPlan* output,bool physical,const void* context,const void* cookie,
 const uint32_t* hashes,const uint8_t* ledger,int64_t team,uint64_t generation){
 return receipt->DeriveCanonical(*output,physical,context,cookie,hashes,ledger,team,generation);
}
extern "C" unsigned qgwd_span_receipt_kind_compile(){
 return PioneerQ38SpanReceiptKind<PioneerQ38DenseQ8UploadReceiptInspection>::Value;
}
extern "C" bool qgwd_receipt_identity_compile(const PioneerQ38DenseQ8UploadReceiptInspection* receipt,
 const void* context,const void* cookie,const uint32_t* hashes,const uint8_t* ledger,int64_t team,uint64_t generation){
 return receipt->Matches(context,cookie,hashes,ledger,team,generation);
}
extern "C" bool qgwd_native_binding_compile(ProbeContext* context,PioneerProbeCookie* cookie,
 uint64_t generation,PioneerQ38CanonicalModelPlan* logical){
 return PioneerQ38QgwdNativeBinding::Matches(*context,cookie,generation)
  && PioneerQ38QgwdNativeBinding::DeriveLogical(*context,cookie,generation,*logical);
}
