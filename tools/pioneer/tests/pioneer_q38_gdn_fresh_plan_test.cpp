/* SPDX-License-Identifier: MIT */
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnFreshPlan.h"
#include <memory>
static void Complete(const PioneerQ38GdnFreshPlan& plan,const std::vector<uint8_t>& initialized) {
	assert(plan.Count()==672 && plan.Regions());
	uint64_t covered[48][2]={};
	std::vector<PioneerQ38GdnRegion> sorted(plan.Regions(),plan.Regions()+plan.Count());
	std::sort(sorted.begin(),sorted.end(),[](const auto& a,const auto& b){return a.bank<b.bank || (a.bank==b.bank && a.offset<b.offset);});
	for (unsigned i=0;i<sorted.size();i++) {
		const auto& r=sorted[i]; assert(r.layer<64 && r.layer%4!=3 && r.slot<2 && r.firstHead+r.heads<=48 && (r.heads==6 || r.heads==7)
			&& r.bank<7 && !(r.offset&4095) && uint64_t(r.offset)+r.heads*65536<=PioneerResidentBanks::Capacity);
		uint64_t mask=((UINT64_C(1)<<r.heads)-1)<<r.firstHead; auto& heads=covered[r.layer-r.layer/4][r.slot];
		assert(!(heads&mask)); heads|=mask;
		if (i && sorted[i-1].bank==r.bank) assert(uint64_t(sorted[i-1].offset)+sorted[i-1].heads*65536<=r.offset);
		for (unsigned p=0;p<r.heads*16;p++) assert(!initialized[uint64_t(r.bank)*(PioneerResidentBanks::Capacity/4096)+r.offset/4096+p]);
	}
	for (const auto& layer:covered) for (auto heads:layer) assert(heads==UINT64_C(0xffffffffffff));
	StoreMock io; auto proof=BankProof(io); PioneerResidentBanks banks; assert(banks.Bind(proof));
	auto owner=std::make_unique<PioneerQ38GdnBanks>(); const PioneerQ38GdnIdentity id={1,2,3,4};
	assert(owner->Bind(banks,proof,id,initialized.data(),unsigned(initialized.size()),plan.Regions(),plan.Count()));
	for (unsigned layer=0;layer<64;layer++) if (layer%4!=3) for (unsigned slot=0;slot<2;slot++) for (unsigned worker=0;worker<96;worker++) {
		PioneerQ38GdnBankExtent extent; assert(owner->InitializeHalf(id,layer,slot,worker,extent) && extent.pages==8);
	}
}
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	constexpr unsigned bankPages=unsigned(PioneerResidentBanks::Capacity/4096),regionPages=7*16;
	std::vector<uint8_t> initialized(PioneerResidentBanks::Pages,0);
	{
		auto plan=std::make_unique<PioneerQ38GdnFreshPlan>(); assert(!plan->Count() && !plan->Regions());
		assert(plan->Build(initialized.data(),unsigned(initialized.size()))); Complete(*plan,initialized);
		assert(!plan->Build(initialized.data(),unsigned(initialized.size())) && plan->Count()==672);
		for (unsigned i=0;i<672;i+=7) {
			bool banks[7]={}; for (unsigned stripe=0;stripe<7;stripe++) banks[plan->Regions()[i+stripe].bank]=true;
			for (bool present:banks) assert(present); // EACH layer/slot spans ALL7 when fresh
		}
		for (auto value:initialized) assert(!value);
	}
	for (unsigned bank=0;bank<7;bank++) {
		std::fill(initialized.begin(),initialized.end(),0);
		std::fill(initialized.begin()+uint64_t(bank)*bankPages,initialized.begin()+uint64_t(bank+1)*bankPages,1);
		auto plan=std::make_unique<PioneerQ38GdnFreshPlan>(); assert(plan->Build(initialized.data(),unsigned(initialized.size())));
		Complete(*plan,initialized); for (unsigned i=0;i<672;i++) assert(plan->Regions()[i].bank!=bank);
	}
	{
		std::fill(initialized.begin(),initialized.end(),1);
		// EXACTLY672 separated fresh regions,not a large empty tail.
		unsigned cursor[7]; for (auto& page:cursor) page=bankPages-1;
		for (unsigned layer=0;layer<48;layer++) for (unsigned slot=0;slot<2;slot++) for (unsigned stripe=0;stripe<7;stripe++) {
			unsigned bank=(layer+slot+stripe)%7,pages=(6+unsigned(stripe<6))*16;
			unsigned first=cursor[bank]-pages; cursor[bank]=first-1;
			std::fill(initialized.begin()+uint64_t(bank)*bankPages+first,initialized.begin()+uint64_t(bank)*bankPages+first+pages,0);
		}
		const auto before=initialized; auto plan=std::make_unique<PioneerQ38GdnFreshPlan>();
		assert(plan->Build(initialized.data(),unsigned(initialized.size())) && initialized==before); Complete(*plan,initialized);
	}
	for (unsigned trial=0;trial<7;trial++) {
		std::fill(initialized.begin(),initialized.end(),trial==0 || trial==6?0:1);
		if (trial>=1 && trial<=3) initialized[trial==1?0:trial==2?initialized.size()/2:initialized.size()-1]=2;
		if (trial==4) for (unsigned i=0;i<initialized.size();i++) initialized[i]=uint8_t(i&1); // no whole region
		if (trial==5) std::fill(initialized.end()-regionPages,initialized.end(),0); // only ONE complete region
		if (trial==6) std::fill(initialized.begin(),initialized.end(),1);
		const auto before=initialized; auto plan=std::make_unique<PioneerQ38GdnFreshPlan>();
		if (!trial) assert(!plan->Build(nullptr,unsigned(initialized.size())));
		else assert(!plan->Build(initialized.data(),unsigned(initialized.size())));
		assert(!plan->Count() && !plan->Regions() && initialized==before && !plan->Build(initialized.data(),unsigned(initialized.size())));
	}
	for (unsigned pages:{0U,PioneerResidentBanks::Pages-1,PioneerResidentBanks::Pages+1}) {
		auto plan=std::make_unique<PioneerQ38GdnFreshPlan>(); assert(!plan->Build(initialized.data(),pages) && !plan->Regions());
	}
	{
		auto plan=std::make_unique<PioneerQ38GdnFreshPlan>();
		assert(!plan->Build(reinterpret_cast<const uint8_t*>(plan.get()),PioneerResidentBanks::Pages) && !plan->Regions());
	}
	puts("PASS OFFLINE heap-only GDN fresh proposal:672 rotated stripes,ALL7 banks for EACH layer/slot when fresh,all48x2x48 heads/9216 real-owner half extents,7 full-bank fallback cases,exactly672 separated fresh regions,learned pages unchanged/excluded,corrupt bitmap/fragmentation/exhaustion/overlap/bounds/partial visibility refused;NO reservation/init/IO/native authority");
}
