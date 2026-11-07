/* SPDX-License-Identifier: MIT */
// Freestanding witness only: no linked driver, public ABI, or device access.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridUploadReceipt.h"
extern "C" bool hybrid_receipt_begin(PioneerQ38HybridUploadReceipt& r,const void* c,const void* fd,const uint32_t* hashes,
 const uint8_t* ledger,int64_t team,uint64_t generation,PioneerResidentBanks& banks,const tt_dram_samples_result& proof,unsigned pages){
 return r.Begin(c,fd,hashes,ledger,team,generation,banks,proof,pages);
}
extern "C" bool hybrid_receipt_span(PioneerQ38HybridUploadReceipt& r,const void* c,const uint32_t* hashes,const uint8_t* ledger,
 const PioneerQ38SpanLease* span,unsigned bank,uint64_t offset,unsigned bytes){return r.Before(c,hashes,ledger,span,bank,offset,bytes);}
extern "C" bool hybrid_receipt_record(PioneerQ38HybridUploadReceipt& r,unsigned index,uint32_t hash){return r.RecordPage(index,hash);}
extern "C" bool hybrid_receipt_finish_span(PioneerQ38HybridUploadReceipt& r){return r.After();}
extern "C" bool hybrid_receipt_seal(PioneerQ38HybridUploadReceipt& r,const PioneerQ38SpanLease& span){return r.Seal(span);}
extern "C" bool hybrid_receipt_layout(PioneerQ38HybridUploadReceipt& r,PioneerQ38CanonicalModelPlan& plan,const void* c,
 const void* fd,const uint32_t* hashes,const uint8_t* ledger,int64_t team,uint64_t generation){
 return r.DeriveBoundLayout(plan,c,fd,hashes,ledger,team,generation);
}
