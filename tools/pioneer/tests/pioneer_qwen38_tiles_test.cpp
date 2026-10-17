/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerResidentBanks.h"
static uint8_t Pattern(unsigned row,unsigned block,unsigned byte)
{
	return uint8_t(row*71+block*11+byte*37); // includes ALL byte values,including0x80
}
static void Geometry(PioneerQwen38Gguf& source)
{
	static_assert(PioneerQwen38PackedPlan::Base==PioneerResidentBanks::Base,"one low reservation");
	static_assert(PioneerQwen38PackedPlan::Capacity==PioneerResidentBanks::Capacity,"one bounded bank policy");
	for (unsigned context:{1U,8192U,32768U}) {
		PioneerQwen38PackedPlan plan; assert(plan.Build(source,context,true) && plan.Tiled());
		uint64_t expected=0,actual=0,ends[7]={},state=0;
		std::vector<std::vector<PioneerQ38PackedPart>> pieces(851);
		for (const auto& t:source.Tensors()) expected+=t.type?uint64_t(PioneerQ38TiledRowBytes(t.type,t.inputs))*t.outputs:t.bytes;
		for (const auto& p:plan.Parts()) {
			assert(p.bank<7 && BlackholeDramBank(p.bank)!=3 && p.proposedAddress==PioneerResidentBanks::Base+ends[p.bank]);
			assert(!(p.proposedAddress&4095) && !(p.allocated&4095) && p.bytes && p.bytes<=p.allocated && p.allocated-p.bytes<4096);
			assert(uint64_t(p.rows)*p.rowBytes==p.bytes);
			ends[p.bank]+=p.allocated; assert(ends[p.bank]<=PioneerResidentBanks::Capacity);
			assert(uint64_t(p.proposedAddress)+p.allocated<=UINT64_C(0xc4000000));
			if (p.kind==PIONEER_Q38_WEIGHT) { actual+=p.bytes; pieces[p.tensor].push_back(p); }
			else state+=p.bytes;
		}
		assert(actual==expected && actual>UINT64_C(18962876416));
		for (unsigned t=0;t<851;t++) {
			auto& parts=pieces[t]; const auto& tensor=source.Tensors()[t];
			std::sort(parts.begin(),parts.end(),[](const PioneerQ38PackedPart& a,const PioneerQ38PackedPart& b){return a.firstRow<b.firstRow;});
			unsigned row=0;
			for (const auto& p:parts) {
				assert(p.firstRow==row && p.rowBytes==(tensor.type?PioneerQ38TiledRowBytes(tensor.type,tensor.inputs):tensor.bytes/tensor.outputs));
				if (tensor.type) assert(!(p.firstRow&15) && !(p.rows&15));
				row+=p.rows;
			}
			assert(row==tensor.outputs);
		}
		uint64_t allocated=0;
		for (unsigned b=0;b<7;b++) { assert(plan.Used(b)==ends[b]); allocated+=ends[b]; }
		printf("OFFLINE_TILED context=%u lossless_weight_bytes=%llu state_bytes=%llu allocated=%llu;NO_GPU_EXECUTION\n",
			context,(unsigned long long)actual,(unsigned long long)state,(unsigned long long)allocated);
	}
}
static void Synthetic()
{
	Fixture fixture;
	std::vector<unsigned> chosen;
	for (unsigned type:{8U,12U,14U}) {
		unsigned tensor=0; while (fixture.expected[tensor].type!=type) tensor++;
		chosen.push_back(tensor); const auto& t=fixture.expected[tensor];
		unsigned blocks=unsigned(t.inputs/PioneerQ38BlockElements(type)),bytes=PioneerQ38BlockBytes(type);
		std::vector<uint8_t> packed(size_t(blocks)*bytes*32);
		for (unsigned row=0;row<32;row++) for (unsigned block=0;block<blocks;block++) for (unsigned byte=0;byte<bytes;byte++)
			packed[(row*blocks+block)*bytes+byte]=Pattern(row,block,byte);
		fixture.Write(t.offset,packed);
	}
	PioneerQwen38Gguf source; assert(source.Open(fixture.file)); Geometry(source);
	PioneerQwen38TiledReader reader; uint8_t output[4098]; memset(output,0x7b,sizeof(output));
	assert(!reader.ReadPage(source,chosen[0],0,32,0,output+1,4096));
	assert(reader.Bind(source) && !reader.Bind(source));
	unsigned checked=0;
	for (unsigned tensor:chosen) {
		const auto& t=source.Tensors()[tensor]; unsigned bytes=PioneerQ38BlockBytes(t.type),tileBytes=PioneerQ38TileBytes(t.type);
		unsigned blocks=unsigned(t.inputs/PioneerQ38BlockElements(t.type)),length=32*PioneerQ38TiledRowBytes(t.type,t.inputs),pages=(length+4095)/4096;
		for (unsigned reverse=0;reverse<2;reverse++) for (unsigned p=0;p<pages;p++) {
			unsigned page=reverse?pages-p-1:p; memset(output,0x7b,sizeof(output));
			assert(reader.ReadPage(source,tensor,0,32,page,output+1,4096));
			assert(output[0]==0x7b && output[4097]==0x7b);
			for (unsigned i=0;i<4096;i++) {
				unsigned offset=page*4096+i,tile=offset/tileBytes,byte=offset%tileBytes; uint8_t expected=0;
				if (offset<length && byte<16*bytes) expected=Pattern((tile/blocks)*16+byte%16,tile%blocks,byte/16);
				assert(output[i+1]==expected); checked++;
			}
		}
		for (unsigned fault=0;fault<9;fault++) {
			memset(output,0x7b,sizeof(output));
			unsigned which=fault==0?851:tensor,first=fault==1?1:fault==2?unsigned(t.outputs):0;
			unsigned rows=fault==3?0:fault==4?17:fault==5?UINT32_MAX:32;
			unsigned page=fault==6?pages:fault==7?UINT32_MAX:0,capacity=fault==8?4095:4096;
			assert(!reader.ReadPage(source,which,first,rows,page,output+1,capacity));
			for (uint8_t byte:output) assert(byte==0x7b);
		}
	}
	PioneerQwen38PackedPlan plan;
	assert(plan.Build(source,8192,true));
	for (unsigned i=0;i<plan.Parts().size();i++) {
		const auto& p=plan.Parts()[i];
		if (p.kind!=PIONEER_Q38_WEIGHT) { assert(!plan.ReadWeightPage(source,i,0,output+1,4096)); continue; }
		for (unsigned page:{0U,p.allocated/4096-1}) assert(plan.ReadWeightPage(source,i,page,output+1,4096));
		assert(!plan.ReadWeightPage(source,i,p.allocated/4096,output+1,4096));
	}
	PioneerQwen38Gguf other; assert(other.Open(fixture.file));
	assert(!reader.ReadPage(other,chosen[0],0,32,0,output+1,4096));
	assert(source.Open(fixture.file)); memset(output,0x7b,sizeof(output));
	assert(!reader.ReadPage(source,chosen[0],0,32,0,output+1,4096));
	assert(!plan.Ready() && !plan.Tiled() && plan.Parts().empty());
	for (uint8_t byte:output) assert(byte==0x7b);
	assert(plan.Build(source,8192) && !plan.Tiled()); // old row mode stays explicit and usable
	reader.Reset(); assert(reader.Bind(source));
	assert(!ftruncate(fileno(fixture.file),off_t(PioneerQwen38Gguf::FullBytes)-1));
	assert(!reader.ReadPage(source,chosen[0],0,32,0,output+1,4096));
	printf("PASS: lossless Q8_0/Q4_K/Q6_K byte-major16-row tiles,ALL%u synthetic bytes in both page orders,zero tile/page padding,alias-free source epochs/bounds;NO_DEVICE\n",checked);
}
static void Real(const char* path)
{
	FILE* file=fopen(path,"rb"); assert(file); PioneerQwen38Gguf source; assert(source.Open(file)); Geometry(source);
	PioneerQwen38PackedPlan plan; assert(plan.Build(source,8192,true));
	uint8_t page[4096]; unsigned sampled=0;
	for (unsigned i=0;i<plan.Parts().size();i++) {
		const auto& p=plan.Parts()[i]; if (p.kind!=PIONEER_Q38_WEIGHT) continue;
		const auto& t=source.Tensors()[p.tensor];
		for (unsigned at:{0U,p.allocated/4096-1}) {
			assert(plan.ReadWeightPage(source,i,at,page,4096));
			if (!t.type) {
				uint64_t offset=uint64_t(at)*4096; unsigned bytes=unsigned(std::min<uint64_t>(4096,p.bytes-offset));
				uint8_t expected[4096]={}; assert(source.ReadPacked(p.tensor,uint64_t(p.firstRow)*p.rowBytes+offset,bytes,expected,4096));
				assert(!memcmp(page,expected,4096));
			} else {
				unsigned bytes=PioneerQ38BlockBytes(t.type),tileBytes=PioneerQ38TileBytes(t.type);
				unsigned blocks=unsigned(t.inputs/PioneerQ38BlockElements(t.type)),originalRow=blocks*bytes;
				std::vector<uint8_t> original(16*originalRow); unsigned prior=UINT32_MAX;
				for (unsigned j=0;j<4096;j++) {
					uint64_t offset=uint64_t(at)*4096+j,tile=offset/tileBytes;
					unsigned byte=unsigned(offset%tileBytes),group=p.firstRow+unsigned(tile/blocks)*16; uint8_t expected=0;
					if (offset<p.bytes && byte<16*bytes) {
						if (group!=prior) { assert(source.ReadPacked(p.tensor,uint64_t(group)*originalRow,unsigned(original.size()),original.data(),unsigned(original.size()))); prior=group; }
						expected=original[(byte%16)*originalRow+unsigned(tile%blocks)*bytes+byte/16];
					}
					assert(page[j]==expected);
				}
			}
			sampled++;
		}
	}
	assert(source.Stable()); fclose(file);
	printf("PASS: actual checkpoint ALL851 tensor fragments/%u first+last tiled/raw pages independently reconstructed byte-exact;NOT_SHA_OR_INFERENCE\n",sampled);
}
int main(int argc,char** argv)
{
	assert(argc==1 || argc==2); Synthetic(); if (argc==2) Real(argv[1]);
}
