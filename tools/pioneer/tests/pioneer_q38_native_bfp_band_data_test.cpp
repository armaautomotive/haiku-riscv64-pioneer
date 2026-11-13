/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY 1
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38NativeBfpBandSource.h"
#include <fcntl.h>
using Data=PioneerQ38NativeBfpBandData;using Cache=PioneerModelWeightCache;
static bool Equal(const Data::Packet& a,const Data::Packet& b) {
	return a.width==b.width && a.matrixRows==b.matrixRows && a.firstRow==b.firstRow && a.validRows==b.validRows && a.phases==b.phases
		&& a.weights==b.weights && a.inputs==b.inputs && a.sourceEpoch==b.sourceEpoch && a.key.entry==b.key.entry && a.key.bytes==b.key.bytes
		&& a.key.sourceSHA256==b.key.sourceSHA256 && a.key.conversionSHA256==b.key.conversionSHA256;
}
struct MemoryReader {
	std::vector<uint8_t> raw;unsigned reads=0,failAt=0,closeAt=0;bool open=true;uint64_t sizeOverride=0;std::function<void()> onRead;
	bool IsOpen() const{return open;}
	uint64_t Size() const{return sizeOverride?sizeOverride:raw.size();}
	bool Read(uint64_t offset,unsigned bytes,void* out,unsigned capacity,std::string& error) {
		reads++;if(reads==failAt || !open || !out || !bytes || bytes>65536 || capacity<bytes || offset>raw.size() || bytes>raw.size()-offset){error="synthetic read refusal";return false;}
		memcpy(out,raw.data()+offset,bytes);if(onRead)onRead();if(reads==closeAt)open=false;return true;
	}
	MemoryReader(unsigned width,unsigned rows) {
		const unsigned tiles=width/32;raw.resize(size_t((rows+31)/32)*tiles*1088);
		for(unsigned band=0;band<(rows+31)/32;band++)for(unsigned tile=0;tile<tiles;tile++) {
			std::array<float,1024> values{};
			for(unsigned r=0;r<32 && band*32+r<rows;r++)for(unsigned c=0;c<32;c++)values[r*32+c]=float(int((r*37+c*19+tile*11+band*23)%255)-127)/128;
			assert(PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,values.data(),1024,raw.data()+(band*tiles+tile)*1088,1088));
		}
	}
};
int main(int argc,char** argv) {
	assert(argc==2);unsigned cases=0;std::string error;
	Data::Packet sentinel;sentinel.width=99;sentinel.weights={0x51,0x62};sentinel.inputs={0x73};sentinel.sourceEpoch=17;
	for(unsigned width:{5120U,6144U,17408U})for(unsigned rows:{1U,17U,32U,48U,64U}) {
		MemoryReader reader(width,rows);std::vector<uint16_t> input(width);
		for(unsigned i=0;i<width;i++)input[i]=uint16_t(0x3f00+(i*13)%256+(i&1?0x8000:0));
		for(unsigned first=0;first<rows;first+=32)for(unsigned phases:{1U,4U}) {
			Data::Packet packet=sentinel;const unsigned before=reader.reads;
			assert(Data::Assemble(reader,width,rows,first,input.data(),width,phases,[]{return true;},packet,error));
			assert(packet.width==width && packet.matrixRows==rows && packet.firstRow==first && packet.validRows==std::min(32U,rows-first) && packet.phases==phases);
			assert(packet.weights.size()==size_t(width/32)*1104 && packet.inputs.size()==size_t(width/32)*80);
			assert(reader.reads-before==(width/32*1088+65535)/65536);
			for(unsigned tile=0;tile<width/32;tile++) {
				for(unsigned h=0;h<16;h++)assert(!packet.weights[tile*1104+h] && !packet.inputs[tile*80+h]);
				assert(!memcmp(packet.weights.data()+tile*1104+16,reader.raw.data()+(size_t(first/32)*(width/32)+tile)*1088,1088));
				for(unsigned c=0;c<32;c++)assert(unsigned(packet.inputs[tile*80+16+c*2])+(unsigned(packet.inputs[tile*80+17+c*2])<<8)==input[tile*32+c]);
			}
			cases++;
		}
	}
	std::vector<uint16_t> input(5120,0x3f80);
	{
		MemoryReader reader(5120,48);reader.onRead=[&]{input[0]=0x7f80;};Data::Packet packet;
		assert(Data::Assemble(reader,5120,48,0,input.data(),5120,4,[]{return true;},packet,error));
		assert(input[0]==0x7f80 && packet.inputs[16]==0x80 && packet.inputs[17]==0x3f);input[0]=0x3f80;cases++;
	}
	for(unsigned bad=0;bad<18;bad++) {
		MemoryReader reader(5120,48);Data::Packet packet=sentinel;bool ready=true;unsigned readyCalls=0;
		unsigned width=5120,rows=48,first=0,count=5120,phases=4;const uint16_t* values=input.data();
		if(bad==0)width=5119;if(bad==1)rows=0;if(bad==2)first=48;if(bad==3)first=1;
		if(bad==4)count=5119;if(bad==5)phases=2;if(bad==6)values=nullptr;
		if(bad==7)values=reinterpret_cast<const uint16_t*>(reinterpret_cast<const uint8_t*>(input.data())+1);
		if(bad==8)input[0]=0x7f80;if(bad==9)input[0]=1;
		if(bad==10)reader.open=false;if(bad==11)reader.sizeOverride=reader.raw.size()+1;
		if(bad==12)ready=false;if(bad==13)reader.failAt=2;if(bad==14)reader.closeAt=2;
		if(bad==17)values=reinterpret_cast<const uint16_t*>(UINTPTR_MAX-1);
		auto stable=[&]{readyCalls++;return ready && (bad!=15 || reader.reads<1) && (bad!=16 || readyCalls<8);};
		assert(!Data::Assemble(reader,width,rows,first,values,count,phases,stable,packet,error) && Equal(packet,sentinel) && !error.empty());
		if(bad<13 || bad==17)assert(!reader.reads);input[0]=0x3f80;cases++;
	}
	for(unsigned corruption=0;corruption<6;corruption++) {
		MemoryReader reader(5120,48);Data::Packet packet=sentinel;unsigned first=0;
		if(corruption==0)reader.raw[0]=255;
		if(corruption==1)reader.raw[64]=0x80;
		if(corruption==2){reader.raw[0]=0;reader.raw[64]=1;}
		if(corruption==3){reader.raw[0]=127;memset(reader.raw.data()+64,1,16);}
		if(corruption==4){first=32;auto* p=reader.raw.data()+160*1088;p[32]=127;memset(p+64+32*16,0,16);p[64+32*16]=64;}
		if(corruption==5)reader.raw[0]=7;
		assert(!Data::Assemble(reader,5120,48,first,input.data(),5120,4,[]{return true;},packet,error) && Equal(packet,sentinel));cases++;
	}
	// Whole 32-row stripes are read from the verified cache as packed pages,
	// with zero padding rather than leaking the following stripe's bytes.
	for(unsigned width:{5120U,17408U})for(unsigned first:{0U,32U}){
		MemoryReader reader(width,64);const uint64_t bandBytes=uint64_t(width/32)*1088;
		const unsigned pages=unsigned((bandBytes+4095)/4096);std::array<uint8_t,4096> page;
		for(unsigned n=0;n<pages;n++){
			page.fill(0xa5);assert(Data::ReadStripePage(reader,width,64,first,32,n,[]{return true;},page.data(),page.size(),error));
			const uint64_t at=uint64_t(n)*4096;const unsigned take=unsigned(std::min(uint64_t(4096),bandBytes-at));
			assert(!memcmp(page.data(),reader.raw.data()+uint64_t(first/32)*bandBytes+at,take));
			for(unsigned i=take;i<4096;i++)assert(!page[i]);
		}
		// Actual multi-page path, including an unaligned tensor offset and
		// zero padding in the last batch, must match individual pages.
		std::vector<uint8_t> batch(16*4096,0xa5),expected(batch.size());
		for(unsigned n=0;n<pages;){
			const unsigned count=std::min(16U,pages-n);const unsigned before=reader.reads;
			assert(Data::ReadStripePages(reader,width,64,first,32,n,count,[]{return true;},batch.data(),batch.size(),error));
			assert(reader.reads==before+1);
			for(unsigned i=0;i<count;i++)assert(Data::ReadStripePage(reader,width,64,first,32,n+i,[]{return true;},expected.data()+i*4096,4096,error));
			assert(!memcmp(batch.data(),expected.data(),count*4096));n+=count;
		}
		const auto beforeBatch=batch;
		assert(!Data::ReadStripePages(reader,width,64,first,32,pages-1,2,[]{return true;},batch.data(),batch.size(),error)
			&& batch==beforeBatch);
		page.fill(0xa5);const auto saved=page;const unsigned before=reader.reads;
		assert(!Data::ReadStripePage(reader,width,64,first,32,pages,[]{return true;},page.data(),page.size(),error));
		assert(page==saved && reader.reads==before);cases++;
	}
	for(unsigned bad=0;bad<10;bad++){
		MemoryReader reader(5120,64);std::array<uint8_t,4096> page;page.fill(0xa5);const auto saved=page;bool ready=true;
		unsigned first=0,rows=32,width=5120,capacity=4096;uint8_t* out=page.data();
		if(bad==0)first=1;if(bad==1)rows=31;if(bad==2)width=5119;if(bad==3)capacity=4095;
		if(bad==4)out=nullptr;if(bad==5)reader.failAt=1;if(bad==6)reader.onRead=[&]{ready=false;};
		if(bad==7)reader.sizeOverride=reader.raw.size()+1;if(bad==8)ready=false;
		if(bad==9)out=reinterpret_cast<uint8_t*>(UINTPTR_MAX-1);
		assert(!Data::ReadStripePage(reader,width,64,first,rows,0,[&]{return ready;},out,capacity,error) && page==saved);
		if(bad<5 || bad>=7)assert(!reader.reads);cases++;
	}
	for(unsigned bad=0;bad<4;bad++){
		MemoryReader reader(5120,64);std::vector<uint8_t> output(16*4096,0xa5);const auto saved=output;bool ready=true;
		unsigned count=16,capacity=unsigned(output.size());
		if(bad==0)count=17;if(bad==1)capacity--;
		if(bad==2)reader.failAt=1;if(bad==3)reader.onRead=[&]{ready=false;};
		assert(!Data::ReadStripePages(reader,5120,64,0,32,0,count,[&]{return ready;},output.data(),capacity,error)
		 && output==saved);
		if(bad<2)assert(!reader.reads);cases++;
	}
	// Exact851-tensor SPARSE fixture;its declared pin/Ready is intentionally
	// synthetic,NOT proof of full original SHA authentication or model quality.
	Fixture fixture;PioneerQwen38Gguf model;assert(model.Open(fixture.file));Cache cache;
	assert(cache.Open((std::string(argv[1])+"/synthetic-native").c_str(),error));
	using Source=PioneerQ38NativeBfpBandSource;bool ready=true;
	auto stable=[&]{return ready && model.Stable();};
	Source badPin(cache);auto wrong=Cache::Hash("SYNTHETIC",9);
	assert(!badPin.Bind(model,wrong,stable,error) && !badPin.Ready() && !badPin.Bind(model,Source::OriginalPin(),stable,error));cases++;
	{
		Source cancelled(cache);unsigned callbacks=0;bool allow=false;
		auto interrupted=[&]{return allow || ++callbacks==1;};
		assert(!cancelled.Bind(model,Source::OriginalPin(),interrupted,error) && !error.empty());
		allow=true;assert(!cancelled.Ready() && !cancelled.Bind(model,Source::OriginalPin(),stable,error));cases++;
	}
	Source source(cache);assert(source.Bind(model,Source::OriginalPin(),stable,error) && !source.Bind(model,Source::OriginalPin(),stable,error));
	const unsigned tensor=unsigned(model.Find("blk.0.ssm_alpha.weight")-model.Tensors().data());Cache::Key key;
	assert(source.KeyFor(tensor,key) && key.bytes==348160 && key.conversionSHA256==Source::ConversionPin());
	Cache::Key unused;assert(!source.KeyFor(851,unused) && !source.KeyFor(unsigned(model.Find("token_embd.weight")-model.Tensors().data()),unused));cases++;
	Data::Packet packet=sentinel;assert(!source.Stage(tensor,0,input.data(),5120,4,packet,error) && Equal(packet,sentinel));cases++;
	MemoryReader data(5120,48);unsigned conversions=0;
	auto produce=[&](uint64_t offset,unsigned bytes,uint8_t* out,std::string&){conversions++;memcpy(out,data.raw.data()+offset,bytes);return true;};
	{
		Cache::Key other=key;other.conversionSHA256=Cache::Hash("WRONG-RECIPE",12);Cache::Reader r;bool hit=false;
		assert(cache.GetOrCreate(other,stable,produce,r,hit,error));
		assert(!source.Stage(tensor,0,input.data(),5120,4,packet,error) && Equal(packet,sentinel));cases++;
	}
	{
		Cache::Reader r;bool hit=false;assert(cache.GetOrCreate(key,stable,produce,r,hit,error));
	}
	const unsigned before=conversions;
	{
		std::array<uint8_t,4096> page;const unsigned last=unsigned((uint64_t(160)*1088-1)/4096);
		std::vector<uint8_t> batched(16*4096,0);
		assert(source.ReadStripePages(tensor,0,32,1,16,batched.data(),batched.size(),error)
		 && !memcmp(batched.data(),data.raw.data()+4096,batched.size()));
		assert(source.ReadStripePage(tensor,0,32,0,page.data(),page.size(),error) && !memcmp(page.data(),data.raw.data(),4096));
		assert(source.ReadStripePage(tensor,0,32,last,page.data(),page.size(),error));
		const unsigned take=160*1088-last*4096;assert(!memcmp(page.data(),data.raw.data()+last*4096,take));
		for(unsigned i=take;i<4096;i++)assert(!page[i]);
		const auto saved=page;assert(!source.ReadStripePage(tensor,0,32,last+1,page.data(),page.size(),error) && page==saved);
		assert(conversions==before);cases++;
	}
	{
		std::vector<uint8_t> bytes(16*4096,0xa5);
		for(unsigned page=0;page<85;){
			const unsigned count=std::min(16U,85-page);
			assert(source.ReadPaddedSmallPages(tensor,page,count,bytes.data(),bytes.size(),error));
			assert(!memcmp(bytes.data(),data.raw.data()+page*4096,count*4096));page+=count;
		}
		const auto saved=bytes;
		assert(!source.ReadPaddedSmallPages(tensor,84,2,bytes.data(),bytes.size(),error) && bytes==saved);
		assert(!source.ReadPaddedSmallPages(tensor,0,0,bytes.data(),bytes.size(),error) && bytes==saved);
		const unsigned notSmall=unsigned(model.Find("blk.0.attn_gate.weight")-model.Tensors().data());
		assert(!source.ReadPaddedSmallPages(notSmall,0,1,bytes.data(),bytes.size(),error) && bytes==saved);
		assert(conversions==before);cases++;
	}
	for(unsigned first:{0U,32U})for(unsigned phases:{1U,4U}) {
		assert(source.Stage(tensor,first,input.data(),5120,phases,packet,error) && packet.key.entry==tensor && packet.key.bytes==key.bytes && packet.key.sourceSHA256==Source::OriginalPin() && packet.key.conversionSHA256==Source::ConversionPin() && packet.sourceEpoch==model.Epoch() && conversions==before);cases++;
	}
	{
		Source snap(cache);bool armed=false;unsigned callbacks=0;
		auto hook=[&]{if(armed && ++callbacks==2)input[0]=0x7f80;return model.Stable();};
		assert(snap.Bind(model,Source::OriginalPin(),hook,error));armed=true;
		assert(snap.Stage(tensor,0,input.data(),5120,4,packet,error) && input[0]==0x7f80 && packet.inputs[16]==0x80 && packet.inputs[17]==0x3f);input[0]=0x3f80;cases++;
	}
	ready=false;packet=sentinel;assert(!source.Stage(tensor,0,input.data(),5120,4,packet,error) && Equal(packet,sentinel));ready=true;cases++;
	const std::string entry=std::string(argv[1])+"/synthetic-native/"+Cache::EntryName(key);
	assert(!chmod(entry.c_str(),0600));int fd=open(entry.c_str(),O_WRONLY);assert(fd>=0);uint8_t byte=0;assert(pwrite(fd,&byte,1,0)==1 && !close(fd) && !chmod(entry.c_str(),0400));
	assert(!source.Stage(tensor,0,input.data(),5120,4,packet,error) && Equal(packet,sentinel) && error.find("not reopened")!=std::string::npos);cases++;
	std::array<uint8_t,4096> corruptedPage;corruptedPage.fill(0xa5);const auto beforeCorruption=corruptedPage;
	assert(!source.ReadStripePage(tensor,0,32,0,corruptedPage.data(),corruptedPage.size(),error)
		&& corruptedPage==beforeCorruption && error.find("not reopened")!=std::string::npos);cases++;
	assert(model.Open(fixture.file) && !source.Ready());assert(!source.Stage(tensor,0,input.data(),5120,4,packet,error));cases++;
	printf("PASS native cache-to-band DATA:%u checks;all widths/padded rows/fidelity variants,zero headers/exact payload and BF16 bytes,bounded chunk reads,atomic failure,invalid/corrupt/source-revoked refusal;frozen original/recipe/tensor key,no create/repair/conversion in bound source,changed reader NOT reopened;SYNTHETIC declared pin is NOT authentication,NO GPU/model-quality/speed proof\n",cases);
}
