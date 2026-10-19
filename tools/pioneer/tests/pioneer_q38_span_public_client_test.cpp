/* SPDX-License-Identifier: MIT */
#include <cstdio>
static int SilentPrintf(const char*,...) { return 0; }
#define printf SilentPrintf
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38SpanPublicTest.h"
#undef printf
#include <cassert>
struct FakeTransport {
	unsigned calls=0,uploads=0,begins=0,ends=0,spanEnds=0,closes=0,failCall=0,badReply=0;
	uint64_t clock=0; bool stopped=false;
	bool work=false;
	int Open() { return 73; }
	int Close(int fd) { assert(fd==73); closes++; return 0; }
	uint64_t NowNanos() { return clock+=1000000; }
	bool Call(int fd,uint32_t op,void* data,size_t) {
		assert(fd==73 && !stopped); calls++;
		if (calls==failCall) { stopped=true; return false; }
		if (op==TT_PCI_PROBE_Q38_SPAN_SUPPORT || op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT) {
			work=op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT; *static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};
		}
		else if (op==TT_PCI_PROBE_Q38_BANK_SUPPORT) *static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,PioneerResidentBanks::Capacity};
		else if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) {
			begins++; auto& r=static_cast<tt_q38_bank_session*>(data)->result; r={}; r.version=1;
			r.state=TT_RESIDENT_SESSION_READY; r.active=r.host_ready=1; r.bytes=PioneerResidentBanks::Total;
		} else if (op==TT_PCI_PROBE_Q38_SPAN_UPLOAD || op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD) {
			auto& q=*static_cast<tt_q38_span_upload*>(data); uploads++;
			assert(PioneerQ38SpanPublicShape(q.version,q.bank,q.offset,q.bytes,q.reserved));
			unsigned kind=(uploads-1)%3; assert(q.bank==(uploads-1)/3);
			for (unsigned w=0;w<q.bytes/4;w++) assert(q.data[w]==0x7fc00000U+(w+q.bank*271+kind*13)*0x9e3779b9U);
			q.result={1,TT_RUNTIME_PASSED,q.bytes,q.bytes/4096,uploads,1,1,1,1,q.bytes/4,0,0};
			if (work) q.result.host_exact=q.result.matched_words=0;
		} else if (op==TT_PCI_PROBE_Q38_SPAN_END) { spanEnds++; *static_cast<tt_q38_span_end*>(data)={1,0,1,0}; }
		else if (op==TT_PCI_PROBE_Q38_BANK_READ) {
			assert(work && spanEnds==1); auto& q=*static_cast<tt_q38_bank_page*>(data); q.result={}; q.result.version=1;
			q.result.state=TT_STORE_PASSED; q.result.requests=q.result.completed=1;
			unsigned kind=q.offset>=PioneerResidentBanks::Capacity-1048576?2:q.offset?1:0;
			uint64_t start=kind==2?PioneerResidentBanks::Capacity-1048576:kind==1?4096:0;
			unsigned page=unsigned((q.offset-start)/4096);
			for (unsigned w=0;w<1024;w++) q.data[w]=0x7fc00000U+(page*1024+w+q.bank*271+kind*13)*0x9e3779b9U;
		}
		else { assert(op==TT_PCI_PROBE_Q38_BANK_END && spanEnds==1); ends++;
			auto& r=static_cast<tt_q38_bank_session*>(data)->result; r={}; r.version=1; r.state=TT_RESIDENT_SESSION_CLOSED; r.pci_restored=r.host_released=1;
		}
		if (calls==badReply) {
			if (op==TT_PCI_PROBE_Q38_SPAN_UPLOAD) static_cast<tt_q38_span_upload*>(data)->result.nonce^=1;
			else if (op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD) static_cast<tt_q38_span_upload*>(data)->result.nonce^=1;
			else if (op==TT_PCI_PROBE_Q38_BANK_READ) static_cast<tt_q38_bank_page*>(data)->data[0]^=1;
			else *static_cast<uint32_t*>(data)^=1;
			stopped=true;
		}
		return true;
	}
};
int main()
{
	FakeTransport good; assert(PioneerQ38SpanPublicTest(good) && good.uploads==21 && good.ends==1 && good.closes==1);
	for (unsigned call=1;call<=good.calls;call++) for (unsigned kind=0;kind<2;kind++) {
		FakeTransport fault; if (kind) fault.badReply=call; else fault.failCall=call;
		assert(!PioneerQ38SpanPublicTest(fault) && fault.calls==call && fault.closes==1);
	}
	FakeTransport work; assert((PioneerQ38SpanPublicTest<FakeTransport,true>(work)) && work.uploads==21 && work.ends==1 && work.closes==1);
	for (unsigned call:{1U,4U,24U,25U,26U,1000U,work.calls}) for (unsigned kind=0;kind<2;kind++) {
		FakeTransport fault; if (kind) fault.badReply=call; else fault.failCall=call;
		assert((!PioneerQ38SpanPublicTest<FakeTransport,true>(fault)) && fault.calls==call && fault.closes==1);
	}
	puts("PASS FIXED PUBLIC SPAN CLIENT: all21 frozen payloads, EVERY transaction/reply failure is CLOSE ONLY with no END/retry/reset/additional card call; NO DEVICE IO");
}
