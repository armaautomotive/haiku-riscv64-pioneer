/* SPDX-License-Identifier: MIT */
#define PIONEER_HOST_READ_MOCK_ONLY
#include "pioneer_host_read_probe_test.cpp"
static bool Page(HostReadMock& io,tt_host_read_result& r)
{
	io.transferBytes=4096;
	auto b=io.Baseline(); auto n=io.Compute(); auto p=io.Path(); auto a=io.Audit();
	return BlackholeHostReadProbe<HostReadMock,4096>(io,b,n,p,a,r);
}
int main()
{
	HostReadMock io; tt_host_read_result r;
	assert(Page(io,r) && r.completed && r.after[1]-r.before[1]==64 && !r.recovery_required); io.Safe();
	unsigned writes=io.Mock::writes;
	for (unsigned drop=1;drop<=writes;drop++) {
		io={}; io.Mock::drop=drop; Page(io,r);
		assert(io.requests<=1);
		if (r.recovery_required) assert(r.quarantined && io.quarantined && io.PageMock::allocated && !io.releases);
		else io.Safe();
		if (r.issued && !r.completed) assert(r.recovery_required && !io.postIssueWrites);
	}
	for (unsigned fault=0;fault<8;fault++) {
		io={}; io.timeout=fault==0; io.acceptedOnly=fault==1; io.badFlits=fault==2; io.changed=fault==3;
		io.clearFails=fault==4; io.restoreFails=fault==5; io.lateMapChange=fault==6; io.pciePending=fault==7;
		assert(!Page(io,r) && r.recovery_required && io.quarantined && !io.releases);
		if (fault!=5) assert(!io.postIssueWrites);
	}
	io={}; io.badData=true;
	assert(!Page(io,r) && r.completed && r.mismatches==1024 && !r.recovery_required); io.Safe();
	io={}; io.hostCorrupt=true;
	assert(!Page(io,r) && r.host_mismatches==1 && !r.recovery_required); io.Safe();
	io={}; io.backupFail=true;
	assert(!Page(io,r) && r.state==TT_HOST_READ_BACKUP_FAILED && !io.PageMock::allocated && !io.Mock::writes);
	io={}; io.physical=UINT64_C(0xfffffffff000);
	for (unsigned w=0;w<4;w++) io.counters[w]=UINT32_MAX;
	assert(Page(io,r)); io.Safe();
}
