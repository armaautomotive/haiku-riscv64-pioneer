/* SPDX-License-Identifier: MIT */
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38StreamProtocol.h"
static tt_q38_stream_probe Job(unsigned type,unsigned inputs,unsigned bank,uint64_t offset,unsigned groups=1)
{
	tt_q38_stream_probe r={}; r.version=TT_Q38_STREAM_ABI; r.type=type; r.inputs=inputs; r.bank=bank; r.offset=offset;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_ROWS
	r.groups=groups;
#else
	(void)groups;
#endif
	for (unsigned w=0;w<inputs/2;w++) r.activation[w]=PioneerInt8Operand(-128)|(uint32_t(PioneerInt8Operand(127))<<16);
	for (unsigned b=0;b<inputs/PioneerQ38StreamBlockInputs(type);b++) r.scales[b]=b%2?0xbc800000:0x3c800000;
	return r;
}
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	BanksMock io; auto dram=BankProof(io); PioneerResidentBanks banks; assert(banks.Bind(dram));
	std::vector<uint8_t> initialized(PioneerResidentBanks::Pages,0);
	unsigned cases=0;
	for (unsigned groups:{1U,3U,16U}) {
#ifndef TT_PIONEER_EXPERIMENTAL_Q38_ROWS
		if (groups!=1) continue;
#endif
	for (unsigned type:{8U,12U,14U}) for (unsigned inputs:{5120U,6144U,17408U}) for (unsigned bank=0;bank<7;bank++) for (unsigned end=0;end<2;end++) {
		auto job=Job(type,inputs,bank,0,groups); unsigned bytes=PioneerQ38StreamBytes(job),pages=(bytes+4095)/4096;
		if (end) job.offset=PioneerResidentBanks::Capacity-uint64_t(pages)*4096;
		unsigned index=bank*uint32_t(PioneerResidentBanks::Capacity/4096)+unsigned(job.offset/4096);
		PioneerQ38StreamExtent a; assert(!PioneerQ38StreamAuthorize(banks,dram,initialized.data(),unsigned(initialized.size()),job,a) && !a.address);
		for (unsigned p=0;p<pages;p++) initialized[index+p]=1;
		assert(PioneerQ38StreamAuthorize(banks,dram,initialized.data(),unsigned(initialized.size()),job,a));
		assert(a.address==PioneerResidentBanks::Base+job.offset && a.index==index && a.pages==pages && a.bytes==bytes
			&& a.blocks==groups*(inputs/PioneerQ38StreamBlockInputs(type)) && a.flits==bytes/64
			&& a.route==(dram.translated[bank][0]|(dram.translated[bank][1]<<6)));
		for (unsigned bad:{0U,pages/2,pages-1}) {
			PioneerQ38StreamExtent unchanged=a; initialized[index+bad]=0;
			assert(!PioneerQ38StreamAuthorize(banks,dram,initialized.data(),unsigned(initialized.size()),job,a) && !memcmp(&a,&unchanged,sizeof(a)));
			initialized[index+bad]=1;
		}
		for (unsigned p=0;p<pages;p++) initialized[index+p]=0;
		cases++;
	}
	}
	auto clean=Job(12,5120,0,0);
	for (unsigned field=0;field<18;field++) {
		auto bad=clean;
		switch (field) {
			case 0:bad.version++;break; case 1:bad.type=13;break; case 2:bad.inputs=17409;break;
			case 3:bad.bank=7;break; case 4:bad.offset=1;break; case 5:bad.offset=UINT64_MAX;break;
			case 6:bad.offset=PioneerResidentBanks::Capacity-4096;break;
			case 7:bad.reserved[0]=1;break; case 8:bad.reserved[1]=1;break;
			case 9:bad.activation[0]=0;break; case 10:bad.activation[100]=0;break;
			case 11:bad.activation[2559]=0;break; case 12:bad.activation[2560]=1;break;
			case 13:bad.activation[8703]=1;break; case 14:bad.scales[0]=0x7fc00000;break;
			case 15:bad.scales[10]=0x47800001;break; case 16:bad.scales[19]=1;break;
			case 17:bad.scales[20]=1;break;
		}
		assert(!PioneerQ38StreamValid(bad));
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_ROWS
	for (unsigned bad:{0U,17U,UINT32_MAX}) { auto job=clean; job.groups=bad; assert(!PioneerQ38StreamValid(job)); }
	{ auto job=clean; job.padding=1; assert(!PioneerQ38StreamValid(job)); }
#endif
	PioneerQ38StreamExtent out;
	assert(!PioneerQ38StreamAuthorize(banks,dram,nullptr,unsigned(initialized.size()),clean,out));
	assert(!PioneerQ38StreamAuthorize(banks,dram,initialized.data(),unsigned(initialized.size())-1,clean,out));
	for (unsigned p=0;p<12;p++) initialized[p]=1;
	assert(PioneerQ38StreamAuthorize(banks,dram,initialized.data(),unsigned(initialized.size()),clean,out));
	dram.translated[0][0]^=1;
	assert(!PioneerQ38StreamAuthorize(banks,dram,initialized.data(),unsigned(initialized.size()),clean,out));
	dram.translated[0][0]^=1; banks.Quarantine();
	assert(!PioneerQ38StreamAuthorize(banks,dram,initialized.data(),unsigned(initialized.size()),clean,out));
	assert(cases==126*(TT_Q38_STREAM_ABI==2?3U:1U) && !io.requests);
	printf("PASS OFFLINE driver%u private fullK authorization:ALL%u type/width/rows/seven-bank/first-last extents,exact initialized bitmap,missing first/middle/last refusal,all input/range/padding/route/quarantine guards;NO hardware/launch authority\n",unsigned(TT_Q38_STREAM_DRIVER),cases);
}
