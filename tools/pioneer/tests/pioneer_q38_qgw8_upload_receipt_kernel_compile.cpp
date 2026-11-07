/* SPDX-License-Identifier: MIT */
// Non-loadable kernel ABI witness only; no device or execution authority.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadReceipt.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RuntimeRanges.h"
extern "C" bool qgw8_receipt_begin(PioneerQ38Qgw8UploadReceipt& r,const void* c,const void* fd,const uint32_t* hashes,
 const uint8_t* ledger,int64_t team,uint64_t generation,PioneerResidentBanks& banks,const tt_dram_samples_result& proof,unsigned pages){
 return r.Begin(c,fd,hashes,ledger,team,generation,banks,proof,pages);
}
extern "C" bool qgw8_receipt_span(PioneerQ38Qgw8UploadReceipt& r,const void* c,const uint32_t* hashes,const uint8_t* ledger,
 const PioneerQ38SpanLease* lease,unsigned bank,uint64_t offset,unsigned length){return r.Before(c,hashes,ledger,lease,bank,offset,length);}
extern "C" bool qgw8_receipt_record(PioneerQ38Qgw8UploadReceipt& r,unsigned index,uint32_t hash){return r.RecordPage(index,hash);}
extern "C" bool qgw8_receipt_after(PioneerQ38Qgw8UploadReceipt& r){return r.After();}
extern "C" bool qgw8_receipt_seal(PioneerQ38Qgw8UploadReceipt& r,const PioneerQ38SpanLease& lease){return r.Seal(lease);}
extern "C" bool qgw8_receipt_layout(PioneerQ38Qgw8UploadReceipt& r,PioneerQ38Qgw8KernelLayout& out,const void* c,const void* fd,
 const uint32_t* hashes,const uint8_t* ledger,int64_t team,uint64_t generation){return r.DeriveBoundLayout(out,c,fd,hashes,ledger,team,generation);}
extern "C" bool qgw8_receipt_canonical(PioneerQ38Qgw8UploadReceipt& r,PioneerQ38CanonicalModelPlan& out,const void* c,const void* fd,
 const uint32_t* hashes,const uint8_t* ledger,int64_t team,uint64_t generation){return r.DeriveBoundCanonical(out,c,fd,hashes,ledger,team,generation);}
extern "C" bool qgw8_runtime_ranges(const PioneerQ38CanonicalModelPlan& plan,PioneerResidentMutableRange (&ranges)[7]){
 return PioneerQ38RuntimeRanges(plan,ranges);
}
