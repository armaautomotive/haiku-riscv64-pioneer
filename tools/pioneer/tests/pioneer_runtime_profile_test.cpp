/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimeProfile.h"
#include <cassert>
#include <cstring>
int main() {
	PioneerRuntimeProfile p;
	assert(p.Snapshot().version==1 && !p.Snapshot().available && !p.Snapshot().sequence && !p.Snapshot().mask);
	p.Mark(900,TT_PROFILE_COMPLETED); assert(!p.Snapshot().mask);
	p.Begin(1000,2); assert(p.Snapshot().sequence==1 && p.Snapshot().mask==1);
	for (unsigned s=1;s<=6;s++) p.Mark(1000+s*100,s);
	p.Mark(1700,TT_PROFILE_SLOTS); p.Mark(1800,TT_PROFILE_PIO);
	p.End(2000,1,0,1,1,1);
	auto result=p.Snapshot();
	assert(result.available && result.mask==0x17f && result.format==2 && result.state==1 && !result.recovery_required);
	assert(result.clock_stable && result.microseconds[TT_PROFILE_PIO]==500 && result.microseconds[TT_PROFILE_FINISHED]==1000);
	p.Mark(5000,TT_PROFILE_DMA); p.End(6000,7,1,1,0,0);
	assert(!memcmp(&result,&p.Snapshot(),sizeof(result))); // completed evidence is immutable
	p.Begin(3000,1); p.Mark(3100,TT_PROFILE_PREPARED); p.Mark(3200,TT_PROFILE_STAGED);
	p.End(4000,3,1,0,0,0);
	assert(p.Snapshot().sequence==2 && p.Snapshot().mask==0x107 && p.Snapshot().state==3 && p.Snapshot().recovery_required);
	assert(!p.Snapshot().started && !p.Snapshot().completed && !p.Snapshot().stopped);
	p.Begin(5000,0); p.Mark(4999,TT_PROFILE_PREPARED); p.End(5500,2,0,0,0,0);
	assert(!p.Snapshot().clock_stable && p.Snapshot().mask==0x101);
	p.Begin(8000,0); p.Mark(8300,TT_PROFILE_PREPARED); p.Mark(8200,TT_PROFILE_STAGED);
	p.End(9000,2,0,0,0,0); assert(!p.Snapshot().clock_stable && !(p.Snapshot().mask&(1<<TT_PROFILE_STAGED)));
}
