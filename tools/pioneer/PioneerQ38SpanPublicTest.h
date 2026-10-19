/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SPAN_PUBLIC_TEST_H
#define PIONEER_Q38_SPAN_PUBLIC_TEST_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include <memory>
#include <cstdio>
#include <cstring>
// Fixed PUBLIC transport qualification ONLY. No learned/model arithmetic.
// Failure is CLOSE ONLY, including a successful transport with a bad reply.
template<class IO,bool Work=false> static bool PioneerQ38SpanPublicTest(IO& io)
{
	int fd=io.Open(); if (fd<0) return false;
	auto fail=[&]() { io.Close(fd); return false; };
	tt_q38_span_support support={}; tt_q38_bank_support banks={};
	if (!io.Call(fd,Work?TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT:TT_PCI_PROBE_Q38_SPAN_SUPPORT,&support,sizeof(support)) || support.driver!=55 || support.version!=1
		|| support.banks!=7 || support.page_bytes!=4096 || support.max_bytes!=1048576 || support.before_cohort!=1
		|| support.explicit_end!=1 || support.reserved
		|| !io.Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&banks,sizeof(banks)) || banks.driver!=45 || banks.version!=1
		|| banks.banks!=7 || banks.page_bytes!=4096 || banks.max_pages!=4 || banks.storage_only!=1
		|| banks.bytes_per_bank!=PioneerResidentBanks::Capacity) return fail();
	tt_q38_bank_session session={}; session.version=1;
	if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session)) || session.version!=1 || session.reserved || session.result.version!=1
		|| session.result.state!=TT_RESIDENT_SESSION_READY || session.result.active!=1 || session.result.host_ready!=1
		|| session.result.cold || session.result.capabilities || session.result.bytes!=PioneerResidentBanks::Total) return fail();
	auto request=std::make_unique<tt_q38_span_upload>(); unsigned nonce=0; uint64_t transferred=0,totalNanos=0;
	for (unsigned bank=0;bank<7;bank++) for (unsigned kind=0;kind<3;kind++) {
		memset(request.get(),0,sizeof(*request)); request->version=1; request->bank=bank; request->bytes=kind==0?4096:kind==1?20480:1048576;
		request->offset=kind==0?0:kind==1?4096:PioneerResidentBanks::Capacity-request->bytes;
		for (unsigned w=0;w<request->bytes/4;w++) request->data[w]=0x7fc00000U+(w+bank*271+kind*13)*0x9e3779b9U;
		uint64_t start=io.NowNanos(); bool okay=io.Call(fd,Work?TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD:TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request));
		uint64_t end=io.NowNanos(); const auto& r=request->result;
		if (!okay || r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.bytes!=request->bytes || r.pages!=request->bytes/4096
			|| r.nonce!=++nonce || r.started!=1 || r.completed!=1 || r.held!=1 || r.host_exact!=(Work?0U:1U)
			|| r.matched_words!=(Work?0U:request->bytes/4) || r.recovery_required || r.reserved || end<start) return fail();
		totalNanos+=end-start; transferred+=request->bytes;
		printf("PUBLIC_SPAN bank%u bytes%u pages%u nonce%u seconds%.6f known_held1 cpu_transport_checked%u;NOT_MODEL_OR_TOKEN_RATE\n",
			bank,request->bytes,r.pages,r.nonce,double(end-start)*1e-9,Work?0U:1U);
	}
	tt_q38_span_end end={1,0,0,0};
	if (!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end)) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required) return fail();
	if constexpr (Work) {
		// ONLY this separate qualification client reads EVERY frozen word
		// independently after code restoration. Production Work has NONE.
		uint64_t words=0;
		for (unsigned bank=0;bank<7;bank++) for (unsigned kind=0;kind<3;kind++) {
			unsigned bytes=kind==0?4096:kind==1?20480:1048576;
			uint64_t start=kind==0?0:kind==1?4096:PioneerResidentBanks::Capacity-bytes;
			for (unsigned page=0;page<bytes/4096;page++) {
				tt_q38_bank_page q={}; q.version=1; q.bank=bank; q.offset=start+uint64_t(page)*4096;
				if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_READ,&q,sizeof(q)) || q.version!=1 || q.bank!=bank
					|| q.offset!=start+uint64_t(page)*4096 || q.result.version!=1 || q.result.state!=TT_STORE_PASSED
					|| q.result.requests!=1 || q.result.completed!=1 || q.result.recovery_required || q.result.mismatches) return fail();
				for (unsigned w=0;w<1024;w++) if (q.data[w]!=0x7fc00000U+(page*1024+w+bank*271+kind*13)*0x9e3779b9U) return fail();
				words+=1024;
			}
		}
		printf("PUBLIC_WORK_SPAN_DEBUG_READBACK matched_bytes%llu;EVERY_ORIGINAL_BYTE_COMPARED_DEBUG_CLIENT_ONLY\n",(unsigned long long)(words*4));
	}
	session={}; session.version=1;
	if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session)) || session.version!=1 || session.reserved || session.result.version!=1
		|| session.result.state!=TT_RESIDENT_SESSION_CLOSED || session.result.active || session.result.pci_restored!=1
		|| session.result.host_released!=1 || session.result.cold) return fail();
	bool clean=io.Close(fd)==0;
	printf("%s_PUBLIC_SPAN_TRANSPORT jobs%u bytes%llu upload_seconds%.6f host_MB_per_second%.6f clean_end%u;NOT_PURE_PCIE_BANDWIDTH_OR_LLM_THROUGHPUT\n",
		clean?"PASS":"FAIL",nonce,(unsigned long long)transferred,double(totalNanos)*1e-9,
		totalNanos?double(transferred)*1000/double(totalNanos):0,unsigned(clean));
	return clean;
}
#endif
