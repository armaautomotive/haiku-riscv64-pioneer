/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SPAN_SERVICE_H
#define PIONEER_Q38_SPAN_SERVICE_H
#include "PioneerQ38SpanProtocol.h"
#include "PioneerQ38SpanController.h"
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
template<class C,bool Work=false> static bool PioneerQ38SpanUploadService(C& c,tt_q38_span_upload& request)
{
	request.result={}; request.result.version=1; request.result.state=TT_RUNTIME_CONFIG_FAILED;
	if (!PioneerQ38SpanOwner(c) || !PioneerQ38SpanPublicShape(request.version,request.bank,request.offset,request.bytes,request.reserved)) return false;
	auto& dram=c.baseline.baseline.baseline.baseline.dram; auto& noc=c.baseline.baseline.baseline.noc;
	PioneerQ38SpanExtent extent;
	if (!PioneerQ38SpanAuthorize(c.banks,dram,c.initialized,uint32_t(c.bytes/4096),request.bank,request.offset,request.bytes,extent)
		|| !PioneerQ38SpanPrepareService(c)) return false;
	PioneerQ38SpanResult result;
	bool passed=PioneerQ38SpanUpload<decltype(c.io),Work>(c.io,dram,noc,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,*c.q38Span,
		c.initialized,uint32_t(c.bytes/4096),request.bank,request.offset,request.bytes,request.data,request.bytes/4,result);
	request.result={result.version,result.state,result.bytes,result.pages,result.nonce,result.started,result.completed,
		result.held,result.host_exact,result.matched_words,result.recovery_required,0};
	if (!passed) return false;
	for (unsigned p=0;p<extent.pages;p++) {
		c.hashes[extent.index+p]=PioneerResidentPageHash(request.data+p*1024);
		c.initialized[extent.index+p]=1; c.uploadedPages++;
	}
	return true;
}
template<class C> static bool PioneerQ38SpanEndService(C& c)
{
	return PioneerQ38SpanOwner(c) && c.q38Span && !c.Unsafe()
		&& PioneerQ38SpanClose(c.io,c.baseline.baseline.baseline.baseline.dram,c.baseline.baseline.baseline.noc,
			c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,*c.q38Span,true);
}
#endif
