/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRecoveryDbi.h"
#include <cassert>
#include <cstdio>

struct DbiIO {
	uint32_t reg=0x00502042,writeValue=0;
	unsigned reads=0,writes=0,failReadyAt=0,changeAt=0;
	bool ready=true,dropWrite=false;
	bool DbiReady() { return ready && (!failReadyAt || reads<failReadyAt); }
	uint32_t DbiRead() {
		++reads;
		if (changeAt==reads) reg^=0x100;
		return reg;
	}
	void DbiWrite(uint32_t value) {
		++writes;writeValue=value;
		// Model status as W1C, and all 16 control bits as writable.
		if (!dropWrite) reg=(reg&0xffff0000&~value)|(value&0xffff);
	}
};

int main()
{
	DbiIO io;PioneerRecoveryDbiSnapshot snapshot{};
	assert(PioneerRecoveryDbiCapture(io,0x2040,snapshot));
	assert(snapshot.valid && snapshot.mps==0x40 && io.writes==0);
	io.reg=0x00501002;io.reads=0;
	bool attempted=false;
	assert(PioneerRecoveryDbiRestore(io,0x2040,snapshot,attempted));
	assert(attempted && io.writes==1 && io.writeValue==0x1042
		&& io.reg==0x00501042);
	io.reads=0;attempted=true;
	assert(PioneerRecoveryDbiRestore(io,0x2040,snapshot,attempted));
	assert(!attempted && io.writes==1);
	DbiIO mismatch;assert(!PioneerRecoveryDbiCapture(mismatch,0,snapshot) && !snapshot.valid);
	DbiIO changing;changing.changeAt=2;
	assert(!PioneerRecoveryDbiCapture(changing,0x2040,snapshot) && !snapshot.valid);
	DbiIO notReady;notReady.ready=false;
	assert(!PioneerRecoveryDbiCapture(notReady,0x2040,snapshot));
	const uint32_t invalid[]={0xffffffffU,0x8000U,0x00010040U,0x00200040U,0x00c0U};
	for (uint32_t bad: invalid) {
		DbiIO failed;failed.reg=bad;
		assert(!PioneerRecoveryDbiCapture(failed,0x2040,snapshot) && !snapshot.valid);
	}
	DbiIO good;assert(PioneerRecoveryDbiCapture(good,0x2040,snapshot));
	DbiIO lost;lost.reg=0x00501002;lost.dropWrite=true;
	assert(!PioneerRecoveryDbiRestore(lost,0x2040,snapshot,attempted)
		&& attempted && lost.writes==1);
	DbiIO lostRoute;lostRoute.reg=0x00501002;lostRoute.failReadyAt=4;
	assert(!PioneerRecoveryDbiRestore(lostRoute,0x2040,snapshot,attempted) && attempted);
	DbiIO wrongPci;assert(!PioneerRecoveryDbiRestore(wrongPci,0,snapshot,attempted)
		&& !attempted && wrongPci.writes==0);
	DbiIO flr;flr.reg=0x8000;
	assert(!PioneerRecoveryDbiRestore(flr,0x2040,snapshot,attempted) && !flr.writes);
	DbiIO unstable;unstable.changeAt=4;
	assert(!PioneerRecoveryDbiRestore(unstable,0x2040,snapshot,attempted));
	std::puts("PASS DBI payload capture/restore checks: simulated transport only");
}
