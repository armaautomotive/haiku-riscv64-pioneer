/* SPDX-License-Identifier: MIT */
#include "PioneerModelWeightCache.h"
#include "PioneerQ38NativeWeightTiles.h"
#ifdef PIONEER_Q38_ALL_BFP8_CACHE
#include "PioneerQ38NativeBfp8WeightTiles.h"
#endif
#include <cstdio>
#include <cstring>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#ifndef PIONEER_Q38_NATIVE_CONVERSION_SHA256
#error Build with the source-fingerprinted native-weight-cache builder
#endif
namespace {
using Cache=PioneerModelWeightCache;
#ifdef PIONEER_Q38_ALL_BFP8_CACHE
using Tiles=PioneerQ38NativeBfp8WeightTiles;
#else
using Tiles=PioneerQ38NativeWeightTiles;
#endif
bool Same(const struct stat& a,const struct stat& b) {
	if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
	return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
	return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
int Run(int argc,char** argv) {
	if((argc!=4 && argc!=6) || strcmp(argv[1],"--build") || (argc==6 && strcmp(argv[4],"--tensor"))) {
		fprintf(stderr,"Usage: %s --build ORIGINAL_QWEN38_GGUF DEDICATED_CACHE_DIRECTORY [--tensor EXACT_NAME]\nCPU-only EXPERIMENTAL native-format cache;no driver/Assistant selection or speed claim.\n",argv[0]);return 2;
	}
	std::unique_ptr<FILE,decltype(&fclose)> file(fopen(argv[2],"rb"),&fclose);struct stat identity{};
	if(!file || fstat(fileno(file.get()),&identity) || !S_ISREG(identity.st_mode)
		|| uint64_t(identity.st_size)!=PioneerQwen38Gguf::FullBytes) {
		fputs("Original model is not the expected regular19GB GGUF.\n",stderr);return 1;
	}
	PioneerQwen38Gguf source;
	if(!source.Open(file.get())) { fputs("Complete original851-tensor GGUF layout refused.\n",stderr);return 1; }
	const uint64_t epoch=source.Epoch();auto ready=[&] {
		struct stat current{};return source.Ready() && source.Epoch()==epoch && source.Stable()
			&& !fstat(fileno(file.get()),&current) && Same(identity,current);
	};
	Cache::Digest modelSHA;std::string error;
	puts("AUTHENTICATING_SAME_OPEN_ORIGINAL_GGUF;NO_ACCELERATOR_ACCESS");fflush(stdout);
	if(!Cache::HashFile(fileno(file.get()),PioneerQwen38Gguf::FullBytes,modelSHA,error) || !ready()
		|| Cache::Hex(modelSHA)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747") {
		fprintf(stderr,"Original model SHA-256/stability refused: %s\n",error.c_str());return 1;
	}
	const std::string recipe=
#ifdef PIONEER_Q38_ALL_BFP8_CACHE
		"pioneer-native-weight-tiles-v2-all-bfp8/outputs32-K32/faces16-row-major/zero-padding/"
		"Q4_K,Q8_0,Q6_K-to-BFP8_B/"
#else
		"pioneer-native-weight-tiles-v1/outputs32-K32/faces16-row-major/zero-padding/"
		"Q4_K-to-BFP4_B/Q8_0,Q6_K-to-BFP8_B/"
#endif
		"shared-exp-FP32-bias/align-truncate-then-RNE-saturate/"
		"no-subnormal-input/source-fingerprint=" PIONEER_Q38_NATIVE_CONVERSION_SHA256;
	const auto conversion=Cache::Hash(recipe.data(),recipe.size());Cache cache;
	if(!cache.Open(argv[3],error)) { fprintf(stderr,"%s\n",error.c_str());return 1; }
	uint64_t entries=0,hits=0,convertedChunks=0,totalBytes=0;
	for(unsigned index=0;index<source.Tensors().size();index++) {
		const auto tensor=source.Tensors()[index];
		if(argc==6 && tensor.name!=argv[5])continue;
		Tiles::Layout layout;if(!Tiles::Describe(tensor,layout)) {
			if(argc==6) { fputs("Selected tensor does not use a supported quantized matrix format.\n",stderr);return 1; }
			continue;
		}
		Cache::Key key;key.sourceSHA256=modelSHA;key.conversionSHA256=conversion;key.entry=index;key.bytes=layout.bytes;
		uint64_t calls=0;auto produce=[&](uint64_t offset,unsigned count,uint8_t* output,std::string& why) {
			calls++;
			if(ready() && Tiles::ReadRange(source,index,offset,count,output,count) && ready())return true;
			why="Original GGUF to native-weight tile conversion refused.";return false;
		};
		Cache::Reader reader;bool hit=false;
		if(!cache.GetOrCreate(key,ready,produce,reader,hit,error)) { fprintf(stderr,"%s: %s\n",tensor.name.c_str(),error.c_str());return 1; }
		if(!ready() || !reader.IsOpen() || reader.Size()!=layout.bytes || (hit && calls))return 1;
		printf("%s tensor%u %s native_bfp%u_b bytes%llu conversion_chunks%llu\n",hit?"CACHE_HIT_VERIFIED_NO_CONVERSION":"CACHE_CREATED_ATOMICALLY",index,tensor.name.c_str(),unsigned(layout.format),
			(unsigned long long)layout.bytes,(unsigned long long)calls);fflush(stdout);
		entries++;hits+=hit;convertedChunks+=calls;totalBytes+=layout.bytes;
	}
	if(!entries || !ready()) { fputs("No selected converted weights,or original source changed.\n",stderr);return 1; }
	printf("PASS_NATIVE_WEIGHT_DISK_CACHE entries%llu hits%llu converted_chunks%llu bytes%llu recipe_sha256%s;EXPERIMENTAL_CONVERTED_WEIGHTS_ONLY_NOT_MODEL_QUALITY_DRIVER_SELECTION_OR_SPEED\n",
		(unsigned long long)entries,(unsigned long long)hits,(unsigned long long)convertedChunks,(unsigned long long)totalBytes,Cache::Hex(conversion).c_str());return 0;
}
}
int main(int argc,char** argv) {
	try { return Run(argc,argv); }
	catch(const std::exception& error) { fprintf(stderr,"Weight cache aborted safely: %s\n",error.what());return 1; }
}
