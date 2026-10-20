/* SPDX-License-Identifier: MIT */
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NonRootHeld.h"
#include <set>
struct NonRootIO {
	uint32_t expected[16]={},tlb[3]={};
	unsigned touches=0,sourceReads=0,tlbReads=0,tlbWrites=0,resets=0,selected=0;
	unsigned failSource=0,failTlb=0,failReset=0;
	bool owner=true,mapped=true,unknown=false;
	std::set<std::pair<unsigned,unsigned>> observed;
	void Touch() { assert(!unknown); touches++; }
	bool NonRootHeldOwner() { return owner; }
	bool NonRootHeldMappingStable() { return mapped; }
	uint32_t SourceRead(unsigned word) { Touch(); assert(word<16); if (++sourceReads==failSource) { unknown=true; return expected[word]^1; } return expected[word]; }
	void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) { Touch(); assert(slot==1 && word<3); tlb[word]=value; tlbWrites++; }
	uint32_t DramTlbRead(unsigned slot,unsigned word) {
		Touch(); assert(slot==1 && word<3);
		if (++tlbReads==failTlb) { unknown=true; return tlb[word]^1; }
		return tlb[word];
	}
	uint32_t ResetRead() {
		Touch(); resets++;
		unsigned x=0,y=0;
		// Determine coordinates independently from all fixed candidate words,
		// not by assuming undocumented TLB field positions.
		bool found=false;
		for (unsigned candidateX:{1U,2U,3U,4U,5U,6U,7U,10U,11U,12U,13U,14U}) for (unsigned candidateY=2;candidateY<=11;candidateY++) {
			bool match=true; for (unsigned w=0;w<3;w++) match &= tlb[w]==BlackholeDramFixedTlbWord(0xffa00000,candidateX,candidateY,true,w);
			if (match) { assert(!found); x=candidateX; y=candidateY; found=true; }
		}
		assert(found && !(x==1 && y==2)); observed.insert({x,y});
		if (resets==failReset) { unknown=true; return 0x46800; }
		return 0x47800;
	}
};
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	StoreMock baseline; auto proof=BankProof(baseline);
	NonRootIO model; for (unsigned i=0;i<16;i++) model.expected[i]=proof.source[i];
	assert(PioneerQ38NonRootHeld(model,proof) && model.observed.size()==119 && model.resets==238
		&& model.tlbWrites==360 && model.tlbReads==360 && model.sourceReads==32);
	for (unsigned w=0;w<3;w++) assert(model.tlb[w]==BlackholeDramFixedTlbWord(0xffa00000,1,2,true,w));
	for (unsigned fault=1;fault<=360;fault++) {
		NonRootIO bad; for (unsigned i=0;i<16;i++) bad.expected[i]=proof.source[i]; bad.failTlb=fault;
		assert(!PioneerQ38NonRootHeld(bad,proof) && bad.unknown && bad.tlbReads==fault);
	}
	for (unsigned fault=1;fault<=238;fault++) {
		NonRootIO bad; for (unsigned i=0;i<16;i++) bad.expected[i]=proof.source[i]; bad.failReset=fault;
		assert(!PioneerQ38NonRootHeld(bad,proof) && bad.unknown && bad.resets==fault);
	}
	for (unsigned fault=1;fault<=32;fault++) {
		NonRootIO bad; for (unsigned i=0;i<16;i++) bad.expected[i]=proof.source[i]; bad.failSource=fault;
		assert(!PioneerQ38NonRootHeld(bad,proof) && bad.unknown && bad.sourceReads==fault);
	}
	for (unsigned fault=0;fault<6;fault++) {
		NonRootIO bad; auto invalid=proof;
		if (!fault) bad.owner=false; else if (fault==1) bad.mapped=false; else if (fault==2) invalid.source[13]|=2;
		else if (fault==3) invalid.version=0; else if (fault==4) invalid.routes_verified=6; else invalid.recovery_required=1;
		assert(!PioneerQ38NonRootHeld(bad,invalid) && !bad.touches);
	}
	for (unsigned x:{1U,2U,3U,4U,5U,6U,7U,10U,11U,12U,13U,14U}) {
		NonRootIO bad; auto invalid=proof; unsigned shift=(x%6)*5,index=1+x/6;
		invalid.source[index]=(invalid.source[index]&~(31U<<shift))|(31U<<shift);
		assert(!PioneerQ38NonRootHeld(bad,invalid) && !bad.touches);
	}
	for (unsigned y=2;y<=11;y++) {
		NonRootIO bad; auto invalid=proof; invalid.source[14]|=1U<<y;
		assert(!PioneerQ38NonRootHeld(bad,invalid) && !bad.touches);
	}
	puts("PASS OFFLINE physical non-root audit:ALL119 tiles/TWO held samples each,root never read/reset/held,complete source+read-only TLB proof,all360 TLB/238 reset/32 source faults stop BEFORE ANY later IO or root restore;NO native adapter/qualification");
}
