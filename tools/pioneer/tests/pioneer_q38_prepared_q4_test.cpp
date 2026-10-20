/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38PreparedQ4.h"
#include "../PioneerQwen38Gguf.h"
#include "../PioneerQwen38TiledReader.h"
#include <array>
#include <assert.h>
#include <string.h>
static void Same(const PioneerQ38Panel& a,const PioneerQ38Panel& b) {
	for (unsigned row=0;row<16;row++) assert(a.coefficient[row]==b.coefficient[row] && a.minimum[row]==b.minimum[row]);
}
static void Stripe(std::array<uint64_t,7>& used,unsigned rows,unsigned rowBytes,unsigned unit=64) {
	unsigned counts[7]={},chunks=rows/unit,tail=rows%unit,count=chunks<7?1:7;
	if (count==1) counts[0]=rows;
	else { for (unsigned b=0;b<7;b++) counts[b]=(chunks/7+unsigned(b<chunks%7))*unit; counts[6]+=tail; }
	unsigned banks[]={0,1,2,3,4,5,6};
	std::sort(banks,banks+7,[&](unsigned a,unsigned b){return used[a]<used[b] || (used[a]==used[b] && a<b);});
	for (unsigned p=0;p<count;p++) used[banks[p]]+=(uint64_t(counts[p])*rowBytes+4095)&~UINT64_C(4095);
}
int main()
{
	// Exhaust every HALF,each6-bit factor and BOTH coefficient/correction
	// signs against the independent existing integer exact-scale routine.
	uint64_t finite=0,refused=0;
	for (unsigned half=0;half<65536;half++) for (int factor=-63;factor<=63;factor++) {
		uint32_t exact=0,packed=0xdecafbad,decoded=0xdecafbad;
		if (!PioneerQ38PanelScale(uint16_t(half),factor,exact)) { refused++; continue; }
		assert(PioneerQ38PreparedQ4::Encode(exact,packed) && PioneerQ38PreparedQ4::Decode(packed,decoded) && decoded==exact); finite++;
	}
	for (unsigned sign:{0U,0x400000U}) {
		for (unsigned code=46;code<=63;code++) for (unsigned fraction:{0U,1U,32768U,65535U}) {
			uint32_t output=0xdeadbeef;
			bool valid=PioneerQ38PreparedQ4::Decode(sign|(code<<16)|fraction,output);
			assert(valid==(code==63 && !fraction)); if (!valid) assert(output==0xdeadbeef);
		}
		uint32_t output=0xdeadbeef; assert(!PioneerQ38PreparedQ4::Decode(sign|0x800000,output) && output==0xdeadbeef);
	}
	for (uint32_t invalid:{1U,0x00800000U,0x7f800000U,0x7fc00000U,0x32800001U,0x4a800000U}) {
		uint32_t output=0xdeadbeef; assert(!PioneerQ38PreparedQ4::Encode(invalid,output) && output==0xdeadbeef);
	}
	std::array<uint8_t,2304> original;
	std::array<uint8_t,2816> prepared;
	for (unsigned fixture=0;fixture<32;fixture++) {
		for (unsigned i=0;i<original.size();i++) original[i]=uint8_t(i*73+fixture*29);
		for (unsigned row=0;row<16;row++) for (unsigned kind=0;kind<2;kind++) {
			uint16_t half=uint16_t((fixture*2011+row*1249+kind*97)%0x7c00); if ((fixture+row+kind)&1) half|=0x8000;
			original[kind*32+row]=uint8_t(half); original[kind*32+16+row]=uint8_t(half>>8);
		}
		assert(PioneerQ38PreparedQ4::Prepare(original.data(),unsigned(original.size()),prepared.data(),unsigned(prepared.size())));
		for (unsigned byte=0;byte<2048;byte++) assert(prepared[byte]==original[256+byte]);
		for (unsigned panel=0;panel<16;panel++) {
			PioneerQ38Panel expected={},actual={};
			for (auto& word:actual.operand) word=0xdeadbeef;
			assert(PioneerQ38UnpackPanel(12,original.data(),2304,panel,&expected)
				&& PioneerQ38PreparedQ4::Panel(prepared.data(),2816,panel,&actual)); Same(expected,actual);
			for (auto word:actual.operand) assert(word==0xdeadbeef);
		}
	}
	// Each bad original half and EACH malformed prepared coefficient must
	// leave the entire destination byte-for-byte unchanged.
	const auto valid=original;
	const auto validPrepared=prepared;
	for (unsigned half=0;half<32;half++) {
		original=valid; unsigned at=(half/16)*32+half%16; original[at]=1; original[at+16]=0x7c;
		prepared.fill(0x5a); assert(!PioneerQ38PreparedQ4::Prepare(original.data(),2304,prepared.data(),2816));
		for (auto byte:prepared) assert(byte==0x5a);
	}
	for (unsigned word=0;word<256;word++) {
		prepared=validPrepared; prepared[2048+word*3+2]|=0x80;
		PioneerQ38Panel output; memset(&output,0x5a,sizeof(output)); const auto before=output;
		assert(!PioneerQ38PreparedQ4::Panel(prepared.data(),2816,(word/32)*2,&output) && !memcmp(&output,&before,sizeof(output)));
	}
	original=valid; prepared=validPrepared;
	const auto before=prepared;
	assert(!PioneerQ38PreparedQ4::Prepare(nullptr,2304,prepared.data(),2816)
		&& !PioneerQ38PreparedQ4::Prepare(original.data(),2303,prepared.data(),2816)
		&& !PioneerQ38PreparedQ4::Prepare(original.data(),2304,prepared.data(),2815)
		&& !PioneerQ38PreparedQ4::Prepare(prepared.data(),2304,prepared.data(),2816) && prepared==before);
	// Frozen public SHAPES ONLY,not learned data/model authentication or a
	// native allocation. Distinct SELECTED-497-only layout;the old all851
	// tensor plan cannot be silently reused with this larger tile format.
	std::array<uint64_t,7> old={},next={}; unsigned matrices=0; uint64_t q4Original=0;
	for (unsigned ordinal=0;ordinal<=64;ordinal++) for (unsigned role=0;;role++) {
		unsigned layer=ordinal?ordinal-1:64; PioneerQ38Tensor tensor;
		if (!PioneerQ38Expected(layer,role,tensor)) break;
		if (!tensor.type || tensor.name=="token_embd.weight") continue;
		unsigned rowBytes=PioneerQ38TiledRowBytes(tensor.type,tensor.inputs),preparedRow=rowBytes;
		if (tensor.type==12) { preparedRow=unsigned(tensor.inputs/256)*(2816/16); q4Original+=tensor.bytes; }
		Stripe(old,unsigned(tensor.outputs),rowBytes); Stripe(next,unsigned(tensor.outputs),preparedRow); matrices++;
	}
	uint64_t oldTotal=0,newTotal=0; for (unsigned b=0;b<7;b++) { oldTotal+=old[b]; newTotal+=next[b]; }
	const uint64_t futureState=UINT64_C(48)*2*48*65536+UINT64_C(48)*10240*3*4+UINT64_C(16)*8*8192*256*2+UINT64_C(7)*67108864;
	assert(matrices==497 && oldTotal==UINT64_C(18672242688) && newTotal>oldTotal
		&& newTotal+futureState<UINT64_C(7)*3*1024*1024*1024);
	auto withState=next;
	for (unsigned layer=0;layer<64;layer++) {
		if (layer%4==3) Stripe(withState,8,8192*256*2,1);
		else { for (unsigned slot=0;slot<2;slot++) Stripe(withState,48,65536,1); Stripe(withState,10240,3*4); }
	}
	uint64_t fullProposal=0,maximumBank=0;
	for (auto& used:withState) {
		used+=67108864; assert(used<UINT64_C(3)*1024*1024*1024);
		fullProposal+=used; maximumBank=std::max(maximumBank,used);
	}
	assert(fullProposal>=newTotal+futureState && fullProposal<UINT64_C(7)*3*1024*1024*1024);
	printf("OFFLINE_PREPARED_Q4 public_shape_only matrices%u original_q4_bytes%llu original_tiled_bytes%llu proposed_tiled_bytes%llu future_state_workspace_bytes%llu total_proposal%llu;NOT_ALLOCATION_OR_NATIVE_READY\n",
		matrices,(unsigned long long)q4Original,(unsigned long long)oldTotal,(unsigned long long)newTotal,(unsigned long long)futureState,(unsigned long long)fullProposal);
	printf("OFFLINE_PREPARED_Q4 page_aligned_state_included maximum_proposed_bank_bytes%llu remaining_at_most_used_bank%llu;WITHIN_UNCHANGED_3GiB_BANK_LIMIT_NOT_PHYSICAL_ALLOCATION\n",
		(unsigned long long)maximumBank,(unsigned long long)(UINT64_C(3)*1024*1024*1024-maximumBank));
	printf("PASS OFFLINE prepared-Q4 metadata ONLY:exact%llu finite half/factor/sign combinations,%llu nonfinite refusals,512 panels,ALL original quant bytes unchanged,32 bad HALF/256 bad metadata atomic refusal,canonical24-bit signed zeros;NO selected image/reader/native authority/performance claim\n",
		(unsigned long long)finite,(unsigned long long)refused);
}
