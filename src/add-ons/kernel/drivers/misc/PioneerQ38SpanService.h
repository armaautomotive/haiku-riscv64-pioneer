/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38QgwdNativeInspectionRole.h"
#ifndef PIONEER_Q38_SPAN_SERVICE_H
#define PIONEER_Q38_SPAN_SERVICE_H
#include "PioneerQ38SpanProtocol.h"
#include "PioneerQ38SpanController.h"
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
#if PIONEER_Q38_HYBRID_TRACKED_UPLOAD != 1 || !defined(PIONEER_Q38_GDN_MUTABLE_STATE)
#error Hybrid tracked upload requires its exact distinct session and mutable-bank layout
#endif
#include "PioneerQ38HybridUploadReceipt.h"
#include "PioneerQ38HybridUploadProtocol.h"
#endif
#ifdef PIONEER_Q38_QGW8_SPAN_INSPECTION
#if PIONEER_Q38_QGW8_SPAN_INSPECTION != 1 || !defined(PIONEER_Q38_HYBRID_TRACKED_UPLOAD) \
 || PIONEER_Q38_HYBRID_TRACKED_UPLOAD != 1 || !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) \
 || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1
#error QGW8 span inspection requires the distinct non-loadable tracked receipt role
#endif
#include "PioneerQ38Qgw8UploadReceipt.h"
#endif
#ifdef PIONEER_Q38_QGWD_SPAN_INSPECTION
#if PIONEER_Q38_QGWD_SPAN_INSPECTION != 1 || !defined(PIONEER_Q38_HYBRID_TRACKED_UPLOAD) \
 || PIONEER_Q38_HYBRID_TRACKED_UPLOAD != 1 || !defined(PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION) \
 || PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION != 1 \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION) && (!defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY != 1)) \
 || ((defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error QGWD span inspection requires its isolated gapped receipt
#endif
#include "../../../../../tools/pioneer/PioneerQ38DenseQ8UploadReceiptInspection.h"
#endif
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
template<class Receipt> struct PioneerQ38SpanReceiptKind {enum {Value=0};};
template<> struct PioneerQ38SpanReceiptKind<PioneerQ38HybridUploadReceipt> {enum {Value=1};};
#ifdef PIONEER_Q38_QGW8_SPAN_INSPECTION
template<> struct PioneerQ38SpanReceiptKind<PioneerQ38Qgw8UploadReceipt> {enum {Value=2};};
#endif
#ifdef PIONEER_Q38_QGWD_SPAN_INSPECTION
template<> struct PioneerQ38SpanReceiptKind<PioneerQ38DenseQ8UploadReceiptInspection> {enum {Value=3};};
#endif
#endif
// Native adapter's shared kernel-only core. Complete owner/fresh-page proof
// BEFORE allocation/preparation IO, and publish ALL page authority only
// after the existing exact transport/terminal/host-visibility proof passes.
template<class C> static bool PioneerQ38SpanOwner(const C& c)
{
	return c.bankStorageOnly && c.bytes==PioneerResidentBanks::Total && !c.q38Dispatch && !c.q38Cohort
		&& !c.panelProbe.saved && !c.streamProbe.saved && !c.fleet && c.runtimeCoreMode==-1;
}
template<class C> static bool PioneerQ38SpanPrepareService(C& c)
{
	if (!PioneerQ38SpanOwner(c) || !c.q38Span || c.q38Span->closed || c.q38Span->unsafe) return false;
	if (c.q38Span->ready) return true;
	if (c.q38Span->saved || c.q38Span->dirty) return false; // never retry partial preparation
	auto& dram=c.baseline.baseline.baseline.baseline.dram; auto& noc=c.baseline.baseline.baseline.noc;
	bool ready=c.lease.ready;
	if (!ready && !c.lease.saved) ready=PioneerResidentLeaseBegin(c.io,noc,c.lease);
	if (ready) ready=c.io.SelectRuntimeCore(0) && c.io.MapBulk();
	if (ready && !c.bulkHost.allocated) ready=PioneerPrepareOutputBuffer(c.io,c.host,c.bulkHost);
	ready=ready && c.bulkHost.ready && PioneerBulkUploadPrepare(c.io,dram,noc,c.host,c.lease,c.bulkHost,c.bulk);
	if (ready && !c.wideHost.allocated) ready=PioneerPrepareWideOutput(c.io,c.host,c.bulkHost,c.wideHost);
	return ready && c.io.SelectRuntimeCore(0) && c.io.MapQ38Panel()
		&& PioneerQ38SpanPrepare(c.io,dram,noc,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,*c.q38Span);
}
template<class C,bool Work=false
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
 ,class Receipt=PioneerQ38HybridUploadReceipt,bool Qgw8Fast=false
#endif
 > static bool PioneerQ38SpanUploadCore(C& c,tt_q38_span_upload& request
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
 ,Receipt* receipt
#endif
 )
{
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
	static_assert((PioneerQ38SpanReceiptKind<Receipt>::Value==1 && !Qgw8Fast)
		|| ((PioneerQ38SpanReceiptKind<Receipt>::Value==2 || PioneerQ38SpanReceiptKind<Receipt>::Value==3)
			&& ((!Work && !Qgw8Fast) || (Work && Qgw8Fast))),
		"Unsupported tracked span receipt/Work pairing");
#endif
	request.result={}; request.result.version=1; request.result.state=TT_RUNTIME_CONFIG_FAILED;
	if (!PioneerQ38SpanOwner(c) || !PioneerQ38SpanPublicShape(request.version,request.bank,request.offset,request.bytes,request.reserved)) return false;
	auto& dram=c.baseline.baseline.baseline.baseline.dram; auto& noc=c.baseline.baseline.baseline.noc;
	PioneerQ38SpanExtent extent;
	if (!PioneerQ38SpanAuthorize(c.banks,dram,c.initialized,uint32_t(c.bytes/4096),request.bank,request.offset,request.bytes,extent)) return false;
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
	if (!receipt || !receipt->Before(&c,c.hashes,c.initialized,c.q38Span,request.bank,request.offset,request.bytes)) return false;
#endif
	if (!PioneerQ38SpanPrepareService(c)) {
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
		if (receipt) receipt->Abort();
#endif
		return false;
	}
	PioneerQ38SpanResult result;
	bool passed=PioneerQ38SpanUpload<decltype(c.io),Work>(c.io,dram,noc,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,*c.q38Span,
		c.initialized,uint32_t(c.bytes/4096),request.bank,request.offset,request.bytes,request.data,request.bytes/4,result);
	request.result={result.version,result.state,result.bytes,result.pages,result.nonce,result.started,result.completed,
		result.held,result.host_exact,result.matched_words,result.recovery_required,0};
	if (!passed) {
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
		if (receipt) receipt->Abort();
#endif
		return false;
	}
	for (unsigned p=0;p<extent.pages;p++) {
		c.hashes[extent.index+p]=PioneerResidentPageHash(request.data+p*1024);
		c.initialized[extent.index+p]=1; c.uploadedPages++;
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
		if (receipt && !receipt->RecordPage(extent.index+p,c.hashes[extent.index+p])) return false;
#endif
	}
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
	if (receipt && !receipt->After()) return false;
#endif
	return true;
}
// The installed/legacy call keeps its exact original receipt type. The
// QGW8 overload is a separate non-loadable inspection entry point and cannot
// be selected through the existing driver control or Work path.
template<class C,bool Work=false> static bool PioneerQ38SpanUploadService(C& c,tt_q38_span_upload& request
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
 ,PioneerQ38HybridUploadReceipt* receipt
#endif
 )
{
	return PioneerQ38SpanUploadCore<C,Work>(c,request
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
		,receipt
#endif
	);
}
#ifdef PIONEER_Q38_QGW8_SPAN_INSPECTION
template<class C> static bool PioneerQ38Qgw8SpanUploadInspection(C& c,tt_q38_span_upload& request,
 PioneerQ38Qgw8UploadReceipt* receipt)
{
	return PioneerQ38SpanUploadCore<C,false>(c,request,receipt);
}
#ifdef PIONEER_Q38_SPAN_FAST
// The QGW8 uploader sends SPAN_WORK_UPLOAD and requires host_exact=0 and
// matched_words=0. This explicit adapter uses that same fast transport with
// the QGW8 receipt, without granting a real ioctl or inference capability.
// The strict inspection adapter above remains available separately.
template<class C> static bool PioneerQ38Qgw8SpanUploadFastInspection(C& c,tt_q38_span_upload& request,
 PioneerQ38Qgw8UploadReceipt* receipt)
{
	return PioneerQ38SpanUploadCore<C,true,PioneerQ38Qgw8UploadReceipt,true>(c,request,receipt);
}
#endif
#endif
#ifdef PIONEER_Q38_QGWD_SPAN_INSPECTION
// Same private span service and transport, but a different, gapped receipt.
// Inspection only: not a live ioctl, driver selection, or inference grant.
template<class C> static bool PioneerQ38QgwdSpanUploadInspection(C& c,tt_q38_span_upload& request,
 PioneerQ38DenseQ8UploadReceiptInspection* receipt)
{
	return PioneerQ38SpanUploadCore<C,false>(c,request,receipt);
}
#ifdef PIONEER_Q38_SPAN_FAST
template<class C> static bool PioneerQ38QgwdSpanUploadFastInspection(C& c,tt_q38_span_upload& request,
 PioneerQ38DenseQ8UploadReceiptInspection* receipt)
{
	return PioneerQ38SpanUploadCore<C,true,PioneerQ38DenseQ8UploadReceiptInspection,true>(c,request,receipt);
}
#endif
#endif
template<class C> static bool PioneerQ38SpanEndService(C& c)
{
	return PioneerQ38SpanOwner(c) && c.q38Span && !c.Unsafe()
		&& PioneerQ38SpanClose(c.io,c.baseline.baseline.baseline.baseline.dram,c.baseline.baseline.baseline.noc,
			c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,*c.q38Span,true);
}
#endif
