/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>

#include "../PioneerQwen35ActivationPlan.h"
#include "../PioneerQwen35PackedPlan.h"

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
	FakePackedPlan packed=Workspaces();
	PioneerQwen35ActivationPlan plan;
	PioneerQwen35PackedPlan unbuiltPackedPlan;
	assert(!plan.Initialize(unbuiltPackedPlan));
	assert(plan.Initialize(packed));
	PioneerQ35ActivationBuffer hidden{},logitsA{},logitsB{};
	assert(plan.AllocateRows(2560,4,hidden));
	assert(plan.AllocateRows(248320,4,logitsA));
	assert(plan.AllocateRows(248320,4,logitsB));
	assert(plan.Valid(hidden) && plan.Valid(logitsA) && plan.Valid(logitsB));
	assert(hidden.fragmentCount==PioneerQ35ActivationBankCount);
	assert(logitsA.rows==248320 && logitsA.fragmentCount==PioneerQ35ActivationBankCount);
	assert(logitsA.slot!=logitsB.slot);

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
	assert(plan.Initialize(packed));
	assert(!plan.Valid(hidden)); // reinitialization starts a fresh buffer epoch
	return 0;
}
