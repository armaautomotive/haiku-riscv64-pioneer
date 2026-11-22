/* SPDX-License-Identifier: MIT */
// Member of the dedicated Qwen35 session IO. Only an in-kernel, exclusive
// recovery operation may call this after reset, fresh firmware/fleet/NoC and
// host-DMA audits. No device access or old TLB/L1 restoration occurs here.
bool postResetDisposeAttempted=false;
bool postResetDisposeComplete=false;
status_t DisposeAfterVerifiedReset(const PioneerDriverRecoveryProof& proof)
{
	if (postResetDisposeAttempted) return B_NOT_ALLOWED;
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
