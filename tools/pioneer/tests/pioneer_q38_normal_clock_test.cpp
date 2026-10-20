/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_ARC_QUEUE_MODEL_ONLY
#include "pioneer_q38_arc_queue_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NormalClock.h"

int main()
{
	const auto arc=StrictQueue::Arc(); const auto health=StrictQueue::Health();
	unsigned queueWrites=0,tlbWrites=0;
	{
		StrictQueue io; PioneerQ38NormalClock<StrictQueue> owner(io);
		assert(!owner.Finish(arc,health) && !io.io);
		assert(owner.Enter(arc,health) && owner.Active() && io.clock==1350 && io.mode==3);
		assert(owner.Snapshot().messages==3 && owner.Snapshot().before_mhz==800 && owner.Snapshot().busy_mhz==1350);
		const unsigned before=io.io; assert(!owner.Enter(arc,health) && io.io==before);
		assert(owner.Finish(arc,health) && !owner.Active() && !owner.Unsafe());
		assert(owner.Snapshot().state==TT_ARC_CLOCK_RESTORED && owner.Snapshot().messages==5 && io.clock==800 && io.Restored());
		const unsigned done=io.io; assert(owner.Finish(arc,health) && !owner.Enter(arc,health) && io.io==done);
		queueWrites=io.writes; tlbWrites=io.tlbWrites;
	}
	for (unsigned drop=1;drop<=queueWrites;drop++) {
		StrictQueue io; io.dropWrite=drop; PioneerQ38NormalClock<StrictQueue> owner(io);
		const bool entered=owner.Enter(arc,health);
		if (!owner.Unsafe()) assert(entered && !owner.Finish(arc,health));
		assert(owner.Unsafe() && io.writes==drop);
		const unsigned before=io.io; assert(!owner.Finish(arc,health) && !owner.Enter(arc,health) && io.io==before);
	}
	for (unsigned drop=1;drop<=tlbWrites;drop++) {
		StrictQueue io; io.dropTlb=drop; PioneerQ38NormalClock<StrictQueue> owner(io);
		const bool entered=owner.Enter(arc,health);
		if (!owner.Unsafe()) assert(entered);
		if (!owner.Unsafe()) owner.Finish(arc,health);
		if (io.effectiveDroppedTlb) {
			assert(owner.Unsafe() && io.tlbWrites==drop);
			const unsigned before=io.io; assert(!owner.Finish(arc,health) && !owner.Enter(arc,health) && io.io==before);
		} else assert(!owner.Unsafe() && io.clock==800 && io.Restored());
	}
	for (unsigned fault=0;fault<6;fault++) {
		StrictQueue io; PioneerQ38NormalClock<StrictQueue> owner(io);
		assert(owner.Enter(arc,health));
		if (!fault) owner.Quarantine(); // external compute UNKNOWN;NO idle message
		else if (fault==1) io.timeout=true; else if (fault==2) io.badReply=true;
		else if (fault==3) io.changed=true; else if (fault==4) io.failRestore=true;
		else io.ignoreIdle=true;
		const unsigned before=io.io;
		assert(!owner.Finish(arc,health) && owner.Unsafe());
		if (!fault) assert(io.io==before && io.notifications==3);
		const unsigned after=io.io; assert(!owner.Finish(arc,health) && !owner.Enter(arc,health) && io.io==after);
	}
	{
		StrictQueue io; io.ignoreBusy=true; PioneerQ38NormalClock<StrictQueue> owner(io);
		assert(!owner.Enter(arc,health) && !owner.Unsafe() && owner.Active());
		assert(owner.Snapshot().messages==18 && owner.Finish(arc,health) && !owner.Active() && io.clock==800);
	}
	for (unsigned temperature:{0U,70U*65536+1,UINT32_MAX}) {
		StrictQueue io; auto hot=health; hot.values[2]=temperature;
		PioneerQ38NormalClock<StrictQueue> owner(io);
		assert(!owner.Enter(arc,hot) && !owner.Unsafe() && !io.io);
		assert(owner.Finish(arc,hot) && !io.io);
	}
	for (unsigned mode:{1U,2U}) {
		StrictQueue io; io.mode=mode; PioneerQ38NormalClock<StrictQueue> owner(io);
		assert(!owner.Enter(arc,health) && !owner.Active() && !owner.Unsafe() && io.notifications==1);
		const unsigned before=io.io; assert(owner.Finish(arc,health) && io.io==before);
	}
	puts("PASS OFFLINE strict normal-clock owner:800->1350->800,all queue/TLB write faults,thermal/mode refusal,known busy-not-settled finish,missing idle/compute UNKNOWN retain with NO later IO or second cleanup;NO driver selection or native clock change");
}
