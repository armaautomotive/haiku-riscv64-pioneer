/* SPDX-License-Identifier: MIT */
#define PIONEER_ARC_QUEUE_MODEL_ONLY
#include "pioneer_arc_queue_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ArcQueue.h"
#include <cstdio>
#include <cstring>

struct StrictQueue:MockQueue {
	unsigned io=0;
	bool effectiveDroppedTlb=false;
	using MockQueue::MockQueue;
	uint32_t TlbRead(unsigned w) { io++; return MockQueue::TlbRead(w); }
	void TlbWrite(unsigned w,uint32_t v) {
		io++; if (tlbWrites+1==dropTlb) effectiveDroppedTlb=tlb[w]!=v;
		MockQueue::TlbWrite(w,v);
	}
	uint32_t CsmRead(uint32_t a) { io++; return MockQueue::CsmRead(a); }
	void SetQueueBounds(uint32_t b,uint32_t q,uint32_t r) { io++; MockQueue::SetQueueBounds(b,q,r); }
	void QueueWrite(uint32_t a,uint32_t v) { io++; MockQueue::QueueWrite(a,v); }
	uint32_t MsiStatusRead() { io++; return MockQueue::MsiStatusRead(); }
	void NotifyArc() { io++; MockQueue::NotifyArc(); }
	void Pause() { io++; MockQueue::Pause(); }
};

[[maybe_unused]] static void Inert(StrictQueue& io,PioneerQ38ArcQueue<StrictQueue>& queue,tt_arc_queue_result& r)
{
	assert(queue.Unsafe() && r.recovery_required);
	const unsigned before=io.io; const auto saved=r;
	for (unsigned message:{0x34U,0x52U,0x54U}) {
		assert(!queue.Message(io.Arc(),io.Health(),r,message) && io.io==before);
		assert(!memcmp(&saved,&r,sizeof(r))); // no result/authority reset either
	}
	queue.Quarantine(); assert(io.io==before);
}

#ifndef PIONEER_Q38_ARC_QUEUE_MODEL_ONLY
int main()
{
	const auto arc=StrictQueue::Arc(); const auto health=StrictQueue::Health();
	unsigned writes=0,tlbWrites=0;
	for (unsigned req=0;req<8;req++) for (unsigned resp=0;resp<8;resp++) {
		StrictQueue io(req,resp); PioneerQ38ArcQueue<StrictQueue> queue(io); tt_arc_queue_result r;
		assert(queue.Message(arc,health,r,0x34) && !queue.Unsafe() && io.Restored());
		assert(r.submitted && r.responded && r.acknowledged && r.request_restored && r.tlb_restored && !r.recovery_required);
		assert(r.clock_mhz==800 && r.clock_mode==3 && r.after[0]==(req+1)%8 && r.after[1]==(resp+1)%8);
		assert(r.before[6]==UINT32_MAX && !r.after[6]);
		for (unsigned w=0;w<3;w++) assert(io.tlb[w]==w+1);
		writes=io.writes; tlbWrites=io.tlbWrites;
	}
	for (unsigned drop=1;drop<=writes;drop++) {
		StrictQueue io; io.dropWrite=drop; PioneerQ38ArcQueue<StrictQueue> queue(io); tt_arc_queue_result r;
		assert(!queue.Message(arc,health,r,0x34));
		assert(io.writes==drop && io.tlbWrites==(drop<=9?9U:15U)); // NO later restoration writes
		if (drop<=8) assert(!r.submitted && !r.acknowledged);
		else if (drop<=10) assert(r.submitted && !r.acknowledged);
		else assert(r.acknowledged && !r.request_restored);
		Inert(io,queue,r);
	}
	for (unsigned drop=1;drop<=tlbWrites;drop++) {
		StrictQueue io; io.dropTlb=drop; PioneerQ38ArcQueue<StrictQueue> queue(io); tt_arc_queue_result r;
		const bool okay=queue.Message(arc,health,r,0x34);
		if (io.effectiveDroppedTlb) {
			assert(!okay && io.tlbWrites==drop && !r.tlb_restored); Inert(io,queue,r);
		} else {
			// A dropped write of the already-exact value is unobservable and
			// harmless; it does NOT permit retrying a mismatching readback.
			assert(okay && !queue.Unsafe() && r.tlb_restored && io.Restored());
		}
	}
	for (unsigned fault=0;fault<6;fault++) {
		StrictQueue io;
		if (!fault) io.timeout=true; else if (fault==1) io.badReply=true;
		else if (fault==2) io.changed=true; else if (fault==3) io.failRestore=true;
		else if (fault==4) io.msiStatus=0; else io.At(0x10001004)=0x405;
		PioneerQ38ArcQueue<StrictQueue> queue(io); tt_arc_queue_result r;
		assert(!queue.Message(arc,health,r,0x34)); Inert(io,queue,r);
		if (fault<3) assert(io.writes==9 && io.tlbWrites==15 && !r.acknowledged);
		if (!fault) assert(r.polls==2000);
		if (fault==3) assert(io.writes==11 && io.tlbWrites==15 && r.acknowledged);
		if (fault>=4) assert(!io.writes && !io.notifications);
	}
	for (unsigned message:{0U,0x33U,0x53U,0xffffffffU}) {
		StrictQueue io; PioneerQ38ArcQueue<StrictQueue> queue(io); tt_arc_queue_result r;
		assert(!queue.Message(arc,health,r,message) && !queue.Unsafe() && !io.io);
	}
	for (unsigned field:{0U,6U,7U,8U,9U}) {
		StrictQueue io; auto bad=health; bad.values[field]^=1;
		PioneerQ38ArcQueue<StrictQueue> queue(io); tt_arc_queue_result r;
		assert(!queue.Message(arc,bad,r,0x34) && !queue.Unsafe() && !io.io);
	}
	// Normal busy/query/idle messages on the SAME owner; no forced clock.
	StrictQueue io; PioneerQ38ArcQueue<StrictQueue> queue(io); tt_arc_queue_result r;
	for (unsigned message:{0x34U,0x52U,0x34U,0x54U,0x34U}) {
		assert(queue.Message(arc,health,r,message) && io.Restored());
		assert(r.clock_mhz==(message==0x34?io.clock:0) && io.mode==3);
	}
	assert(io.clock==800 && io.notifications==5);
	queue.Quarantine(); const unsigned before=io.io;
	assert(!queue.Message(arc,health,r,0x54) && io.io==before); // external compute UNKNOWN
	puts("PASS strict private ARC queue:64 pointer-wrap combinations,normal busy/query/idle,ALL dropped queue/TLB writes,timeout/response/ownership/restore faults STOP at first detection with NO later IO/retry/ACK/restore;bad metadata ZERO IO;OFFLINE UNSELECTED");
}
#endif
