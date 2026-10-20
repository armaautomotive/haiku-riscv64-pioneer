/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen38Gguf.h"
#include "../PioneerQwen38PackedPlan.h"
#include <cassert>
#include <cmath>
#include <unistd.h>
#include <map>

struct Fixture {
	FILE* file=tmpfile(); std::vector<uint8_t> header;
	std::vector<PioneerQ38Tensor> expected;
	std::vector<unsigned> dimensionPositions,typePositions,offsetPositions;
	std::map<std::string,unsigned> meta;
	uint64_t dataStart=0;
	void Number(uint64_t n,unsigned bytes) { for (unsigned j=0;j<bytes;j++) header.push_back(uint8_t(n>>(j*8))); }
	void Text(const std::string& s) { Number(s.size(),8); header.insert(header.end(),s.begin(),s.end()); }
	void Scalar(const char* key,unsigned type,uint64_t value) { Text(key); Number(type,4); meta[key]=unsigned(header.size()); Number(value,4); }
	void String(const char* key,const char* value) { Text(key); Number(8,4); meta[key]=unsigned(header.size()); Text(value); }
	Fixture() {
		assert(file); Number(0x46554747,4); Number(3,4); Number(851,8); Number(22,8);
		String("general.architecture","qwen35"); String("general.name","Qwen3.8-27B");
		const std::pair<const char*,unsigned> fields[]={
			{"qwen35.block_count",64},{"qwen35.context_length",262144},{"qwen35.embedding_length",5120},
			{"qwen35.feed_forward_length",17408},{"qwen35.attention.head_count",24},{"qwen35.attention.head_count_kv",4},
			{"qwen35.attention.key_length",256},{"qwen35.attention.value_length",256},{"qwen35.ssm.conv_kernel",4},
			{"qwen35.ssm.state_size",128},{"qwen35.ssm.group_count",16},{"qwen35.ssm.time_step_rank",48},
			{"qwen35.ssm.inner_size",6144},{"qwen35.full_attention_interval",4},{"qwen35.rope.dimension_count",64}};
		for (const auto& field:fields) Scalar(field.first,4,field.second);
		Scalar("qwen35.attention.layer_norm_rms_epsilon",6,PioneerAttentionBits(1e-6f));
		Scalar("qwen35.rope.freq_base",6,PioneerAttentionBits(10000000.f));
		Text("qwen35.rope.dimension_sections"); Number(9,4); Number(5,4); Number(4,8);
		meta["sections"]=unsigned(header.size()); for (unsigned n:{11U,11U,10U,0U}) Number(n,4);
		Text("qwen35.attention.recurrent_layers"); Number(9,4); Number(7,4); Number(64,8);
		meta["recurrent"]=unsigned(header.size()); for (unsigned j=0;j<64;j++) Number(j%4!=3,1);
		// Unknown template is skipped as inert data,not parsed/evaluated.
		String("tokenizer.chat_template","{{ MUST_NEVER_EXECUTE }}");
		uint64_t offset=0;
		for (unsigned layer=0;layer<=64;layer++) for (unsigned role=0;;role++) {
			PioneerQ38Tensor tensor; if (!PioneerQ38Expected(layer,role,tensor)) break;
			tensor.offset=offset; expected.push_back(tensor); offset=(offset+tensor.bytes+31)&~UINT64_C(31);
		}
		assert(expected.size()==851);
		for (const auto& entry:expected) {
			Text(entry.name); Number(entry.rank,4); dimensionPositions.push_back(unsigned(header.size()));
			Number(entry.inputs,8); if (entry.rank==2) Number(entry.outputs,8);
			typePositions.push_back(unsigned(header.size())); Number(entry.type,4);
			offsetPositions.push_back(unsigned(header.size())); Number(entry.offset,8);
		}
		dataStart=(header.size()+31)&~uint64_t(31);
		assert(dataStart+offset<PioneerQwen38Gguf::FullBytes);
		assert(ftruncate(fileno(file),off_t(PioneerQwen38Gguf::FullBytes))==0);
		assert(fwrite(header.data(),1,header.size(),file)==header.size() && !fflush(file));
		for (auto& tensor:expected) tensor.offset+=dataStart;
	}
	~Fixture() { fclose(file); }
	void Modify(unsigned offset,uint8_t delta) {
		uint8_t byte=header.at(offset)^delta;
		assert(!fseeko(file,offset,SEEK_SET) && fwrite(&byte,1,1,file)==1 && !fflush(file));
	}
	void Restore(unsigned offset) {
		uint8_t byte=header.at(offset);
		assert(!fseeko(file,offset,SEEK_SET) && fwrite(&byte,1,1,file)==1 && !fflush(file));
	}
	void Write(uint64_t offset,const std::vector<uint8_t>& bytes) {
		assert(!fseeko(file,off_t(offset),SEEK_SET) && fwrite(bytes.data(),1,bytes.size(),file)==bytes.size() && !fflush(file));
	}
};
#ifndef PIONEER_Q38_FIXTURE_ONLY
static void Fixtures()
{
	Fixture fixture; PioneerQwen38Gguf index;
	assert(index.Open(fixture.file) && index.Ready() && index.Tensors().size()==851);
	for (unsigned i=0;i<851;i++) {
		const auto& a=index.Tensors()[i]; const auto& b=fixture.expected[i];
		assert(a.name==b.name && a.inputs==b.inputs && a.outputs==b.outputs && a.rank==b.rank && a.type==b.type && a.offset==b.offset && a.bytes==b.bytes);
	}
	for (unsigned i=0;i<851;i++) for (unsigned at:{fixture.dimensionPositions[i],fixture.typePositions[i]}) {
		fixture.Modify(at,1); assert(!index.Open(fixture.file) && !index.Ready() && index.Tensors().empty() && !index.Find("output.weight")); fixture.Restore(at);
	}
	for (const auto& field:fixture.meta) {
		fixture.Modify(field.second,1); bool accepted=index.Open(fixture.file);
		// An UNKNOWN inert template is not part of architecture validation.
		if (field.first!="tokenizer.chat_template") assert(!accepted);
		fixture.Restore(field.second);
	}
	for (unsigned at:{fixture.offsetPositions[0],fixture.offsetPositions[1],fixture.offsetPositions.back()}) {
		fixture.Modify(at,1); assert(!index.Open(fixture.file)); fixture.Restore(at);
	}
	assert(index.Open(fixture.file));
	for (unsigned wanted:{0U,8U,12U,14U}) {
		unsigned which=0; while (fixture.expected.at(which).type!=wanted) which++;
		const auto& entry=fixture.expected[which];
		unsigned block=PioneerQ38BlockElements(wanted),bytes=PioneerQ38BlockBytes(wanted);
		std::vector<uint8_t> row(unsigned(entry.bytes/entry.outputs));
		for (unsigned at=0;at<entry.inputs;at+=block) {
			uint8_t* p=row.data()+(at/block)*bytes;
			if (wanted==0) { uint32_t bits=PioneerAttentionBits(5.25f); for (unsigned j=0;j<4;j++) p[j]=uint8_t(bits>>(8*j)); }
			if (wanted==8) { p[1]=0x38; for (unsigned j=2;j<34;j++) p[j]=7; }
			if (wanted==12) { p[1]=0x3c; p[4]=1; for (unsigned j=16;j<144;j++) p[j]=0x11; }
			if (wanted==14) { for (unsigned j=0;j<128;j++) p[j]=0x11; for (unsigned j=128;j<192;j++) p[j]=0xaa;
				for (unsigned j=192;j<208;j++) p[j]=1; p[209]=0x3c; }
		}
		fixture.Write(entry.offset,row); assert(index.Open(fixture.file));
		std::vector<float> output(entry.inputs+2,-123);
		assert(index.ReadRow(which,0,output.data()+1,unsigned(entry.inputs)));
		for (unsigned i=0;i<entry.inputs;i++) assert(output[1+i]==(wanted==0?5.25f:wanted==8?3.5f:wanted==14?1:i%256<32?1:0));
		assert(output.front()==-123 && output.back()==-123);
		assert(!index.ReadRow(which,unsigned(entry.outputs),output.data(),unsigned(output.size())));
		assert(!index.ReadRow(which,0,output.data(),unsigned(entry.inputs)-1));
		uint8_t data[8]={}; assert(!index.ReadPacked(which,entry.bytes-4,8,data,sizeof(data)));
		assert(!index.ReadPacked(which,0,8,data,7)); assert(!index.ReadPacked(851,0,8,data,8));
		// Nonfinite scale/value cannot escape the row decoder.
		if (wanted==0) { row[2]=0x80; row[3]=0x7f; }
		else if (wanted==14) { row[208]=0; row[209]=0x7c; }
		else row[1]=0x7c;
		fixture.Write(entry.offset,row); assert(index.Open(fixture.file));
		assert(!index.ReadRow(which,0,output.data(),unsigned(output.size())));
	}
	assert(index.Open(fixture.file)); assert(!ftruncate(fileno(fixture.file),off_t(PioneerQwen38Gguf::FullBytes)-1));
	uint8_t data[8]={}; assert(!index.Stable() && !index.ReadPacked(0,0,8,data,8));
}
static void RealModel(const char* path)
{
	FILE* file=fopen(path,"rb"); assert(file); PioneerQwen38Gguf index; assert(index.Open(file));
	unsigned formats[15]={}; uint64_t values=0,packed=0; std::vector<float> row(17408);
	for (unsigned i=0;i<index.Tensors().size();i++) {
		const auto& entry=index.Tensors()[i]; formats[entry.type]++; packed+=entry.bytes;
		for (unsigned r:{0U,unsigned(entry.outputs-1)}) {
			assert(index.ReadRow(i,r,row.data(),unsigned(row.size()))); values+=entry.inputs;
			for (unsigned j=0;j<entry.inputs;j++) assert(std::isfinite(row[j]));
			if (entry.name.size()>=5 && entry.name.compare(entry.name.size()-5,5,"ssm_a")==0)
				for (unsigned j=0;j<48;j++) assert(row[j]<0);
		}
	}
	assert(formats[0]==353 && formats[8]==288 && formats[12]==193 && formats[14]==17 && packed==UINT64_C(18962876416));
	assert(index.Stable());
	printf("PASS: actual requested GGUF ALL851 layouts,first+last rows/%llu finite values/mixed Q4_K+Q6_K+Q8_0+F32/%llu packed bytes/negative decay;read-only,NOT inference or SHA authentication\n",(unsigned long long)values,(unsigned long long)packed);
	fclose(file);
}
static void BankPlan()
{
	Fixture fixture; PioneerQwen38Gguf source; assert(source.Open(fixture.file));
	PioneerQwen38PackedPlan plan;
	for (unsigned context:{1U,8192U,32768U}) {
		assert(plan.Build(source,context) && plan.Ready());
		uint64_t ends[7]={},weights=0,state[5]={};
		std::vector<std::vector<PioneerQ38PackedPart>> tensors(851);
		for (const auto& part:plan.Parts()) {
			assert(part.bank<7 && part.proposedAddress==PioneerQwen38PackedPlan::Base+ends[part.bank]);
			assert(!(part.proposedAddress&4095) && !(part.allocated&4095) && part.bytes && part.bytes<=part.allocated && part.allocated-part.bytes<4096);
			assert(uint64_t(part.rows)*part.rowBytes==part.bytes && part.kind<=PIONEER_Q38_WORKSPACE);
			ends[part.bank]+=part.allocated; assert(ends[part.bank]<=PioneerQwen38PackedPlan::Capacity);
			assert(uint64_t(part.proposedAddress)+part.allocated<=UINT64_C(0xff000000));
			if (part.kind==PIONEER_Q38_WEIGHT) { assert(part.tensor<851); weights+=part.bytes; tensors[part.tensor].push_back(part); }
			else { assert(part.tensor==851); state[part.kind]+=part.bytes; }
		}
		assert(weights==UINT64_C(18962876416));
		assert(state[PIONEER_Q38_DELTA_STATE]==150994944 && state[PIONEER_Q38_CONV_STATE]==5898240
			&& state[PIONEER_Q38_KV_STATE]==uint64_t(16)*context*4096 && state[PIONEER_Q38_WORKSPACE]==uint64_t(7)*64*1024*1024);
		for (unsigned i=0;i<851;i++) {
			const auto& tensor=source.Tensors()[i];
			auto& pieces=tensors[i]; std::sort(pieces.begin(),pieces.end(),[](const PioneerQ38PackedPart& a,const PioneerQ38PackedPart& b){return a.firstRow<b.firstRow;});
			unsigned row=0,mask=0;
			for (const auto& part:pieces) {
				assert(part.firstRow==row && part.rowBytes==tensor.bytes/tensor.outputs);
				mask|=1U<<part.bank; row+=part.rows;
			}
			assert(row==tensor.outputs);
			if (tensor.outputs>=448) assert(mask==127 && pieces.size()==7);
		}
		for (unsigned b=0;b<7;b++) assert(plan.Used(b)==ends[b]);
	}
	assert(plan.Build(source,8192));
	uint8_t page[4098];
	for (unsigned i=0;i<plan.Parts().size();i++) {
		const auto& part=plan.Parts()[i];
		if (part.kind!=PIONEER_Q38_WEIGHT) { assert(!plan.ReadWeightPage(source,i,0,page,4096)); continue; }
		for (unsigned p:{0U,part.allocated/4096-1}) {
			memset(page,0x7b,sizeof(page)); assert(plan.ReadWeightPage(source,i,p,page+1,4096));
			assert(page[0]==0x7b && page[4097]==0x7b);
			// Synthetic weight file is sparse zero data,including padding.
			for (unsigned j=1;j<=4096;j++) assert(page[j]==0);
		}
		assert(!plan.ReadWeightPage(source,i,part.allocated/4096,page,4096));
		assert(!plan.ReadWeightPage(source,i,0,page,4095));
	}
	PioneerQwen38Gguf other; assert(other.Open(fixture.file));
	assert(!plan.ReadWeightPage(other,0,0,page,4096));
	assert(source.Open(fixture.file)); assert(!plan.ReadWeightPage(source,0,0,page,4096));
	assert(!plan.Ready() && plan.Parts().empty());
	for (unsigned bank=0;bank<7;bank++) assert(!plan.Used(bank));
	for (unsigned invalid:{0U,32769U,262144U,UINT32_MAX}) {
		assert(!plan.Build(source,invalid) && !plan.Ready() && plan.Parts().empty() && !plan.Used(0));
	}
	printf("PASS: canonical seven-bank packed blueprint,ALL851 rows/bytes/non-overlap/NIU reservation,contexts1+8K+32K,zeroed page padding/source binding;NO allocation/upload authority\n");
}
static void DoubleBufferedBankPlan()
{
	Fixture fixture; PioneerQwen38Gguf source; assert(source.Open(fixture.file));
	PioneerQwen38PackedPlan legacy,plan;
	PioneerQ38DeltaHalfProposal proposal={99,99,99,99};
	assert(!plan.DeltaHalf(0,0,0,0,proposal) && proposal.proposedAddress==99 && !plan.DeltaSlots());
	for (bool tiled:{false,true}) for (unsigned context:{1U,8192U,32768U}) {
		assert(legacy.Build(source,context,tiled) && legacy.DeltaSlots()==1);
		assert(!legacy.DeltaHalf(0,0,0,0,proposal)); // no accidental legacy opt-in
		assert(plan.Build(source,context,tiled,true) && plan.DeltaSlots()==2);
		uint64_t ends[7]={},deltaBytes[2]={}; unsigned weights=0;
		for (const auto& part:plan.Parts()) {
			assert(part.bank<7 && part.proposedAddress==PioneerQwen38PackedPlan::Base+ends[part.bank]);
			assert(!(part.proposedAddress&4095) && !(part.allocated&4095) && part.bytes && part.bytes<=part.allocated);
			ends[part.bank]+=part.allocated; assert(ends[part.bank]<=PioneerQwen38PackedPlan::Capacity);
			if (part.kind==PIONEER_Q38_WEIGHT) {
				const auto& old=legacy.Parts()[weights++];
				assert(old.kind==part.kind && old.layer==part.layer && old.role==part.role && old.tensor==part.tensor
					&& old.bank==part.bank && old.firstRow==part.firstRow && old.rows==part.rows && old.rowBytes==part.rowBytes
					&& old.proposedAddress==part.proposedAddress && old.bytes==part.bytes && old.allocated==part.allocated);
			}
			if (part.kind==PIONEER_Q38_DELTA_STATE) { assert(part.role<2); deltaBytes[part.role]+=part.bytes; }
		}
		assert(deltaBytes[0]==150994944 && deltaBytes[1]==150994944);
		for (unsigned bank=0;bank<7;bank++) assert(ends[bank]==plan.Used(bank));
		for (unsigned layer=0;layer<64;layer++) for (unsigned slot=0;slot<2;slot++) for (unsigned head=0;head<48;head++) {
			PioneerQ38DeltaHalfProposal a={},b={};
			if (layer%4==3) { assert(!plan.DeltaHalf(layer,slot,head,0,a)); continue; }
			assert(plan.DeltaHalf(layer,slot,head,0,a) && plan.DeltaHalf(layer,slot,head,1,b));
			assert(a.fragment==b.fragment && a.bank==b.bank && a.bytes==32768 && b.bytes==32768
				&& b.proposedAddress==a.proposedAddress+32768);
			const auto& p=plan.Parts()[a.fragment];
			assert(p.kind==PIONEER_Q38_DELTA_STATE && p.role==slot && p.layer==layer
				&& a.proposedAddress==p.proposedAddress+(head-p.firstRow)*65536);
			PioneerQ38DeltaHalfProposal opposite={}; assert(plan.DeltaHalf(layer,slot^1,head,0,opposite));
			assert(opposite.bank!=a.bank || opposite.proposedAddress!=a.proposedAddress);
		}
		for (unsigned field=0;field<4;field++) {
			proposal={99,99,99,99}; assert(!plan.DeltaHalf(field==0?64:0,field==1?2:0,field==2?48:0,field==3?2:0,proposal));
			assert(proposal.fragment==99 && proposal.bank==99 && proposal.proposedAddress==99 && proposal.bytes==99);
		}
	}
	assert(source.Open(fixture.file) && !plan.DeltaSlots() && !plan.DeltaHalf(0,0,0,0,proposal));
	assert(!plan.Build(source,0,true,true) && !plan.DeltaSlots());
	puts("PASS OFFLINE GDN two-slot placement:row/tile-major contexts1/8K/32K,ALL48 layers x2 slots x96 half-head ranges disjoint/in-bounds,288MiB state,ALL learned weight proposals unchanged,legacy/stale/invalid refuse;NO allocation/transport authority");
}
int main(int argc,char** argv)
{
	assert(argc==1 || argc==2); Fixtures(); BankPlan(); DoubleBufferedBankPlan(); if (argc==2) RealModel(argv[1]);
	puts("PASS: bounded Qwen3.8 packed tensor reader,ALL851 shape/format refusals,metadata/layout/range/change/NaN/Inf guards,no template execution or device access");
}
#endif
