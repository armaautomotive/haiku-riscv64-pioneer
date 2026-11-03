/* SPDX-License-Identifier: MIT */
// Read-only full-source-authenticated CPU inventory + deterministic tile sample.
// This never creates a disk cache, opens a device, or grants bank authority.
#include "PioneerQ38NativeBfp8WeightTiles.h"
#include "PioneerQ38NativeBfp8Plan.h"
#include "PioneerQ38NativeFidelityView.h"
#include "PioneerModelWeightCache.h"
#include <cmath>
#include <cstdio>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

namespace {
struct Metric {
	uint64_t tensors=0,tiles=0,mixedBytes=0,bfp8Bytes=0;
	double energy=0,mixedError=0,bfp8Error=0,phase0OperandError=0;
};
int Run(int argc,char** argv) {
	if(argc!=3 || strcmp(argv[1],"--sample-original-tiles"))return 2;
	std::unique_ptr<FILE,decltype(&fclose)> file(fopen(argv[2],"rb"),&fclose);
	PioneerQwen38Gguf source;
	if(!file || !source.Open(file.get()))return 1;
	const uint64_t epoch=source.Epoch();PioneerModelWeightCache::Digest digest{};std::string error;
	puts("AUTHENTICATING_SAME_OPEN_COMPLETE_ORIGINAL;READ_ONLY_CPU_SAMPLE_NO_CACHE_OR_GPU");fflush(stdout);
	if(!PioneerModelWeightCache::HashFile(fileno(file.get()),PioneerQwen38Gguf::FullBytes,digest,error)
		|| !source.Stable() || source.Epoch()!=epoch
		|| PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747")return 1;
	std::array<Metric,3> stats{};uint64_t otherBytes=0,embeddingBytes=0;
	for(unsigned index=0;index<source.Tensors().size();index++) {
		const auto& tensor=source.Tensors()[index];PioneerQ38NativeWeightTiles::Layout mixed,eight;
		if(!PioneerQ38NativeBfp8WeightTiles::Describe(tensor,eight)) {otherBytes+=tensor.bytes;continue;}
		if(!PioneerQ38NativeWeightTiles::Describe(tensor,mixed))return 1;
		Metric& metric=stats[tensor.type==8?0:tensor.type==12?1:2];
		metric.tensors++;metric.mixedBytes+=mixed.bytes;metric.bfp8Bytes+=eight.bytes;
		if(tensor.name=="token_embd.weight")embeddingBytes+=eight.bytes;
		const uint64_t tiles=eight.rowTiles*eight.columnTiles;
		for(uint64_t tile:{UINT64_C(0),tiles/2,tiles-1}) {
			std::array<float,1024> original{},decoded{};std::array<uint8_t,1088> bytes{};
			if(!PioneerQ38NativeWeightTiles::ReadFloatTile(source,index,tile,original.data(),original.size()))return 1;
			using Codec=PioneerBlackholeBlockFloat;
			if(!PioneerQ38NativeBfp8WeightTiles::ReadRange(source,index,tile*1088,1088,bytes.data(),bytes.size())
				|| !Codec::UnpackTile(Codec::Format::Bfp8B,bytes.data(),1088,decoded.data(),decoded.size()))return 1;
			for(unsigned i=0;i<1024;i++) {
				const double value=original[i],delta=value-decoded[i];metric.energy+=value*value;metric.bfp8Error+=delta*delta;
				float phase0=0;if(!PioneerQ38NativePhase0View(decoded[i],5,phase0))return 1;
				const double extra=value-phase0;metric.phase0OperandError+=extra*extra;
			}
			if(!PioneerQ38NativeWeightTiles::ReadRange(source,index,tile*Codec::Bytes(mixed.format),Codec::Bytes(mixed.format),bytes.data(),bytes.size())
				|| !Codec::UnpackTile(mixed.format,bytes.data(),Codec::Bytes(mixed.format),decoded.data(),decoded.size()))return 1;
			for(unsigned i=0;i<1024;i++){const double delta=double(original[i])-decoded[i];metric.mixedError+=delta*delta;}
			metric.tiles++;
		}
	}
	if(!source.Stable() || source.Epoch()!=epoch)return 1;
	uint64_t total=0;
	for(unsigned i=0;i<stats.size();i++) {
		const auto& metric=stats[i];if(!metric.tensors || !metric.energy)return 1;total+=metric.bfp8Bytes;
		printf("ORIGINAL_TYPE%u matrices%llu sampled_tiles%llu mixed_bytes%llu all_bfp8_bytes%llu mixed_sample_relative_L2%.9f all_bfp8_sample_relative_L2%.9f phase0_weight_operand_sample_relative_L2%.9f\n",
			i==0?8:i==1?12:14,(unsigned long long)metric.tensors,(unsigned long long)metric.tiles,
			(unsigned long long)metric.mixedBytes,(unsigned long long)metric.bfp8Bytes,
			std::sqrt(metric.mixedError/metric.energy),std::sqrt(metric.bfp8Error/metric.energy),std::sqrt(metric.phase0OperandError/metric.energy));
	}
	const uint64_t currentArena=UINT64_C(7)*3*1024*1024*1024;
	printf("ALL_BFP8_MATRIX_BYTES%llu EXCLUDED_CPU_EMBEDDING_BYTES%llu OTHER_WEIGHT_BYTES%llu CURRENT_SEVEN_BANK_ARENA_BYTES%llu\n",
		(unsigned long long)total,(unsigned long long)embeddingBytes,(unsigned long long)otherBytes,(unsigned long long)currentArena);
	printf("ALL_BFP8_NONEMBEDDING_MATRIX_BYTES%llu WEIGHTS_ONLY_CURRENT_ARENA_FIT%u;NO_NEW_BANK_RANGE_AUTHORITY\n",
		(unsigned long long)(total-embeddingBytes),unsigned(total-embeddingBytes+otherBytes<=currentArena));
	using Plan=PioneerQ38NativeBfp8Plan;Plan::Proposal proposal;
	if(Plan::Compute(source.Tensors(),128,true,UINT64_C(3)*1024*1024*1024,proposal)!=Plan::InsufficientCapacity)return 1;
	if(Plan::Compute(source.Tensors(),128,true,UINT64_C(15)*1024*1024*1024/4,proposal)!=Plan::Proposed)return 1;
	for(unsigned bank=0;bank<7;bank++)printf("HYPOTHETICAL_3_75GIB_BANK%u proposed_bytes%llu;DATA_ONLY_NOT_PHYSICAL_RANGE_OR_OWNER_AUTHORITY\n",bank,(unsigned long long)proposal.used[bank]);
	if(!source.Stable() || source.Epoch()!=epoch)return 1;
	puts("PASS_AUTHENTICATED_ORIGINAL_NATIVE_BFP8_READONLY_SAMPLE;NOT_FULL_MODEL_QUALITY_BANK_ALLOCATION_GPU_MATH_OR_SPEED_PROOF");return 0;
}
}
int main(int argc,char** argv) {
	try{return Run(argc,argv);}catch(const std::exception& error){fprintf(stderr,"Read-only native BFP8 sample refused: %s\n",error.what());return 1;}
}
