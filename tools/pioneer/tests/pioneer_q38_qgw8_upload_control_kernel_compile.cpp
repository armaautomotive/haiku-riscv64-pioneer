/* SPDX-License-Identifier: MIT */
// Compile the actual non-loadable handler against declared kernel-shaped
// stubs. No driver ABI, public selector, device, or runtime implementation.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_QGW8_SPAN_INSPECTION 1
#define PIONEER_Q38_QGW8_CONTROL_INSPECTION 1
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#define PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define B_DEVICE_OP_CODES_END 1000
#include <stddef.h>
#include <new>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadReceipt.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerResidentSession.h"
using status_t=int;using uint32=uint32_t;
enum {B_OK=0,B_NOT_ALLOWED=-1,B_BAD_VALUE=-2,B_DEV_NOT_READY=-3};
struct thread_info {int team;};
extern int geteuid();extern int find_thread(void*);extern status_t get_thread_info(int,thread_info*);
extern status_t user_memcpy(void*,const void*,size_t);extern void release_sem(int);
struct ProbeIO {bool SpanBeforeCohort();};
struct DramRef {tt_dram_samples_result dram;};
struct Baseline2 {DramRef baseline;};struct Baseline3 {Baseline2 baseline;};struct Baseline4 {Baseline3 baseline;};
class PioneerQ38HybridUploadReceipt;
struct ProbeContext {
 PioneerResidentBanks banks;PioneerQ38Qgw8UploadReceipt* q38Qgw8Receipt;
 PioneerQ38HybridUploadReceipt* q38HybridReceipt;PioneerQ38SpanLease* q38Span;
 void* q38TokenNative;uint32_t* hashes;uint8_t* initialized;uint64_t bytes;unsigned uploadedPages;
 ProbeIO io;Baseline4 baseline;bool Unsafe();
};
struct PioneerProbeCookie {uint32 magic;int team;ProbeContext* context;};
extern PioneerProbeCookie* sRuntimeOwner;extern bool sBankStorageOnly,sColdRecoveryRequired,sReloadQuiesced;
extern uint64_t sReloadGeneration;extern int sLock;
extern status_t AcquireProbeLock(PioneerProbeCookie*,bool);
extern bool PioneerQ38SpanOwner(const ProbeContext&);
extern void RetainQ38DispatchOwner(ProbeContext&,tt_resident_session_result&);
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadControlInspection.inc"
extern "C" status_t qgw8_upload_control_compile(PioneerProbeCookie* cookie,void* data,size_t length){
 return Q38Qgw8UploadControlInspection(cookie,data,length);
}
extern "C" status_t qgw8_upload_control_dispatch_compile(PioneerProbeCookie* instance,uint32 operation,void* data,size_t length){
#include "pioneer_qgw8_dispatch.inc"
 return B_NOT_ALLOWED;
}
