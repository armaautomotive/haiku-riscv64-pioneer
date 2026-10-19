/* SPDX-License-Identifier: MIT */
// Metadata-only:the real bound owner+bitmap,NO card IO/driver selection.
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanAuthority.h"
static bool Empty(const PioneerQ38SpanExtent& e)
{ return !e.bank && !e.address && !e.route && !e.index && !e.pages && !e.bytes; }
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	StoreMock io; auto baseline=BankProof(io); PioneerResidentBanks banks;
	assert(banks.Bind(baseline)); std::vector<uint8_t> initialized(PioneerResidentBanks::Pages,0);
	PioneerQ38SpanExtent e;
	for (unsigned bank=0;bank<7;bank++) for (unsigned bytes:{4096U,12288U,16384U,20480U,1048576U})
		for (bool edge:{false,true}) {
			uint64_t offset=edge?PioneerResidentBanks::Capacity-bytes:0;
			assert(PioneerQ38SpanAuthorize(banks,baseline,initialized.data(),unsigned(initialized.size()),bank,offset,bytes,e));
			assert(e.bank==bank && e.address==0x04000000+offset && e.route==PioneerQ38SpanRoute(bank)
				&& e.index==bank*786432+offset/4096 && e.pages==bytes/4096 && e.bytes==bytes);
		}
	for (unsigned bank=0;bank<7;bank++) {
		unsigned first=bank*786432;
		for (unsigned page=0;page<256;page++) {
			initialized[first+page]=1; e={1,2,3,4,5,6};
			assert(!PioneerQ38SpanAuthorize(banks,baseline,initialized.data(),unsigned(initialized.size()),bank,0,1048576,e) && Empty(e));
			initialized[first+page]=0;
		}
	}
	for (unsigned fault=0;fault<12;fault++) {
		unsigned bank=0,bytes=4096,pages=unsigned(initialized.size()); uint64_t offset=0;
		const uint8_t* bitmap=initialized.data();
		if (fault==0) bank=7; if (fault==1) bank=~0U;
		if (fault==2) bytes=0; if (fault==3) bytes=4097; if (fault==4) bytes=1048576+4096;
		if (fault==5) offset=1; if (fault==6) offset=PioneerResidentBanks::Capacity;
		if (fault==7) { offset=PioneerResidentBanks::Capacity-4096; bytes=8192; }
		if (fault==8) offset=UINT64_MAX-4095; if (fault==9) pages--;
		if (fault==10) bitmap=nullptr; if (fault==11) pages++;
		e={1,2,3,4,5,6};
		assert(!PioneerQ38SpanAuthorize(banks,baseline,bitmap,pages,bank,offset,bytes,e) && Empty(e));
	}
	auto copy=baseline;
	assert(!PioneerQ38SpanAuthorize(banks,copy,initialized.data(),unsigned(initialized.size()),0,0,4096,e));
	for (unsigned bank=0;bank<7;bank++) for (unsigned sample=0;sample<3;sample++) for (unsigned word=0;word<4;word++) {
		baseline.id[bank][sample][word]^=1;
		assert(!PioneerQ38SpanAuthorize(banks,baseline,initialized.data(),unsigned(initialized.size()),0,0,4096,e));
		baseline.id[bank][sample][word]^=1;
	}
	baseline.source[1]^=1;
	assert(!PioneerQ38SpanAuthorize(banks,baseline,initialized.data(),unsigned(initialized.size()),0,0,4096,e));
	baseline.source[1]^=1;
	assert(PioneerQ38SpanAuthorize(banks,baseline,initialized.data(),unsigned(initialized.size()),0,0,4096,e));
	assert(!banks.Range(baseline,0,0,5)); // preserved legacy maximum
	banks.Quarantine();
	for (unsigned bank=0;bank<7;bank++)
		assert(!PioneerQ38SpanAuthorize(banks,baseline,initialized.data(),unsigned(initialized.size()),bank,0,4096,e));
	assert(!io.Mock::writes && !io.requests);
	puts("PASS OFFLINE span authority:ALL7 banks/3GiB edges/1MiB,every initialized page refuses ALL authority,foreign/stale/quarantined owner/shape/overflow refusal,legacy<=4 unchanged;NO card IO/native selection");
}
