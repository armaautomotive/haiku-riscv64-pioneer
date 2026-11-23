/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_ARC_QUEUE_MODEL_ONLY
#include "pioneer_q38_arc_queue_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerFirmwareReset.h"

struct ResetIO : StrictQueue {
	bool permitted=true,pciReady=true,notificationReady=true,dropMarker=false;
	bool dropPublication=false,dropNotification=false,published=false,complete=false,losePermitOnDelay=false;
	bool driftDescriptor=false,driftQueue=false;
	unsigned prepared=0,markerWrites=0,publications=0,bells=0,resetWrites=0,dropResetWrite=0,dropPreparedTlb=0;
	uint32_t command=0x402,cardVendor=0xb1401e52,rootVendor=0x20421e30;
	uint32_t rootClass=0x06040000,rootCommand=0x402;
	using StrictQueue::StrictQueue;
	bool RecoveryPermitted() { return permitted; }
	bool PciReady() { return pciReady; }
	bool NotificationReady() {
		++prepared;
		if (dropPreparedTlb) dropTlb=tlbWrites+dropPreparedTlb;
		if (driftDescriptor) At(0x10001004)=0x405;
		if (driftQueue) At(base)=At(base+16)=(At(base)+1)%8;
		return notificationReady;
	}
	uint32_t TlbRead(unsigned w) { assert(!published);return StrictQueue::TlbRead(w); }
	void TlbWrite(unsigned w,uint32_t v) { assert(!published);StrictQueue::TlbWrite(w,v); }
	uint32_t CsmRead(uint32_t a) { assert(!published);return StrictQueue::CsmRead(a); }
	void SetQueueBounds(uint32_t b,uint32_t q,uint32_t r) {
		assert(!published);StrictQueue::SetQueueBounds(b,q,r);
	}
	void QueueWrite(uint32_t a,uint32_t v) {
		assert(!published);
		if (prepared && a==base) {
			assert(command==0x442 && At(request)==0x56 && At(request+4)==3);
			for (unsigned w=2;w<8;w++) assert(At(request+w*4)==0);
			++publications;if (!dropPublication) StrictQueue::QueueWrite(a,v);
			published=true;return;
		}
		if (prepared && a>=request && a<request+32 && ++resetWrites==dropResetWrite) return;
		StrictQueue::QueueWrite(a,v);
	}
	uint32_t MsiStatusRead() { assert(!published);return StrictQueue::MsiStatusRead(); }
	void NotifyArc() { assert(!published);StrictQueue::NotifyArc(); }
	void NotifyPreparedArc() { assert(published && publications==1);if (!dropNotification) ++bells; }
	void RecoveryDelay() { assert(published);if (complete) command=0x402;if (losePermitOnDelay) permitted=false; }
	uint32_t CardRead(uint16_t a,uint8_t size) {
		if (a==0 && size==4) return cardVendor;
		if (a==4 && size==2) return command;
		assert(false);return 0;
	}
	uint32_t RootRead(uint16_t a,uint8_t size) {
		if (a==0 && size==4) return rootVendor;
		if (a==8 && size==4) return rootClass;
		if (a==4 && size==2) return rootCommand;
		assert(false);return 0;
	}
	void CardWrite(uint16_t a,uint8_t size,uint32_t v) {
		assert(!published && a==4 && size==2 && v==0x442);
		++markerWrites;if (!dropMarker) command=v;
	}
};

int main()
{
	const auto arc=ResetIO::Arc();const auto health=ResetIO::Health();
	for (unsigned req=0;req<8;req++) for (unsigned resp=0;resp<8;resp++) {
		ResetIO io(req,resp,0);PioneerFirmwareReset<ResetIO> reset(io);
		assert(reset.Start(arc,health));
		assert(reset.Result().stage==PioneerFirmwareResetPublished && io.publications==1 && io.bells==1);
		assert(reset.Result().nextRequest==(req+2)%8 && reset.Result().commandBefore==0x402);
		assert(!reset.Start(arc,health) && io.publications==1 && io.markerWrites==1);
		assert(!reset.ResumeWait(2) && reset.Result().polls==2);
		io.complete=true;
		assert(reset.ResumeWait(2) && reset.Result().stage==PioneerFirmwareResetComplete);
		assert(reset.ResumeWait(1) && io.publications==1 && io.bells==1);
	}
	for (unsigned drop=1;drop<=8;drop++) {
		ResetIO io(0,0,0);io.dropResetWrite=drop;PioneerFirmwareReset<ResetIO> reset(io);
		assert(!reset.Start(arc,health) && reset.Result().failedStage==PioneerFirmwareResetStageRequest);
		assert(!io.markerWrites && !io.publications && !reset.Start(arc,health));
	}
	for (unsigned drop=1;drop<=3;drop++) {
		ResetIO io(0,0,0);io.dropPreparedTlb=drop;PioneerFirmwareReset<ResetIO> reset(io);
		assert(!reset.Start(arc,health) && reset.Result().failedStage==PioneerFirmwareResetProbe);
		assert(!io.markerWrites && !io.publications && !reset.Start(arc,health));
	}
	for (unsigned fault=0;fault<9;fault++) {
		ResetIO io(0,0,0);
		if (fault==0) io.permitted=false;
		else if (fault==1) io.pciReady=false;
		else if (fault==2) io.notificationReady=false;
		else if (fault==3) io.timeout=true;
		else if (fault==4) io.dropMarker=true;
		else if (fault==5) io.driftDescriptor=true;
		else if (fault==6) io.driftQueue=true;
		else if (fault==7) io.command=0x406;
		else io.rootCommand=0x406;
		PioneerFirmwareReset<ResetIO> reset(io);
		assert(!reset.Start(arc,health) && reset.Result().stage==PioneerFirmwareResetFailed);
		assert(!io.publications && !reset.Start(arc,health) && !reset.ResumeWait(1));
	}
	for (unsigned fault=0;fault<2;fault++) {
		ResetIO io(0,0,0);io.dropPublication=fault==0;io.dropNotification=fault==1;
		PioneerFirmwareReset<ResetIO> reset(io);
		assert(reset.Start(arc,health) && !reset.ResumeWait(3));
		assert(reset.Result().stage==PioneerFirmwareResetWaiting && io.publications==1);
		assert(!reset.Start(arc,health) && !reset.ResumeWait(1) && io.publications==1);
	}
	ResetIO io(0,0,0);PioneerFirmwareReset<ResetIO> reset(io);
	assert(reset.Start(arc,health));io.permitted=false;
	assert(!reset.ResumeWait(1) && reset.Result().polls==0 && io.publications==1);
	ResetIO changed(0,0,0);PioneerFirmwareReset<ResetIO> changedReset(changed);
	assert(changedReset.Start(arc,health));changed.losePermitOnDelay=true;changed.complete=true;
	assert(!changedReset.ResumeWait(1) && changedReset.Result().polls==0 && changed.publications==1);
	for (unsigned fault=0;fault<4;fault++) {
		ResetIO bad(0,0,0);PioneerFirmwareReset<ResetIO> r(bad);
		assert(r.Start(arc,health));bad.complete=true;
		if (fault==0) bad.cardVendor=UINT32_MAX;
		else if (fault==1) bad.rootVendor=UINT32_MAX;
		else if (fault==2) bad.rootClass=0;
		else bad.rootCommand=0x406;
		assert(!r.ResumeWait(1) && r.Result().stage==PioneerFirmwareResetWaiting && bad.publications==1);
	}
	puts("PASS one-shot firmware-reset queue sequencing and marker polling; MOCK ONLY, NO HARDWARE BINDING");
}
