/* SPDX-License-Identifier: MIT */
// Member of the dedicated Qwen35 session IO. Only an in-kernel, exclusive
// recovery operation may call this after reset, fresh firmware/fleet/NoC and
// host-DMA audits. No device access or old TLB/L1 restoration occurs here.
bool postResetDisposeAttempted=false;
bool postResetDisposeComplete=false;
#if defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) && defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
bool fabricOldMmioDisposeAttempted=false;
bool fabricOldMmioDisposeComplete=false;
bool fabricHostDisposeAttempted=false;
bool fabricHostDisposeComplete=false;
#endif
status_t DisposeAfterVerifiedReset(const PioneerDriverRecoveryProof& proof)
{
	if (postResetDisposeAttempted) return B_NOT_ALLOWED;
#if defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) && defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
	if (fabricOldMmioDisposeAttempted) return B_NOT_ALLOWED;
#endif
	if (!proof.ReadyToRetireOneOwner()) return B_BAD_VALUE;
	// Map areas first and the wired host buffers last. All entries are the
	// actual owner fields, so successful deletion disarms the destructors.
	area_id* owned[]={
		&q38PanelArea,&attentionArea,&bulkArea,&coordinatorArea,&layerArea,&fleetArea,
		&runtimeAreas[0],&runtimeAreas[1],&runtimeAreas[2],&runtimeAreas[3],&runtimeAreas[4],
		&this->PioneerHostReadIO::scratchArea,&this->PioneerHostReadIO::writeArea,
		&hdmaArea,&inboundArea,&siiArea,&nocArea,&this->PioneerDramSamplesIO::dramArea,
		&this->PioneerDramSamplesIO::dataArea,&resetArea,&csmArea,&this->PioneerArcIO::scratchArea,
		&tlbArea,&this->PioneerMmioIO::area,
		&this->PioneerHostPageIO::area,&outputArea,&wideArea
	};
	const unsigned count=sizeof(owned)/sizeof(owned[0]);
	// An aliased area ID would make a partial cleanup ambiguous. Refuse it
	// before deleting anything, including the host buffers.
	for (unsigned i=0;i<count;i++) for (unsigned j=0;j<i;j++)
		if (*owned[i]>=B_OK && *owned[i]==*owned[j]) return B_BAD_VALUE;
	postResetDisposeAttempted=true;
	for (unsigned i=0;i<count;i++) {
		if (*owned[i]<B_OK) continue;
		const status_t status=delete_area(*owned[i]);
		if (status!=B_OK) return status; // retain all remaining areas and owner
		*owned[i]=-1;
	}
	PioneerHostPageIO::page=NULL;outputPage=NULL;widePage=NULL;
	runtimeMapped=false;
	postResetDisposeComplete=true;
	return B_OK;
}
#if defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) && defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
// After the retained fabric transaction has closed its audit views and put
// PCI into its saved idle layout, release only the old CPU MMIO mappings.
// Host DMA buffers, role-owner mappings, bank claims and scratch snapshots
// remain allocated. The caller's check supplies the exact locked owner and
// ticket; this operation does not claim complete mapping disposal or recovery.
template<class Check,class Protected> status_t DisposeFabricOldMmio(Check& check,Protected& protectedArea)
{
	if (fabricOldMmioDisposeAttempted || postResetDisposeAttempted) return B_NOT_ALLOWED;
	if (!check()) return B_DEV_NOT_READY;
	area_id* mmio[]={
		&q38PanelArea,&attentionArea,&bulkArea,&coordinatorArea,&layerArea,&fleetArea,
		&runtimeAreas[0],&runtimeAreas[1],&runtimeAreas[2],&runtimeAreas[3],&runtimeAreas[4],
		&this->PioneerHostReadIO::scratchArea,&this->PioneerHostReadIO::writeArea,
		&hdmaArea,&inboundArea,&siiArea,&nocArea,&this->PioneerDramSamplesIO::dramArea,
		&this->PioneerDramSamplesIO::dataArea,&resetArea,&csmArea,&this->PioneerArcIO::scratchArea,
		&tlbArea,&this->PioneerMmioIO::area
	};
	area_id* host[]={&this->PioneerHostPageIO::area,&outputArea,&wideArea};
	const unsigned count=sizeof(mmio)/sizeof(mmio[0]);
	for (unsigned i=0;i<count;i++) {
		if (*mmio[i]>=B_OK) {
			if (!check()) return B_DEV_NOT_READY;
			if (protectedArea(*mmio[i])) return B_BAD_VALUE;
			if (!check()) return B_DEV_NOT_READY;
		}
		for (unsigned j=0;j<i;j++) if (*mmio[i]>=B_OK && *mmio[i]==*mmio[j]) return B_BAD_VALUE;
		for (area_id* buffer:host) if (*mmio[i]>=B_OK && *mmio[i]==*buffer) return B_BAD_VALUE;
	}
	for (area_id* buffer:host) if (*buffer>=B_OK) {
		if (!check()) return B_DEV_NOT_READY;
		if (protectedArea(*buffer)) return B_BAD_VALUE;
		if (!check()) return B_DEV_NOT_READY;
	}
	fabricOldMmioDisposeAttempted=true;
	for (unsigned i=0;i<count;i++) if (*mmio[i]>=B_OK) {
		if (!check()) return B_DEV_NOT_READY;
		const status_t status=delete_area(*mmio[i]);
		if (status!=B_OK) return status;
		*mmio[i]=-1;
		if (!check()) return B_DEV_NOT_READY;
	}
	if (!check()) return B_DEV_NOT_READY;
	runtimeMapped=false;fabricOldMmioDisposeComplete=true;return B_OK;
}
template<class Check> status_t DisposeFabricOldMmio(Check& check)
{
	auto noProtectedAreas=[](area_id) { return false; };
	return DisposeFabricOldMmio(check,noProtectedAreas);
}

// The old host buffers remain pinned after old MMIO removal. Only a later
// fresh-session scope that has proved DMA quiescence, writer exclusion and
// no remaining host references may call this; it also protects all fresh
// and role-owner areas against aliasing. Direct deletion is necessary after
// an error because the ordinary release APIs reject quarantined buffers.
// All attempts are consumed before the first delete; no failure is replayed.
template<class Check,class Protected> status_t DisposeFabricHostBuffers(Check& check,Protected& protectedArea)
{
	if (fabricHostDisposeAttempted || postResetDisposeAttempted) return B_NOT_ALLOWED;
	if (!fabricOldMmioDisposeComplete || !check()) return B_DEV_NOT_READY;
	area_id* areas[]={&this->PioneerHostPageIO::area,&outputArea,&wideArea};
	volatile uint32** pages[]={&this->PioneerHostPageIO::page,&outputPage,&widePage};
	for (unsigned i=0;i<3;i++) {
		if ((*areas[i]>=B_OK) != (*pages[i]!=NULL)) return B_BAD_VALUE;
		for (unsigned j=0;j<i;j++) if (*areas[i]>=B_OK && *areas[i]==*areas[j]) return B_BAD_VALUE;
		if (*areas[i]>=B_OK && (!check() || protectedArea(*areas[i]) || !check())) return B_DEV_NOT_READY;
	}
	if (!check()) return B_DEV_NOT_READY;
	fabricHostDisposeAttempted=true;
	this->PioneerHostPageIO::releaseAttempted=true;outputReleaseAttempted=true;wideReleaseAttempted=true;
	for (unsigned i=0;i<3;i++) if (*areas[i]>=B_OK) {
		if (!check()) return B_DEV_NOT_READY;
		const status_t result=delete_area(*areas[i]);
		if (result!=B_OK) return result;
		*areas[i]=-1;*pages[i]=NULL;
		if (!check()) return B_DEV_NOT_READY;
	}
	if (!check()) return B_DEV_NOT_READY;
	fabricHostDisposeComplete=true;return B_OK;
}
#endif
