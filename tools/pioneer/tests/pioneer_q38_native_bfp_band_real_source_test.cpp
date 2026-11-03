/* SPDX-License-Identifier: MIT */
// ACTUAL original/cache source -> packet -> compiled RV32 -> ideal ISA model.
// CPU reference diagnostic ONLY;never stage or execute any image on a device.
#define PIONEER_NATIVE_BFP_BAND_IMAGE_ONLY
#include "pioneer_q38_native_bfp_band_image_test.cpp"
#include "../PioneerQ38NativeBfpBandSource.h"
#include <memory>
#include <unistd.h>
using WCache=PioneerModelWeightCache;
static NativeBfpBandMachine::Fixture Reference(PioneerQwen38Gguf& model,unsigned tensor,unsigned first,const std::vector<uint16_t>& input,
	const PioneerQ38NativeBfpBandData::Packet& packet) {
	const unsigned width=unsigned(input.size());NativeBfpBandMachine::Fixture reference;reference.packed.resize(width/32);reference.input.resize(width/32);
	for(unsigned tile=0;tile<width/32;tile++) {
		std::array<float,1024> original{},decoded{};
		const uint64_t ordinal=uint64_t(first/32)*(width/32)+tile;
		assert(PioneerQ38NativeWeightTiles::ReadFloatTile(model,tensor,ordinal,original.data(),1024));
		using C=PioneerBlackholeBlockFloat;
		assert(C::PackTile(C::Format::Bfp8B,original.data(),1024,reference.packed[tile].data(),1088));
		// Compare EVERY cached byte with fresh conversion from the authenticated
		// ORIGINAL,not with another copy of the same packet or cache header.
		assert(!memcmp(packet.weights.data()+tile*1104+16,reference.packed[tile].data(),1088));
		assert(C::UnpackTile(C::Format::Bfp8B,reference.packed[tile].data(),1088,decoded.data(),1024));
		for(unsigned c=0;c<32;c++)reference.input[tile][c]=input[tile*32+c];
		for(unsigned half=0;half<2;half++)for(unsigned phase=0;phase<4;phase++)for(unsigned row=0;row<32;row++) {
			long double sum=0;
			for(unsigned c=0;c<16;c++)sum+=static_cast<long double>(NativeBfpMachine::Part(NativeBfpMachine::Float(uint32_t(input[tile*32+half*16+c])<<16),phase,false))*NativeBfpMachine::Part(decoded[row*32+half*16+c],phase,true);
			reference.full[row]=float(float(sum)+reference.full[row]);if(!phase)reference.fast[row]=float(float(sum)+reference.fast[row]);
		}
	}
	return reference;
}
int main(int argc,char** argv) {
	assert(argc==5);std::unique_ptr<FILE,decltype(&fclose)> file(fopen(argv[1],"rb"),&fclose);assert(file);
	PioneerQwen38Gguf model;assert(model.Open(file.get()));const uint64_t epoch=model.Epoch();WCache::Digest original{};std::string error;
	puts("AUTHENTICATING_SAME_OPEN_COMPLETE_ORIGINAL;OFFBOARD_CACHE_SOURCE_AND_RV32_ONLY");fflush(stdout);
	assert(WCache::HashFile(fileno(file.get()),PioneerQwen38Gguf::FullBytes,original,error) && original==PioneerQ38NativeBfpBandSource::OriginalPin() && model.Stable() && model.Epoch()==epoch);
	// Require the existing private cache before setup. The bound source itself
	// NEVER opens/creates directories or entries,converts,repairs or falls back.
	struct stat directory{};assert(!lstat(argv[2],&directory) && S_ISDIR(directory.st_mode) && directory.st_uid==geteuid() && (directory.st_mode&07777)==0700);
	WCache cache;assert(cache.Open(argv[2],error));PioneerQ38NativeBfpBandSource source(cache);
	assert(source.Bind(model,original,[&]{return model.Stable() && model.Epoch()==epoch;},error));
	const auto* tensor=model.Find("blk.0.ffn_gate.weight");assert(tensor && tensor->inputs==5120 && tensor->outputs==17408 && tensor->type==12);
	const unsigned index=unsigned(tensor-model.Tensors().data());std::vector<uint16_t> input(5120);
	for(unsigned i=0;i<input.size();i++)input[i]=uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0));
	unsigned cases=0,pages=0;
	for(unsigned first:{0U,8704U,17376U}) {
		PioneerQ38NativeBfpBandData::Packet packet;
		assert(source.Stage(index,first,input.data(),input.size(),4,packet,error));
		assert(packet.key.sourceSHA256==original && packet.key.conversionSHA256==PioneerQ38NativeBfpBandSource::ConversionPin()
			&& packet.key.entry==index && packet.key.bytes==94699520 && packet.sourceEpoch==epoch && packet.validRows==32);
		const auto reference=Reference(model,index,first,input,packet);pages+=unsigned(reference.packed.size());
		for(unsigned phases:{4U,1U}) {
			if(phases==1)assert(source.Stage(index,first,input.data(),input.size(),1,packet,error));
			BandRV32 image(argv[phases==4?3:4],5120,phases,true);
			assert(packet.weights.size()==176640 && packet.inputs.size()==12800);
			memcpy(image.native.l1.data()+0x8fff0,packet.weights.data(),packet.weights.size());
			memcpy(image.native.l1.data()+0x123ff0,packet.inputs.data(),packet.inputs.size());
			image.native.delay[0]=7;image.native.delay[1]=100003;
			assert(image.Execute());image.Exact(phases==4?reference.full:reference.fast);
			printf("PASS_REAL_CACHED_NATIVE_BAND tensor%u first_row%u K5120 phases%u original_to_cache_bytes_exact174080 two_compiled_RV32_launches_exact64_ideal_outputs;NO_DEVICE_OR_SILICON_ARITHMETIC_QUALITY_SPEED_PROOF\n",index,first,phases);fflush(stdout);cases++;
		}
	}
	assert(source.Ready() && model.Stable() && model.Epoch()==epoch);
	printf("PASS_AUTHENTICATED_ORIGINAL_CACHE_TO_NATIVE_BAND cases%u fresh_original_tiles%u;ZERO_SOURCE_CONVERSION_OR_REPAIR_IN_STAGER_TEST_REFERENCE_CONVERSION_ONLY,NO_HARDWARE_MODEL_QUALITY_OR_THROUGHPUT_PROOF\n",cases,pages);
}
