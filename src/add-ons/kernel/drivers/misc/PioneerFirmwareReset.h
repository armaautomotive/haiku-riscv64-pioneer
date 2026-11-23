/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_FIRMWARE_RESET_H
#define PIONEER_FIRMWARE_RESET_H

#include "PioneerQ38ArcQueue.h"

// Experimental transport contract, not an ioctl or a hardware binding. The
// caller must hold the exclusive device/recovery ticket and retain all host
// and device mappings on every failure. NotificationReady must configure and
// verify a *separate* notification aperture; queue TLB 201 must remain mapped
// to CSM while the request pointer is published. No callback may retry an
// uncertain write. QueueWrite and NotifyPreparedArc must each perform at most
// one fixed MMIO write. Notification uses the separate, already-prepared
// aperture and must perform no reads or mapping changes. Neither proves reset.
enum PioneerFirmwareResetStage {
	PioneerFirmwareResetFresh=0,
	PioneerFirmwareResetProbe=1,
	PioneerFirmwareResetStageRequest=2,
	PioneerFirmwareResetMarker=3,
	PioneerFirmwareResetPublished=4,
	PioneerFirmwareResetWaiting=5,
	PioneerFirmwareResetComplete=6,
	PioneerFirmwareResetFailed=7
};

struct PioneerFirmwareResetResult {
	unsigned stage=PioneerFirmwareResetFresh;
	unsigned failedStage=PioneerFirmwareResetFresh;
	uint32_t queue=0,request=0,nextRequest=0;
	uint32_t commandBefore=0,markerAfter=0,polls=0;
	tt_arc_queue_result probe={};
};

template<class IO> class PioneerFirmwareReset {
	IO& io;
	PioneerQ38ArcQueue<IO,false,true> probe;
	PioneerFirmwareResetResult result={};
	bool Fail() {
		result.failedStage=result.stage;result.stage=PioneerFirmwareResetFailed;
		return false;
	}
	bool ConfigureQueue() {
		for (unsigned w=0;w<3;w++) {
			if (!io.RecoveryPermitted()) return false;
			const uint32_t value=BlackholeArcQueueTlbWord(false,w);
			io.TlbWrite(w,value);
			if (io.TlbRead(w)!=value) return false;
		}
		return true;
	}
public:
	explicit PioneerFirmwareReset(IO& transport):io(transport),probe(transport) {}
	PioneerFirmwareReset(const PioneerFirmwareReset&)=delete;
	PioneerFirmwareReset& operator=(const PioneerFirmwareReset&)=delete;
	const PioneerFirmwareResetResult& Result() const { return result; }
	bool BoundTo(const IO& transport) const { return &io==&transport; }

	// A true result means only that publication and notification were
	// attempted once. Firmware may not have received either operation.
	bool Start(const tt_arc_scratch_result& arc,const tt_arc_telemetry_result& health) {
		if (result.stage!=PioneerFirmwareResetFresh) return false;
		result.stage=PioneerFirmwareResetProbe;
		if (!io.RecoveryPermitted() || !io.PciReady()
			|| !probe.Message(arc,health,result.probe,0x90)
			|| !io.RecoveryPermitted() || !io.PciReady()
			|| !io.NotificationReady() || !ConfigureQueue()) return Fail();
		result.stage=PioneerFirmwareResetStageRequest;
		uint32_t descriptor[4];
		for (unsigned w=0;w<4;w++) descriptor[w]=io.CsmRead(arc.message_queue+w*4);
		if (descriptor[0]!=result.probe.descriptor[0] || descriptor[1]!=0x404
			|| descriptor[2] || descriptor[3]
			|| !BlackholeArcQueueRange(descriptor[0],4*288)) return Fail();
		result.queue=descriptor[0];
		uint32_t before[8],current[8];
		BlackholeArcQueueHeader(io,result.queue,before);
		io.Pause();BlackholeArcQueueHeader(io,result.queue,current);
		if (!BlackholeArcQueueHeaderValid(before) || !BlackholeArcQueueSame(before,current)
			|| before[0]!=before[4] || before[1]!=before[5]
			|| before[0]!=result.probe.after[0] || before[1]!=result.probe.after[1]
			|| before[6]!=result.probe.after[6]) return Fail();
		result.request=result.queue+32+(before[0]%4)*32;
		result.nextRequest=(before[0]+1)%8;
		io.SetQueueBounds(result.queue,result.request,result.queue+160+(before[1]%4)*32);
		// The opcode and argument are fixed by the private recovery path. Do
		// not offer arbitrary ARC messages or reset parameters to userspace.
		for (unsigned w=0;w<8;w++) {
			if (!io.RecoveryPermitted()) return Fail();
			const uint32_t value=w==0?0x56U:w==1?3U:0U;
			io.QueueWrite(result.request+w*4,value);
			if (io.CsmRead(result.request+w*4)!=value) return Fail();
		}
		BlackholeArcQueueHeader(io,result.queue,current);
		if (!BlackholeArcQueueSame(before,current) || !io.RecoveryPermitted()
			|| !io.PciReady()) return Fail();
		result.stage=PioneerFirmwareResetMarker;
		const uint32_t command=io.CardRead(4,2);
		result.commandBefore=command;
		if (command!=0x402 || io.RootRead(4,2)!=0x402) return Fail();
		io.CardWrite(4,2,command|0x40);
		if (io.CardRead(4,2)!=(command|0x40) || !io.RecoveryPermitted()) return Fail();
		// Mark uncertain *before* publication. Do not touch the card again
		// here after the notification: reset can invalidate all MMIO mappings.
		result.stage=PioneerFirmwareResetPublished;
		io.QueueWrite(result.queue,result.nextRequest);
		io.NotifyPreparedArc();
		return true;
	}

	// Independent PCI-marker polling only. This never reissues the request,
	// acknowledges firmware, restores a TLB or accesses a pre-reset BAR.
	bool ResumeWait(unsigned limit) {
		if (!limit || (result.stage!=PioneerFirmwareResetPublished
			&& result.stage!=PioneerFirmwareResetWaiting
			&& result.stage!=PioneerFirmwareResetComplete)) return false;
		if (result.stage==PioneerFirmwareResetComplete) return true;
		result.stage=PioneerFirmwareResetWaiting;
		for (unsigned n=0;n<limit;n++) {
			if (!io.RecoveryPermitted()) return false;
			io.RecoveryDelay();
			if (!io.RecoveryPermitted()) return false;
			const uint32_t vendor=io.CardRead(0,4),marker=io.CardRead(4,2);
			const uint32_t root=io.RootRead(0,4),rootClass=io.RootRead(8,4);
			const uint32_t rootCommand=io.RootRead(4,2);
			if (result.polls!=UINT32_MAX) ++result.polls;
			result.markerAfter=marker;
			if (vendor==0xb1401e52 && root==0x20421e30 && (rootClass>>8)==0x060400
				&& marker!=UINT32_MAX && rootCommand!=UINT32_MAX
				&& !(marker&0x44) && !(rootCommand&4)) {
				result.stage=PioneerFirmwareResetComplete;return true;
			}
		}
		return false;
	}
};
#endif
