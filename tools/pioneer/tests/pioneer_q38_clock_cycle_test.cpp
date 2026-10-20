/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38ClockCycle.h"
struct CycleIO {
	unsigned opened=0,closed=0,failCall=0,mutation=0;
	bool failOpen=false,failClose=false;
	std::vector<unsigned> calls;
	int Open() { opened++; return failOpen?-1:17; }
	bool Close(int fd) { assert(fd==17); closed++; return !failClose; }
	bool Call(int fd,unsigned op,void* data,size_t bytes) {
		assert(fd==17); calls.push_back(op); if (calls.size()==failCall) return false;
		if (op==TT_PCI_PROBE_RELOAD_STATUS) {
			assert(bytes==sizeof(tt_pioneer_driver_reload)); auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
			assert(r.version==1); r.driver=55; r.guard_version=1; r.build_tag=UINT64_C(0x2026102000010004); r.generation=4; r.open_count=1;
			if (mutation==1) r.version=0; else if (mutation==2) r.driver=54; else if (mutation==3) r.guard_version=0;
			else if (mutation==4) r.build_tag--; else if (mutation==5) r.generation=0; else if (mutation==6) r.quiesced=1;
			else if (mutation==7) r.cold=1; else if (mutation==8) r.active_owner=1; else if (mutation==9) r.open_count=2;
		} else if (op==TT_PCI_PROBE_Q38_BANK_SUPPORT) {
			assert(bytes==sizeof(tt_q38_bank_support)); auto& r=*static_cast<tt_q38_bank_support*>(data);
			r={45,1,7,4096,4,1,PioneerResidentBanks::Capacity}; if (mutation==10) r.version=0;
		} else {
			assert(bytes==sizeof(tt_q38_bank_session)); auto& r=*static_cast<tt_q38_bank_session*>(data);
			assert(r.version==1 && !r.reserved); auto& v=r.result; v={}; v.version=1;
			if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) {
				v.state=TT_RESIDENT_SESSION_READY; v.active=v.host_ready=1; v.bytes=PioneerResidentBanks::Total;
				if (mutation==11) v.version=0; else if (mutation==12) v.state=TT_RESIDENT_SESSION_PREFLIGHT_FAILED;
				else if (mutation==13) v.active=0; else if (mutation==14) v.host_ready=0; else if (mutation==15) v.cold=1;
				else if (mutation==16) v.capabilities=1; else if (mutation==17) v.bytes--;
			} else {
				assert(op==TT_PCI_PROBE_Q38_BANK_END); v.state=TT_RESIDENT_SESSION_CLOSED; v.pci_restored=v.host_released=1;
				if (mutation==18) v.version=0; else if (mutation==19) v.state=TT_RESIDENT_SESSION_CLEANUP_FAILED;
				else if (mutation==20) v.active=1; else if (mutation==21) v.cold=1;
				else if (mutation==22) v.pci_restored=0; else if (mutation==23) v.host_released=0;
			}
		}
		return true;
	}
};
int main()
{
	CycleIO good; assert(PioneerQ38ClockCycle(good) && good.opened==1 && good.closed==1);
	assert((good.calls==std::vector<unsigned>{TT_PCI_PROBE_RELOAD_STATUS,TT_PCI_PROBE_Q38_BANK_SUPPORT,TT_PCI_PROBE_Q38_BANK_BEGIN,TT_PCI_PROBE_Q38_BANK_END}));
	for (unsigned fail=1;fail<=4;fail++) {
		CycleIO io; io.failCall=fail; assert(!PioneerQ38ClockCycle(io) && io.calls.size()==fail && io.closed==1);
	}
	for (unsigned mutation=1;mutation<=23;mutation++) {
		CycleIO io; io.mutation=mutation; assert(!PioneerQ38ClockCycle(io) && io.closed==1);
		assert(io.calls.size()==(mutation<=9?1U:mutation==10?2U:mutation<=17?3U:4U));
	}
	CycleIO unopened; unopened.failOpen=true; assert(!PioneerQ38ClockCycle(unopened) && !unopened.closed && unopened.calls.empty());
	CycleIO close; close.failClose=true; assert(!PioneerQ38ClockCycle(close) && close.closed==1 && close.calls.size()==4);
	puts("PASS bounded clock qualification client:pinned idle metadata BEFORE BEGIN,only BEGIN/END,no compute/upload,each transport/metadata fault CLOSE ONLY,NO END after bad BEGIN/NO retry/status/reset after uncertainty");
}
