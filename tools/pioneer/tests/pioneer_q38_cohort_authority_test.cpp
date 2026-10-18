/* SPDX-License-Identifier: MIT */
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CohortProtocol.h"
#include <memory>
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	BanksMock io; auto dram=BankProof(io); PioneerResidentBanks banks; assert(banks.Bind(dram));
	std::vector<uint8_t> initialized(PioneerResidentBanks::Pages,0);
	unsigned cases=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned inputs:{5120U,6144U,17408U}) for (unsigned groups:{1U,3U,16U})
	for (unsigned bank=0;bank<7;bank++) for (unsigned head:{0U,64U,1984U,4032U}) for (unsigned end=0;end<2;end++) {
		unsigned bytes=PioneerQ38CohortBytes(type,inputs,groups);
		uint64_t offset=end?PioneerResidentBanks::Capacity-((bytes+head+4095)/4096)*4096+head:head;
		tt_q38_cohort_part p={bank,groups,offset}; PioneerQ38CohortExtent a;
		unsigned index=bank*unsigned(PioneerResidentBanks::Capacity/4096)+unsigned(offset/4096),pages=(head+bytes+4095)/4096;
		assert(!PioneerQ38CohortAuthorizePart(banks,dram,initialized.data(),unsigned(initialized.size()),type,inputs,p,119,a));
		for (unsigned w=0;w<pages;w++) initialized[index+w]=1;
		assert(PioneerQ38CohortAuthorizePart(banks,dram,initialized.data(),unsigned(initialized.size()),type,inputs,p,119,a));
		unsigned blocks=inputs/PioneerQ38CohortBlockInputs(type),scaleBytes=(blocks*4+63)&~63U;
		assert(a.address==PioneerResidentBanks::Base+offset && a.index==index && a.pages==pages && a.bytes==bytes
			&& a.blocks==groups*blocks && a.reads==groups*blocks+(inputs*2+16383)/16384+1
			&& a.flits==(bytes+inputs*2+scaleBytes)/64 && a.route==(dram.translated[bank][0]|(dram.translated[bank][1]<<6)));
		for (unsigned bad:{0U,pages/2,pages-1}) {
			auto before=a; initialized[index+bad]=0;
			assert(!PioneerQ38CohortAuthorizePart(banks,dram,initialized.data(),unsigned(initialized.size()),type,inputs,p,119,a)
				&& !memcmp(&a,&before,sizeof(a))); initialized[index+bad]=1;
		}
		for (unsigned w=0;w<pages;w++) initialized[index+w]=0;
		cases++;
	}
	// Full120-worker request:each part lies inside its own non-overlapping
	// byte extent;all have nonzero64B head,distributed over seven banks.
	auto r=std::make_unique<tt_q38_cohort_probe>(); *r={};
	r->version=1; r->type=12; r->inputs=5120; r->workers=120;
	for (unsigned w=0;w<2560;w++) r->activation[w]=PioneerInt8Operand(-128)|(uint32_t(PioneerInt8Operand(127))<<16);
	for (unsigned b=0;b<20;b++) r->scales[b]=b%2?0xbc800000:0x3c800000;
	for (unsigned w=0;w<120;w++) r->part[w]={w%7,1,64+uint64_t(w/7)*65536};
	PioneerQ38CohortExtent out[120],before[120];
	memset(out,0xa5,sizeof(out)); memcpy(before,out,sizeof(out));
	assert(PioneerQ38CohortValid(*r) && !PioneerQ38CohortAuthorize(banks,dram,initialized.data(),unsigned(initialized.size()),*r,out)
		&& !memcmp(out,before,sizeof(out)));
	for (auto& value:initialized) value=1;
	assert(PioneerQ38CohortAuthorize(banks,dram,initialized.data(),unsigned(initialized.size()),*r,out));
	assert(out[0].reads==20 && out[119].reads==22);
	for (unsigned w=0;w<120;w++) {
		unsigned last=out[w].index+out[w].pages-1; initialized[last]=2; memcpy(before,out,sizeof(out));
		assert(!PioneerQ38CohortAuthorize(banks,dram,initialized.data(),unsigned(initialized.size()),*r,out)
			&& !memcmp(out,before,sizeof(out))); initialized[last]=1;
	}
	for (unsigned bad=0;bad<24;bad++) {
		auto copy=std::make_unique<tt_q38_cohort_probe>(*r);
		switch(bad) {
			case 0:copy->version=2;break; case 1:copy->type=13;break; case 2:copy->inputs=17409;break;
			case 3:copy->workers=0;break; case 4:copy->workers=121;break; case 5:copy->reserved[3]=1;break;
			case 6:copy->part[119].bank=7;break; case 7:copy->part[119].groups=0;break; case 8:copy->part[119].groups=17;break;
			case 9:copy->part[119].offset=1;break; case 10:copy->part[119].offset=UINT64_MAX;break;
			case 11:copy->part[119].offset=PioneerResidentBanks::Capacity-64;break;
			case 12:copy->activation[0]=0;break; case 13:copy->activation[2559]=0;break;
			case 14:copy->activation[2560]=1;break; case 15:copy->activation[8703]=1;break;
			case 16:copy->scales[0]=0x7fc00000;break; case 17:copy->scales[19]=1;break;
			case 18:copy->scales[20]=1;break; case 19:copy->scales[543]=1;break;
			case 20:copy->part[119]=copy->part[0];break;
			case 21:copy->part[119]=copy->part[0];copy->part[119].offset+=64;break;
			case 22:copy->workers=119;break;
			case 23:copy->workers=1;copy->part[119]={0,0,0};break;
		}
		assert(!PioneerQ38CohortValid(*copy));
	}
	assert(!PioneerQ38CohortAuthorize(banks,dram,nullptr,unsigned(initialized.size()),*r,out));
	assert(!PioneerQ38CohortAuthorize(banks,dram,initialized.data(),unsigned(initialized.size())-1,*r,out));
	dram.translated[6][0]^=1;
	assert(!PioneerQ38CohortAuthorize(banks,dram,initialized.data(),unsigned(initialized.size()),*r,out));
	dram.translated[6][0]^=1; banks.Quarantine();
	assert(!PioneerQ38CohortAuthorize(banks,dram,initialized.data(),unsigned(initialized.size()),*r,out));
	assert(cases==1512 && !io.requests);
	printf("PASS OFFLINE cohort private authority:%u extents/64B heads/seven banks/120 workers,exact initialized coverage,atomic all-worker rejection,overlap/input/route/quarantine guards;NO hardware/launch authority\n",cases);
}
