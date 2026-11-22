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

struct ApertureIO {
	uint32_t tlb[3]={0x12345678,0x55667788,0x1234},reg=0x00502042;
	unsigned tlbWrites=0,dropTlbWrite=0,dataWrites=0;
	bool route=true;
	bool DbiRouteStable() { return route; }
	uint32_t DbiTlbRead(unsigned n) { assert(n<3);return tlb[n]; }
	void DbiTlbWrite(unsigned n,uint32_t v) {
		assert(n<3);if (++tlbWrites!=dropTlbWrite) tlb[n]=v;
	}
	uint32_t DbiApertureRead() { return reg; }
	void DbiApertureWrite(uint32_t value) {
		assert(!(value&0xffff0000));dataWrites++;
		reg=(reg&0xffff0000&~value)|(value&0xffff);
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
	assert(PioneerRecoveryDbiTlbWord(0,false)==uint32_t(UINT64_C(0xf800000000000000)>>21));
	assert(PioneerRecoveryDbiTlbWord(1,false)==0x10017c0
		&& PioneerRecoveryDbiTlbWord(2,false)==0xa40
		&& PioneerRecoveryDbiTlbWord(2,true)==0x40
		&& PioneerRecoveryDbiTlbWord(3,false)==UINT32_MAX);
	ApertureIO mapped;PioneerRecoveryDbiAperture<ApertureIO> capture(mapped);
	assert(capture.Begin(false) && capture.Dirty());
	assert(PioneerRecoveryDbiCapture(capture,0x2040,snapshot));
	capture.DbiWrite(0);assert(!mapped.dataWrites);
	assert(capture.Finish() && !capture.Dirty() && mapped.tlb[0]==0x12345678
		&& mapped.tlb[1]==0x55667788 && mapped.tlb[2]==0x1234);
	assert(!capture.Begin(false));
	ApertureIO writable;writable.reg=0x00501002;
	PioneerRecoveryDbiAperture<ApertureIO> restore(writable);
	assert(restore.Begin(true));
	assert(PioneerRecoveryDbiRestore(restore,0x2040,snapshot,attempted)
		&& attempted && writable.reg==0x00501042 && writable.dataWrites==1);
	assert(restore.Finish() && !restore.Dirty());
	for (unsigned drop=1;drop<=6;drop++) {
		ApertureIO failed;failed.dropTlbWrite=drop;
		PioneerRecoveryDbiAperture<ApertureIO> aperture(failed);
		const bool began=aperture.Begin(false);
		if (drop<=3) assert(!began && aperture.Dirty());
		else assert(began);
		const bool finished=aperture.Finish();
		if (drop<=3) assert(finished && !aperture.Dirty());
		else assert(!finished && aperture.Dirty());
	}
	ApertureIO badRoute;badRoute.route=false;
	PioneerRecoveryDbiAperture<ApertureIO> refused(badRoute);
	assert(!refused.Begin(false) && !refused.Dirty() && !badRoute.tlbWrites);
	ApertureIO changed;PioneerRecoveryDbiAperture<ApertureIO> retained(changed);
	assert(retained.Begin(false));changed.route=false;
	assert(!retained.Finish() && retained.Dirty() && changed.tlbWrites==3);
	std::puts("PASS DBI payload capture/restore checks: simulated transport only");
}
