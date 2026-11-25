/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadState.h"
using area_id=int;using status_t=int;
constexpr int B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_ERROR=-3,B_DEV_NOT_READY=-4;
static std::vector<int> deleted;static int failArea=-1;
static bool* authorityLive=nullptr;static unsigned deleteAfterLoss=0;
static status_t delete_area(area_id id) {
	if (authorityLive && !*authorityLive) ++deleteAfterLoss;
	deleted.push_back(id);return id==failArea?B_ERROR:B_OK;
}
struct PioneerHostReadIO {area_id scratchArea=-1,writeArea=-1;};
struct PioneerDramSamplesIO {area_id dramArea=-1,dataArea=-1;};
struct PioneerArcIO {area_id scratchArea=-1;};
struct PioneerMmioIO {area_id area=-1;};
struct PioneerHostPageIO {area_id area=-1;void* page=(void*)1;};
struct FakeIO:PioneerHostReadIO,PioneerDramSamplesIO,PioneerArcIO,PioneerMmioIO,PioneerHostPageIO {
	area_id q38PanelArea=-1,attentionArea=-1,bulkArea=-1,coordinatorArea=-1,layerArea=-1,fleetArea=-1;
	area_id runtimeAreas[5]={-1,-1,-1,-1,-1},hdmaArea=-1,inboundArea=-1,siiArea=-1,nocArea=-1;
	area_id resetArea=-1,csmArea=-1,tlbArea=-1,outputArea=-1,wideArea=-1;
	void* outputPage=(void*)2;void* widePage=(void*)3;bool runtimeMapped=true;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPostResetDispose.inc"
};
static PioneerDriverRecoveryProof Evidence() {
	PioneerDriverRecoveryProof p{};
	p.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
	p.core_count=120;p.retained_owners=1;p.reset_generation=7;p.firmware_generation=7;
	return p;
}
struct Permit {
	unsigned calls=0,failAt=0;bool live=true;
	bool operator()() { if (++calls==failAt) live=false;return live; }
};
static void FabricAreas(FakeIO& io) {
	io.q38PanelArea=11;io.runtimeAreas[1]=12;io.PioneerMmioIO::area=13;
	io.PioneerHostPageIO::area=20;io.outputArea=21;io.wideArea=22;
}
int main() {
	auto proof=Evidence();assert(proof.ReadyToRetireOneOwner() && !proof.Complete());
	FakeIO invalid;invalid.q38PanelArea=11;auto incomplete=proof;incomplete.flags=0;
	assert(invalid.DisposeAfterVerifiedReset(incomplete)==B_BAD_VALUE && deleted.empty()
		&& !invalid.postResetDisposeAttempted);
	incomplete=proof;incomplete.noc_outstanding_mask=1;
	assert(invalid.DisposeAfterVerifiedReset(incomplete)==B_BAD_VALUE && deleted.empty());
	incomplete=proof;incomplete.retained_owners=2;
	assert(invalid.DisposeAfterVerifiedReset(incomplete)==B_BAD_VALUE && deleted.empty());
	FakeIO alias;alias.q38PanelArea=11;alias.PioneerHostPageIO::area=11;
	assert(alias.DisposeAfterVerifiedReset(proof)==B_BAD_VALUE && deleted.empty());
	FakeIO clean;clean.q38PanelArea=11;clean.runtimeAreas[1]=12;
	clean.PioneerMmioIO::area=13;clean.PioneerHostPageIO::area=20;clean.outputArea=21;clean.wideArea=22;
	assert(clean.DisposeAfterVerifiedReset(proof)==B_OK);
	assert((deleted==std::vector<int>{11,12,13,20,21,22}));
	assert(clean.q38PanelArea<0 && clean.PioneerHostPageIO::area<0 && !clean.page
		&& clean.outputArea<0 && !clean.outputPage && clean.wideArea<0 && !clean.widePage && !clean.runtimeMapped
		&& clean.postResetDisposeComplete);
	assert(clean.DisposeAfterVerifiedReset(proof)==B_NOT_ALLOWED);
	deleted.clear();FakeIO partial;partial.q38PanelArea=11;partial.runtimeAreas[1]=12;
	partial.PioneerHostPageIO::area=20;partial.outputArea=21;failArea=12;
	assert(partial.DisposeAfterVerifiedReset(proof)==B_ERROR);
	assert((deleted==std::vector<int>{11,12}) && partial.q38PanelArea<0
		&& partial.runtimeAreas[1]==12 && partial.PioneerHostPageIO::area==20 && partial.outputArea==21
		&& !partial.postResetDisposeComplete);
	assert(partial.DisposeAfterVerifiedReset(proof)==B_NOT_ALLOWED && deleted.size()==2);
	deleted.clear();failArea=-1;unsigned checks=0;
	{
		FakeIO io;FabricAreas(io);Permit permit;authorityLive=&permit.live;
		assert(io.DisposeFabricOldMmio(permit)==B_OK && io.fabricOldMmioDisposeComplete
			&& (deleted==std::vector<int>{11,12,13}) && !deleteAfterLoss
			&& io.PioneerHostPageIO::area==20 && io.outputArea==21 && io.wideArea==22
			&& io.page==(void*)1 && io.outputPage==(void*)2 && io.widePage==(void*)3
			&& !io.runtimeMapped && !io.postResetDisposeComplete);
		checks=permit.calls;assert(io.DisposeFabricOldMmio(permit)==B_NOT_ALLOWED
			&& io.DisposeAfterVerifiedReset(proof)==B_NOT_ALLOWED);
	}
	for (unsigned loss=1;loss<=checks;loss++) {
		deleted.clear();deleteAfterLoss=0;FakeIO io;FabricAreas(io);Permit permit;
		permit.failAt=loss;authorityLive=&permit.live;
		assert(io.DisposeFabricOldMmio(permit)==B_DEV_NOT_READY && !io.fabricOldMmioDisposeComplete
			&& !deleteAfterLoss && io.PioneerHostPageIO::area==20 && io.outputArea==21 && io.wideArea==22);
		const auto count=deleted.size();
		if (io.fabricOldMmioDisposeAttempted)
			assert(io.DisposeFabricOldMmio(permit)==B_NOT_ALLOWED && deleted.size()==count);
	}
	for (int failed:{11,12,13}) {
		deleted.clear();deleteAfterLoss=0;FakeIO io;FabricAreas(io);Permit permit;
		authorityLive=&permit.live;failArea=failed;
		assert(io.DisposeFabricOldMmio(permit)==B_ERROR && !io.fabricOldMmioDisposeComplete
			&& io.PioneerHostPageIO::area==20 && io.outputArea==21 && io.wideArea==22);
		const auto count=deleted.size();assert(io.DisposeFabricOldMmio(permit)==B_NOT_ALLOWED && deleted.size()==count);
	}
	{
		deleted.clear();failArea=-1;FakeIO io;FabricAreas(io);io.runtimeAreas[1]=20;Permit permit;
		authorityLive=&permit.live;
		assert(io.DisposeFabricOldMmio(permit)==B_BAD_VALUE && deleted.empty()
			&& !io.fabricOldMmioDisposeAttempted && io.PioneerHostPageIO::area==20);
	}
	for (int roleArea:{11,12,13,20,21,22}) {
		deleted.clear();failArea=-1;FakeIO io;FabricAreas(io);Permit permit;
		authorityLive=&permit.live;
		auto overlaps=[&](area_id area) { return area==roleArea; };
		assert(io.DisposeFabricOldMmio(permit,overlaps)==B_BAD_VALUE && deleted.empty()
			&& !io.fabricOldMmioDisposeAttempted && !io.fabricOldMmioDisposeComplete);
	}
	{
		deleted.clear();FakeIO io;FabricAreas(io);Permit permit;authorityLive=&permit.live;
		auto revoked=[&](area_id) { permit.live=false;return false; };
		assert(io.DisposeFabricOldMmio(permit,revoked)==B_DEV_NOT_READY && deleted.empty()
			&& !io.fabricOldMmioDisposeAttempted && !io.fabricOldMmioDisposeComplete);
	}
	authorityLive=nullptr;
	puts("PASS post-reset owner area retirement: proof, duplicate refusal, host-last order and partial-failure retention; SIMULATED OS ONLY");
}
