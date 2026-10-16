/* SPDX-License-Identifier: MIT */
#define PIONEER_DUAL_MOCK_ONLY
#include "pioneer_runtime_dual_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimeAttention.h"

int main()
{
	// Shared fixtures also declare single-core helpers, tested elsewhere.
	(void)&Run; (void)&SecondRun; (void)&SecondClose;
	Fixture f; auto b=f.io.first.Baseline(); auto n=f.io.first.Compute(),n2=n; n2.version=2;
	PioneerAttentionTileLease a; a.saved=a.ready=true;
	// Treat the separately tested attention image as already prepared.
	assert(PioneerRuntimeAttentionPeersHeld(f.io,b,n,n2,f.store[0],f.store[1],f.compute[0],f.compute[1],a));
	unsigned resets=f.io.first.starts+f.io.second.starts,requests=f.io.first.requests+f.io.second.requests;
	for (unsigned repeat=0;repeat<4;repeat++) {
		assert(PioneerRuntimeAttentionSelect(f.io,b,n,n2,f.store[0],f.store[1],f.compute[0],f.compute[1],a,true));
		assert(f.io.current==0 && f.io.first.code[0]==TT_ATTENTION_TILE_BOOT && f.io.second.code[0]==0x0006a06f);
		assert(PioneerRuntimeAttentionSelect(f.io,b,n,n2,f.store[0],f.store[1],f.compute[0],f.compute[1],a,false));
		assert(f.io.first.code[0]==0x0006a06f && f.io.second.code[0]==0x0006a06f);
	}
	assert(resets==f.io.first.starts+f.io.second.starts && requests==f.io.first.requests+f.io.second.requests);
	f.Close();
	for (unsigned fault=0;fault<8;fault++) {
		Fixture trial; PioneerAttentionTileLease attention; attention.saved=attention.ready=true;
		unsigned writes=trial.io.first.Mock::writes;
		if (fault==0) trial.io.failSelect=trial.io.selects+1;
		if (fault==1) trial.io.failSelect=trial.io.selects+2;
		if (fault==2) trial.io.failSelect=trial.io.selects+3;
		if (fault==3) trial.io.first.reset=0x46800;
		if (fault==4) trial.io.second.reset=0x46800;
		if (fault==5) trial.io.first.code[0]^=1;
		if (fault==6) trial.io.second.code[0]=TT_ATTENTION_TILE_BOOT;
		if (fault==7) trial.io.first.Mock::drop=writes+1;
		assert(!PioneerRuntimeAttentionSelect(trial.io,b,n,n2,trial.store[0],trial.store[1],trial.compute[0],trial.compute[1],attention,true));
		assert(attention.unsafe && trial.compute[0].unsafe && trial.compute[1].unsafe && trial.store[0].recovery_required
			&& trial.store[1].recovery_required && trial.io.unsafe && trial.io.first.quarantined && trial.io.second.quarantined);
		assert(!trial.io.first.starts && !trial.io.second.starts && !trial.io.first.stops && !trial.io.second.stops);
		unsigned after=trial.io.first.Mock::writes+trial.io.second.Mock::writes;
		assert(!PioneerRuntimeAttentionSelect(trial.io,b,n,n2,trial.store[0],trial.store[1],trial.compute[0],trial.compute[1],attention,false));
		assert(after==trial.io.first.Mock::writes+trial.io.second.Mock::writes);
	}
	puts("PASS: OFFLINE shared entry switch; both peers held/quiet, fixed programs only, no DMA/reset, both quarantined after mapping/entry/lost-write uncertainty");
}
