/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>

#include "../PioneerQwen35ActivationPlan.h"
#include "../PioneerQwen35DeviceLayoutBuild.h"
#include "../PioneerQwen35ManifestValidation.h"
#include "../PioneerQwen35PackedPlan.h"

static_assert(sizeof(PioneerQwen35::DeviceLayout)==81952,
	"manifest carries two row-sharded activation buffers");

struct FakePackedPlan {
	PioneerQ35ActivationWorkspace workspace[PioneerQ35ActivationBankCount];
	bool Workspace(unsigned bank,uint32_t& address,uint32_t& bytes) const
	{
		if (bank>=PioneerQ35ActivationBankCount || workspace[bank].bank!=bank) return false;
		address=workspace[bank].address; bytes=workspace[bank].bytes; return true;
	}
};

static FakePackedPlan Workspaces()
{
	FakePackedPlan packed{};
	for (unsigned bank=0;bank<PioneerQ35ActivationBankCount;bank++)
		packed.workspace[bank]={bank,0x08000000,64*1024*1024};
	return packed;
}

int main()
{
	PioneerQwen35::DeviceLayout transferLayout{};
	auto& transferSpace=transferLayout.activation[0];
	transferSpace.rows=96; transferSpace.rowBytes=sizeof(float);
	transferSpace.count=3; transferSpace.unit=16;
	transferSpace.parts[0]={0,0x04000000,0,32};
	transferSpace.parts[1]={1,0x04400000,32,32};
	transferSpace.parts[2]={2,0x04800000,64,32};
	PioneerQwen35::RowTransfer transfers[PioneerQwen35::Banks]{};
	unsigned transferCount=0;
	assert(transferLayout.ResolveActivationTransfers(0,16,64,transfers,transferCount));
	assert(transferCount==3);
	assert(transfers[0].range.bank==0 && transfers[0].range.address==0x04000040
		&& transfers[0].range.bytes==64 && transfers[0].localRow==0 && transfers[0].rows==16);
	assert(transfers[1].range.bank==1 && transfers[1].range.address==0x04400000
		&& transfers[1].range.bytes==128 && transfers[1].localRow==16 && transfers[1].rows==32);
	assert(transfers[2].range.bank==2 && transfers[2].range.address==0x04800000
		&& transfers[2].range.bytes==64 && transfers[2].localRow==48 && transfers[2].rows==16);
	assert(!transferLayout.ResolveActivationTransfers(2,0,16,transfers,transferCount));
	assert(!transferLayout.ResolveActivationTransfers(0,1,16,transfers,transferCount));
	assert(!transferLayout.ResolveActivationTransfers(0,80,32,transfers,transferCount));
	PioneerQwen35::DeviceLayout invalidManifest{};
	PioneerQwen35::Range invalidRope{};
	assert(!PioneerQwen35::ValidateManifest(invalidManifest,invalidRope,
		[](unsigned,unsigned) { return false; },0));
	FakePackedPlan packed=Workspaces();
	PioneerQwen35ActivationPlan plan;
	PioneerQwen35PackedPlan unbuiltPackedPlan;
	assert(!plan.InitializeFromPacked(unbuiltPackedPlan));
	assert(plan.InitializeFromPacked(packed,32768)); // bank 0 begins after the reserved RoPE prefix
	PioneerQ35ActivationBuffer hidden{},ffnA{},ffnB{},logitsA{},logitsB{};
	assert(plan.AllocateRows(2560,4,hidden));
	assert(plan.AllocateRows(9216,4,ffnA));
	assert(plan.AllocateRows(9216,4,ffnB));
	assert(plan.AllocateRows(248320,4,logitsA));
	assert(plan.AllocateRows(248320,4,logitsB));
	assert(plan.Valid(hidden) && plan.Valid(ffnA) && plan.Valid(ffnB)
		&& plan.Valid(logitsA) && plan.Valid(logitsB));
	assert(hidden.fragmentCount==PioneerQ35ActivationBankCount);
	assert(ffnA.rows==9216 && ffnA.fragmentCount==PioneerQ35ActivationBankCount
		&& ffnB.slot!=ffnA.slot);
	assert(logitsA.rows==248320 && logitsA.fragmentCount==PioneerQ35ActivationBankCount);
	assert(logitsA.slot!=logitsB.slot);
	assert(hidden.fragments[0].address==packed.workspace[0].address+32768);
	assert(logitsA.fragments[0].address>=hidden.fragments[0].address
		+((hidden.fragments[0].bytes+4095)&~4095U));

	unsigned covered=0;
	for (unsigned i=0;i<logitsA.fragmentCount;i++) {
		const auto& fragment=logitsA.fragments[i];
		assert(fragment.bank==i && fragment.firstRow==covered);
		assert(fragment.rows && !(fragment.rows&15));
		PioneerQ35ActivationRange range{};
		assert(plan.ResolveRows(logitsA,fragment.firstRow,fragment.rows,range));
		assert(range.bank==fragment.bank && range.address==fragment.address
			&& range.bytes==fragment.bytes && range.rows==fragment.rows);
		covered+=fragment.rows;
	}
	assert(covered==logitsA.rows);
	PioneerQ35ActivationRange edge{};
	assert(!plan.ResolveRows(logitsA,logitsA.fragments[0].firstRow
		+logitsA.fragments[0].rows-16,32,edge));
	assert(!plan.ResolveRows(logitsA,1,16,edge));
	assert(!plan.ResolveRows(logitsA,logitsA.rows-16,32,edge));

	PioneerQ35ActivationBuffer forged=logitsA;
	forged.fragments[0].address+=4096;
	assert(!plan.Valid(forged));
	forged=logitsA; forged.fragments[1].bank=forged.fragments[0].bank;
	assert(!plan.Valid(forged));
	forged=logitsA; forged.rows-=16;
	assert(!plan.Valid(forged));

	PioneerQ35ActivationBuffer invalidBuffer{};
	assert(!plan.AllocateRows(1,4,invalidBuffer));
	assert(!plan.AllocateRows(16,0,invalidBuffer));
	assert(!plan.AllocateRows(248320,3000,invalidBuffer));
	assert(plan.AllocateRows(4096,4,invalidBuffer)); // a failed allocation consumed no arena space

	PioneerQ35ActivationWorkspace bad[PioneerQ35ActivationBankCount];
	for (unsigned i=0;i<PioneerQ35ActivationBankCount;i++) bad[i]=packed.workspace[i];
	bad[1].bank=bad[0].bank;
	assert(!plan.Initialize(bad,PioneerQ35ActivationBankCount));
	for (unsigned i=0;i<PioneerQ35ActivationBankCount;i++) bad[i]=packed.workspace[i];
	bad[0].address++;
	assert(!plan.Initialize(bad,PioneerQ35ActivationBankCount));
	for (unsigned i=0;i<PioneerQ35ActivationBankCount;i++) bad[i]=packed.workspace[i];
	bad[0].address=PioneerQwen35ActivationPlan::BankLimit-4096;
	bad[0].bytes=8192;
	assert(!plan.Initialize(bad,PioneerQ35ActivationBankCount));
	for (unsigned i=0;i<PioneerQ35ActivationBankCount;i++) bad[i]=packed.workspace[i];
	assert(!plan.Initialize(bad,PioneerQ35ActivationBankCount-1));
	assert(!plan.InitializeFromPacked(packed,1)); // reservation must be page aligned
	assert(!plan.InitializeFromPacked(packed,64*1024*1024+4096));
	assert(plan.InitializeFromPacked(packed));
	assert(!plan.Valid(hidden)); // reinitialization starts a fresh buffer epoch
	return 0;
}
