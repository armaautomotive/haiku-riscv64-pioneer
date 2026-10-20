/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_ARC_QUEUE_H
#define PIONEER_Q38_ARC_QUEUE_H
#include "BlackholeArcQueueProbe.h"
// Private transport for default-OFF PIONEER_Q38_NORMAL_CLOCK ownership.
// Unlike the older diagnostic, ANY unexpected transport state
// stops immediately: no acknowledgement, restore, second attempt or later IO.
// A native caller MUST retain its context/cookie/PCI/ALL mappings and host
// leases, and latch cold recovery. This object alone cannot do that.
template<class IO> class PioneerQ38ArcQueue {
	IO& io;
	bool unsafe=false;
	bool Fault(tt_arc_queue_result& r) { unsafe=true; r.recovery_required=1; return false; }
	bool Configure(bool apb) {
		for (unsigned w=0;w<3;w++) {
			const uint32_t word=BlackholeArcQueueTlbWord(apb,w);
			io.TlbWrite(w,word); if (io.TlbRead(w)!=word) return false;
		}
		return true;
	}
public:
	explicit PioneerQ38ArcQueue(IO& transport):io(transport) {}
	PioneerQ38ArcQueue(const PioneerQ38ArcQueue&)=delete;
	PioneerQ38ArcQueue& operator=(const PioneerQ38ArcQueue&)=delete;
	bool Unsafe() const { return unsafe; }
	void Quarantine() { unsafe=true; } // MEMORY ONLY after external uncertainty
	bool Message(const tt_arc_scratch_result& arc,const tt_arc_telemetry_result& health,
		tt_arc_queue_result& r,uint32_t message) {
		if (unsafe) return false; // even the result stays unchanged after fault
		r={}; r.version=1; r.request_restored=r.tlb_restored=1;
		if ((message!=0x34 && message!=0x52 && message!=0x54)
			|| arc.version!=1 || arc.state!=TT_ARC_SCRATCH_READY || arc.tlb_restored!=1
			|| health.version!=1 || health.state!=TT_ARC_TELEMETRY_OK || health.tlb_restored!=1
			|| (health.present_mask&0x3c1)!=0x3c1 || health.values[0]!=0x12050001
			|| health.values[6]!=0xf7 || health.values[7]!=0x5515 || health.values[8] || health.values[9]!=0xfff
			|| !health.entry_count || health.entry_count>128
			|| !BlackholeTelemetryRange(health.tags_address,8+health.entry_count*4)
			|| !BlackholeTelemetryRange(health.data_address,health.entry_count*4)
			|| !BlackholeArcQueueRange(arc.message_queue,16)) return false; // ZERO IO
		for (unsigned w=0;w<3;w++) {
			r.tlb_before[w]=io.TlbRead(w);
			if (io.TlbRead(w)!=r.tlb_before[w]) return Fault(r);
		}
		if (r.tlb_before[2]&0xffffc000) return Fault(r);
		r.tlb_restored=0; r.state=TT_ARC_QUERY_BAD_DESCRIPTOR;
		if (!Configure(false)) return Fault(r);
		for (unsigned w=0;w<4;w++) r.descriptor[w]=io.CsmRead(arc.message_queue+w*4);
		const uint32_t allBase=r.descriptor[0];
		if (r.descriptor[1]!=0x404 || r.descriptor[2] || r.descriptor[3]
			|| !BlackholeArcQueueRange(allBase,4*288)
			|| BlackholeArcQueueOverlap(allBase,4*288,arc.message_queue,16)
			|| BlackholeArcQueueOverlap(allBase,4*288,health.tags_address,8+health.entry_count*4)
			|| BlackholeArcQueueOverlap(allBase,4*288,health.data_address,health.entry_count*4)) return Fault(r);
		r.state=TT_ARC_QUERY_NOTIFY_FAILED;
		if (!Configure(true)) return Fault(r);
		r.msi_status=io.MsiStatusRead();
		if ((r.msi_status&0x301)!=1) return Fault(r);
		if (!Configure(false)) return Fault(r);
		for (unsigned w=0;w<4;w++) if (io.CsmRead(arc.message_queue+w*4)!=r.descriptor[w]) return Fault(r);
		r.base=allBase+3*288; r.state=TT_ARC_QUERY_BUSY;
		uint32_t current[8]; BlackholeArcQueueHeader(io,r.base,r.before);
		io.Pause(); BlackholeArcQueueHeader(io,r.base,current);
		if (!BlackholeArcQueueHeaderValid(r.before) || !BlackholeArcQueueSame(current,r.before)
			|| r.before[0]!=r.before[4] || r.before[1]!=r.before[5]) return Fault(r);
		r.request_address=r.base+32+(r.before[0]%4)*32;
		r.response_address=r.base+160+(r.before[1]%4)*32;
		io.SetQueueBounds(r.base,r.request_address,r.response_address);
		uint32_t saved[8];
		for (unsigned w=0;w<8;w++) {
			saved[w]=io.CsmRead(r.request_address+w*4);
			if (io.CsmRead(r.request_address+w*4)!=saved[w]) return Fault(r);
		}
		BlackholeArcQueueHeader(io,r.base,current);
		if (!BlackholeArcQueueSame(current,r.before)) return Fault(r);
		r.state=TT_ARC_QUERY_STAGE_FAILED; r.request_restored=0;
		for (unsigned w=0;w<8;w++) {
			const uint32_t value=w?0:message;
			io.QueueWrite(r.request_address+w*4,value);
			if (io.CsmRead(r.request_address+w*4)!=value) return Fault(r);
		}
		BlackholeArcQueueHeader(io,r.base,current);
		if (!BlackholeArcQueueSame(current,r.before)) return Fault(r);
		const uint32_t nextRequest=(r.before[0]+1)%8,nextResponse=(r.before[1]+1)%8;
		r.state=TT_ARC_QUERY_SUBMIT_FAILED; r.submitted=1; // BEFORE uncertain publication
		io.QueueWrite(r.base,nextRequest);
		if (io.CsmRead(r.base)!=nextRequest) return Fault(r);
		r.state=TT_ARC_QUERY_NOTIFY_FAILED;
		if (!Configure(true)) return Fault(r);
		r.msi_status=io.MsiStatusRead();
		if ((r.msi_status&0x301)!=1) return Fault(r);
		io.NotifyArc();
		if (!Configure(false)) return Fault(r);
		r.state=TT_ARC_QUERY_TIMEOUT;
		for (unsigned attempt=0;attempt<2000;attempt++) {
			r.polls++; BlackholeArcQueueHeader(io,r.base,current);
			if (!BlackholeArcQueueHeaderValid(current) || current[0]!=nextRequest || current[1]!=r.before[1]
				|| (current[4]!=r.before[4] && current[4]!=nextRequest)
				|| (current[5]!=r.before[5] && current[5]!=nextResponse)
				|| (current[6]!=r.before[6] && current[6]!=uint32_t(r.before[6]+1))) {
				r.state=TT_ARC_QUERY_CHANGED; return Fault(r);
			}
			if (current[4]==nextRequest && current[5]==nextResponse && current[6]==uint32_t(r.before[6]+1)) {
				r.responded=1; break;
			}
			io.Pause();
		}
		if (!r.responded) return Fault(r);
		uint32_t response[8]; for (unsigned w=0;w<8;w++) response[w]=io.CsmRead(r.response_address+w*4);
		r.status=response[0]; r.clock_mhz=response[1]; r.clock_mode=response[2];
		r.state=TT_ARC_QUERY_BAD_RESPONSE;
		if (r.status || (message==0x34?(r.clock_mhz<200 || r.clock_mhz>1400 || r.clock_mode<1 || r.clock_mode>3)
			:r.clock_mhz || r.clock_mode)) return Fault(r);
		for (unsigned w=3;w<8;w++) if (response[w]) return Fault(r);
		BlackholeArcQueueHeader(io,r.base,r.after);
		if (!BlackholeArcQueueSame(current,r.after)) { r.state=TT_ARC_QUERY_CHANGED; return Fault(r); }
		for (unsigned w=0;w<4;w++) if (io.CsmRead(arc.message_queue+w*4)!=r.descriptor[w]) return Fault(r);
		r.state=TT_ARC_QUERY_ACK_FAILED;
		io.QueueWrite(r.base+4,nextResponse);
		if (io.CsmRead(r.base+4)!=nextResponse) return Fault(r);
		current[1]=nextResponse;
		BlackholeArcQueueHeader(io,r.base,r.after);
		if (!BlackholeArcQueueSame(current,r.after)) return Fault(r);
		io.Pause(); BlackholeArcQueueHeader(io,r.base,r.after);
		if (!BlackholeArcQueueSame(current,r.after)) return Fault(r);
		r.acknowledged=1;
		// Known clean completion ONLY:single-attempt payload/TLB restoration.
		// No pointer rewind,firmware response write or retry exists here.
		r.state=TT_ARC_QUERY_RESTORE_FAILED;
		for (unsigned w=0;w<8;w++) {
			io.QueueWrite(r.request_address+w*4,saved[w]);
			if (io.CsmRead(r.request_address+w*4)!=saved[w]) return Fault(r);
		}
		BlackholeArcQueueHeader(io,r.base,current);
		if (!BlackholeArcQueueSame(current,r.after)) return Fault(r);
		r.request_restored=1;
		for (unsigned w=0;w<3;w++) {
			io.TlbWrite(w,r.tlb_before[w]);
			r.tlb_after[w]=io.TlbRead(w);
			if (r.tlb_after[w]!=r.tlb_before[w]) return Fault(r);
		}
		r.tlb_restored=1; r.state=TT_ARC_QUERY_PASSED; return true;
	}
};
#endif
