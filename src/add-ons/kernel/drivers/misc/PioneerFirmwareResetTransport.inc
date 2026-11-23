/* SPDX-License-Identifier: MIT */
// Experimental private transport for PioneerFirmwareReset. Not selected by
// any ioctl. Caller holds the driver lock and the exclusive recovery ticket,
// and retains this object, the mapping and the old session on ALL outcomes.
// In particular, there is deliberately no destructor-side unmap or TLB
// restoration. A future retirement path must first prove reset, fresh firmware,
// fleet and DMA recovery and then dispose virtual areas without touching the
// pre-reset device state.
#include "PioneerFirmwareReset.h"

template<class OwnerIO,class Guard> class PioneerFirmwareResetTransport {
	PioneerPostResetMapping& m;
	OwnerIO& owner;
	Guard* guard;
	uint64 ticket;
	area_id queueArea=-1,msiArea=-1,preparedArea=-1;
	volatile uint32* queue=NULL;
	volatile uint32* msi=NULL;
	volatile uint32* preparedMsi=NULL;
	uint32 queueBase=0,request=0;
	bool opened=false,openAttempted=false,prepareAttempted=false,prepared=false;
	bool publicationAttempted=false,notificationAttempted=false,delayFailed=false;
	bool Allowed() const {
		return ticket && guard && guard->validate && guard->validate(ticket)==B_OK && !delayFailed;
	}
	bool Accessible() { return opened && !publicationAttempted && Allowed() && m.PciReady(); }
	bool PublishAccessible() { return opened && !publicationAttempted && Allowed() && m.PciReady(true); }
public:
	PioneerFirmwareResetTransport(PioneerPostResetMapping& mapping,OwnerIO& io,Guard* g,uint64 t)
		:m(mapping),owner(io),guard(g),ticket(t) {}
	PioneerFirmwareResetTransport(const PioneerFirmwareResetTransport&)=delete;
	PioneerFirmwareResetTransport& operator=(const PioneerFirmwareResetTransport&)=delete;
	~PioneerFirmwareResetTransport() = default;

	bool Open() {
		if (openAttempted) return false;
		openAttempted=true;
		if (!Allowed() || !m.ready || !m.firmwareReady || !m.topologyReady
			|| !m.savedValid || m.queueAttempted || !m.PciReady()) return false;
		// Any partial mapping or later queue operation is retained; Close()
		// must not restore pre-reset TLBs or retire uncertain queue state.
		m.queueAttempted=true;m.queueUnsafe=true;
		void* address=NULL;
		queueArea=map_physical_memory("P100A recovery ARC queue",
			m.profile.bar0_host_start+0x19200000,TT_ARC_QUEUE_CSM_BYTES,
			B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,&address);
		if (queueArea<B_OK || !address) return false;
		queue=(volatile uint32*)address;address=NULL;
		msiArea=map_physical_memory("P100A recovery ARC probe notification",
			m.profile.bar0_host_start+0x192b0000,B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,&address);
		if (msiArea<B_OK || !address) return false;
		msi=(volatile uint32*)address;address=NULL;
		preparedArea=map_physical_memory("P100A recovery ARC reset notification",
			m.profile.bar0_host_start+0x190b0000,B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,&address);
		if (preparedArea<B_OK || !address) return false;
		preparedMsi=(volatile uint32*)address;
		opened=Allowed() && m.PciReady();return opened;
	}
	bool RecoveryPermitted() { return opened && Allowed(); }
	bool PciReady() { return Accessible(); }
	uint32 TlbRead(unsigned w) { return Accessible() && w<3?m.TlbRead(w):UINT32_MAX; }
	void TlbWrite(unsigned w,uint32 v) { if (Accessible() && w<3) m.TlbWrite(w,v); }
	uint32 CsmRead(uint32 a) {
		return Accessible() && BlackholeArcQueueRange(a,4)
			?PioneerRead32(queue+(a-0x10000000)/4):UINT32_MAX;
	}
	void SetQueueBounds(uint32 b,uint32 q,uint32) {
		if (!Accessible() || !BlackholeArcQueueRange(b,288)
			|| q<b+32 || q>b+128 || (q-b-32)%32) return;
		queueBase=b;request=q;
	}
	void QueueWrite(uint32 a,uint32 value) {
		const bool resetPublication=prepared && a==queueBase;
		if (!(resetPublication?PublishAccessible():Accessible())
			|| !queueBase || !BlackholeArcQueueRange(a,4)) return;
		if (a!=queueBase && a!=queueBase+4 && (a<request || a>=request+32)) return;
		if (prepared) {
			if (a==queueBase) {
				if (value>=8) return;
				publicationAttempted=true; // before the one uncertain write
			} else {
				if (a<request || a>=request+32) return;
				const unsigned word=(a-request)/4;
				if (value!=(word==0?0x56U:word==1?3U:0U)) return;
			}
		}
		PioneerWrite32(queue+(a-0x10000000)/4,value);
	}
	uint32 MsiStatusRead() { return Accessible()?PioneerRead32(msi+2):UINT32_MAX; }
	void NotifyArc() { if (Accessible()) PioneerWrite32(msi,0); }
	void Pause() { if (!Accessible() || snooze(500)!=B_OK) delayFailed=true; }

	// Configure TLB200 separately so the queue stays on TLB201. The original
	// value is checked twice and marked dirty before the first write. It is
	// never restored after an uncertain publication or reset.
	bool NotificationReady() {
		if (prepareAttempted) return false;
		prepareAttempted=true;
		if (!Accessible() || !m.savedValid || m.dmaModified) return false;
		for (unsigned w=0;w<3;w++)
			if (PioneerRead32(m.tlb-3+w)!=m.dmaSaved[w]
				|| PioneerRead32(m.tlb-3+w)!=m.dmaSaved[w]) return false;
		m.dmaModified=true;
		for (unsigned w=0;w<3;w++) {
			if (!Accessible()) return false;
			const uint32 value=BlackholeArcQueueTlbWord(true,w);
			PioneerWrite32(m.tlb-3+w,value);
			if (PioneerRead32(m.tlb-3+w)!=value) return false;
		}
		if (!Accessible() || (PioneerRead32(preparedMsi+2)&0x301)!=1) return false;
		prepared=true;return true;
	}
	void NotifyPreparedArc() {
		if (!publicationAttempted || notificationAttempted) return;
		notificationAttempted=true;
		// No PCI reads, TLB accesses or retries here: publication may already
		// have caused the reset. PioneerWrite32 supplies the platform fence.
		if (Allowed()) PioneerWrite32(preparedMsi,0);
	}
	uint32 CardRead(uint16 a,uint8 size) { return Allowed()?owner.CardRead(a,size):UINT32_MAX; }
	uint32 RootRead(uint16 a,uint8 size) { return Allowed()?owner.RootRead(a,size):UINT32_MAX; }
	void CardWrite(uint16 a,uint8 size,uint32 value) {
		if (Accessible() && a==4 && size==2 && value==0x442) owner.CardWrite(a,size,value);
	}
	void RecoveryDelay() { if (!Allowed() || snooze(100000)!=B_OK) delayFailed=true; }
};
