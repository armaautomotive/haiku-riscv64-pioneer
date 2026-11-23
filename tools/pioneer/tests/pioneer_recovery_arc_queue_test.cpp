/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_ARC_QUEUE_MODEL_ONLY
#include "pioneer_q38_arc_queue_test.cpp"

int main()
{
	const auto arc=StrictQueue::Arc();const auto health=StrictQueue::Health();
	unsigned totalWrites=0,totalTlbWrites=0;
	for (unsigned req=0;req<8;req++) for (unsigned resp=0;resp<8;resp++) {
		StrictQueue io(req,resp,0);PioneerQ38ArcQueue<StrictQueue,true> queue(io);
		tt_arc_queue_result result{};
		assert(queue.Message(arc,health,result,0xa0) && !queue.Unsafe() && io.Restored());
		assert(result.base==0x10029ea8 && result.submitted && result.responded
			&& result.acknowledged && result.request_restored && result.tlb_restored);
		totalWrites=io.writes;totalTlbWrites=io.tlbWrites;
	}
	for (unsigned drop=1;drop<=totalWrites;drop++) {
		StrictQueue io(0,0,0);io.dropWrite=drop;
		PioneerQ38ArcQueue<StrictQueue,true> queue(io);tt_arc_queue_result result{};
		assert(!queue.Message(arc,health,result,0xa0) && queue.Unsafe() && result.recovery_required);
		const unsigned before=io.io;
		assert(!queue.Message(arc,health,result,0xa0) && io.io==before);
	}
	StrictQueue payload(0,0,0);payload.statePayload=true;
	PioneerQ38ArcQueue<StrictQueue,true> payloadQueue(payload);tt_arc_queue_result payloadResult{};
	assert(payloadQueue.Message(arc,health,payloadResult,0xa0) && payloadResult.status==0);
	for (unsigned fault=0;fault<4;fault++) {
		StrictQueue io(0,0,0);
		if (!fault) io.timeout=true;else if (fault==1) io.badReply=true;
		else if (fault==2) io.changed=true;else io.msiStatus=0;
		PioneerQ38ArcQueue<StrictQueue,true> queue(io);tt_arc_queue_result result{};
		assert(!queue.Message(arc,health,result,0xa0) && queue.Unsafe() && result.recovery_required);
	}
	for (unsigned message:{0U,0x34U,0x52U,0x54U,0xffffffffU}) {
		StrictQueue io(0,0,0);PioneerQ38ArcQueue<StrictQueue,true> queue(io);
		tt_arc_queue_result result{};
		assert(!queue.Message(arc,health,result,message) && !io.io && !queue.Unsafe());
	}
	StrictQueue app;PioneerQ38ArcQueue<StrictQueue> appQueue(app);tt_arc_queue_result result{};
	assert(!appQueue.Message(arc,health,result,0xa0) && !app.io && !appQueue.Unsafe());
	assert(!appQueue.Message(arc,health,result,0x90) && !app.io && !appQueue.Unsafe());
	for (unsigned req=0;req<8;req++) for (unsigned resp=0;resp<8;resp++) {
		StrictQueue io(req,resp,0);PioneerQ38ArcQueue<StrictQueue,false,true> queue(io);
		tt_arc_queue_result probe{};
		assert(queue.Message(arc,health,probe,0x90) && !queue.Unsafe() && io.Restored());
		assert(probe.base==0x10029ea8 && probe.submitted && probe.responded
			&& probe.acknowledged && probe.request_restored && probe.tlb_restored);
	}
	for (unsigned drop=1;drop<=totalWrites;drop++) {
		StrictQueue io(0,0,0);io.dropWrite=drop;
		PioneerQ38ArcQueue<StrictQueue,false,true> queue(io);tt_arc_queue_result probe{};
		assert(!queue.Message(arc,health,probe,0x90) && queue.Unsafe() && probe.recovery_required);
		const unsigned before=io.io;
		assert(!queue.Message(arc,health,probe,0x90) && io.io==before);
	}
	for (unsigned drop=1;drop<=totalTlbWrites;drop++) {
		StrictQueue io(0,0,0);io.dropTlb=drop;
		PioneerQ38ArcQueue<StrictQueue,false,true> queue(io);tt_arc_queue_result probe{};
		const bool okay=queue.Message(arc,health,probe,0x90);
		if (io.effectiveDroppedTlb) {
			assert(!okay && queue.Unsafe() && probe.recovery_required && io.tlbWrites==drop);
			const unsigned before=io.io;
			assert(!queue.Message(arc,health,probe,0x90) && io.io==before);
		} else assert(okay && !queue.Unsafe() && io.Restored());
	}
	StrictQueue pingPayload(0,0,0);pingPayload.statePayload=true;
	PioneerQ38ArcQueue<StrictQueue,false,true> pingQueue(pingPayload);tt_arc_queue_result pingResult{};
	assert(pingQueue.Message(arc,health,pingResult,0x90) && pingResult.status==0 && pingResult.clock_mhz==0x1234);
	for (unsigned fault=0;fault<4;fault++) {
		StrictQueue io(0,0,0);
		if (!fault) io.timeout=true;else if (fault==1) io.badReply=true;
		else if (fault==2) io.changed=true;else io.msiStatus=0;
		PioneerQ38ArcQueue<StrictQueue,false,true> queue(io);tt_arc_queue_result probe{};
		assert(!queue.Message(arc,health,probe,0x90) && queue.Unsafe() && probe.recovery_required);
	}
	for (unsigned message:{0U,0x34U,0x56U,0xa0U,0xffffffffU}) {
		StrictQueue io(0,0,0);PioneerQ38ArcQueue<StrictQueue,false,true> queue(io);
		tt_arc_queue_result probe{};
		assert(!queue.Message(arc,health,probe,message) && !io.io && !queue.Unsafe());
	}
	puts("PASS recovery ARC state0 and firmware liveness on kernel queue 0: wraparound, dropped writes, timeout and opcode admission; SIMULATED ONLY");
}
