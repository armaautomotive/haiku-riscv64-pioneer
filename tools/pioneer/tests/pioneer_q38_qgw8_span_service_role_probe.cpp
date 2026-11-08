/* SPDX-License-Identifier: MIT */
// Compile-only role/format separation checks, no device context.
#include <stddef.h>
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#ifndef PIONEER_TEST_OMIT_TRACKED
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanService.h"
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
struct UnsupportedReceipt {};
static_assert(PioneerQ38SpanReceiptKind<PioneerQ38HybridUploadReceipt>::Value==1,"legacy identity");
static_assert(PioneerQ38SpanReceiptKind<UnsupportedReceipt>::Value==0,"unknown receipt refused");
#ifdef PIONEER_Q38_QGW8_SPAN_INSPECTION
static_assert(PioneerQ38SpanReceiptKind<PioneerQ38Qgw8UploadReceipt>::Value==2,"distinct QGW8 identity");
extern "C" unsigned qgw8_span_receipt_kind(){return PioneerQ38SpanReceiptKind<PioneerQ38Qgw8UploadReceipt>::Value;}
#ifdef PIONEER_TEST_QGW8_WORK_PAIR
struct IncompleteContext;
bool refuseWork(IncompleteContext& c,tt_q38_span_upload& p,PioneerQ38Qgw8UploadReceipt* r){
 return PioneerQ38SpanUploadCore<IncompleteContext,true>(c,p,r);
}
#endif
#ifdef PIONEER_TEST_QGW8_FALSE_FAST
struct IncompleteContext;
bool refuseFalseFast(IncompleteContext& c,tt_q38_span_upload& p,PioneerQ38Qgw8UploadReceipt* r){
 return PioneerQ38SpanUploadCore<IncompleteContext,false,PioneerQ38Qgw8UploadReceipt,true>(c,p,r);
}
#endif
#ifdef PIONEER_TEST_LEGACY_QGW8_FAST
struct IncompleteContext;
bool refuseLegacyFast(IncompleteContext& c,tt_q38_span_upload& p,PioneerQ38HybridUploadReceipt* r){
 return PioneerQ38SpanUploadCore<IncompleteContext,true,PioneerQ38HybridUploadReceipt,true>(c,p,r);
}
#endif
#endif
#endif
