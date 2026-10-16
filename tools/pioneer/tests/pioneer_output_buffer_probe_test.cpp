/* SPDX-License-Identifier: MIT */
#define PIONEER_HOST_PAGE_MOCK_ONLY
#include "pioneer_host_page_test.cpp"
#define PIONEER_OUTPUT_BUFFER_MOCK_ONLY
#include "pioneer_output_buffer_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerOutputBufferProbe.h"
struct ProbeMock:PageMock,OutputBufferMock {};
int main()
{
	(void)&Input;
	tt_output_buffer_probe_result r={};
	{
		ProbeMock io; assert(PioneerOutputBufferProbe(io,r));
		assert(r.version==1 && r.state==TT_OUTPUT_PROBE_PASSED && r.maps_stable);
		assert(r.input_released && r.output_released && !r.output_quarantined);
		assert(r.input_patterns==2 && r.output_patterns==4 && r.input_address_bits>32 && r.output_address_bits>32);
		assert(io.PageMock::releases==1 && io.OutputBufferMock::releases==1);
	}
	for (unsigned drop=1;drop<=2048;drop++) {
		ProbeMock io; io.PageMock::drop=drop; assert(!PioneerOutputBufferProbe(io,r));
		assert(r.input_released && !r.output_allocated && r.output_released && !io.OutputBufferMock::allocations);
	}
	for (unsigned drop=1;drop<=16384;drop++) {
		ProbeMock io; io.OutputBufferMock::drop=drop; assert(!PioneerOutputBufferProbe(io,r));
		assert(r.input_released && r.output_released && !r.output_quarantined && r.output_mismatches);
	}
	for (unsigned fault=0;fault<11;fault++) {
		ProbeMock io;
		io.PageMock::allocFail=fault==0; io.PageMock::mapFail=fault==1; io.PageMock::cacheFail=fault==2;
		io.PageMock::mapChanged=fault==3; io.PageMock::releaseFail=fault==4;
		io.OutputBufferMock::allocationFails=fault==5; io.OutputBufferMock::cacheFails=fault==6;
		io.OutputBufferMock::mapFails=fault==7; io.OutputBufferMock::lateMap=fault==8;
		io.OutputBufferMock::releaseFails=fault==9;
		if (fault==10) io.OutputBufferMock::physical=io.PageMock::physical;
		assert(!PioneerOutputBufferProbe(io,r));
		if (fault!=4) assert(r.input_released);
		if (fault!=9) assert(r.output_released && !r.output_quarantined);
		else assert(r.output_quarantined && !r.output_released && io.OutputBufferMock::releaseAttempts==1);
	}
	for (unsigned page=0;page<4;page++) for (unsigned fault=0;fault<2;fault++) {
		ProbeMock io;
		if (fault) io.OutputBufferMock::shortPage=page; else io.OutputBufferMock::badPage=page;
		assert(!PioneerOutputBufferProbe(io,r) && r.input_released && r.output_released && !io.OutputBufferMock::writes);
	}
	puts("PASS: OFFLINE CPU-only4KiB+16KiB allocation probe, all18432 lost stores, every page, alias/map/cache failures and cleanup; no PCI/MMIO/DMA interface or public physical address");
}
