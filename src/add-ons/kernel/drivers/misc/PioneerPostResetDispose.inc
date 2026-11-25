/* SPDX-License-Identifier: MIT */
// Member of the dedicated Qwen35 session IO. Only an in-kernel, exclusive
// recovery operation may call this after reset, fresh firmware/fleet/NoC and
// host-DMA audits. No device access or old TLB/L1 restoration occurs here.
bool postResetDisposeAttempted=false;
bool postResetDisposeComplete=false;
#if defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) && defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
bool fabricOldMmioDisposeAttempted=false;
bool fabricOldMmioDisposeComplete=false;
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
template<class Check> status_t DisposeFabricOldMmio(Check& check)
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
		for (unsigned j=0;j<i;j++) if (*mmio[i]>=B_OK && *mmio[i]==*mmio[j]) return B_BAD_VALUE;
		for (area_id* buffer:host) if (*mmio[i]>=B_OK && *mmio[i]==*buffer) return B_BAD_VALUE;
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
#endif
