/* SPDX-License-Identifier: MIT */
#include "PioneerQ38LosslessWeightCache.h"
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#ifndef PIONEER_Q38_LOSSLESS_CONVERSION_SHA256
#error Use the source-fingerprinted lossless cache builder
#endif
namespace {
using Cache=PioneerModelWeightCache;
int Run(int argc,char** argv) {
	bool check=argc>1 && !strcmp(argv[argc-1],"--check-upload-pages");if(check)argc--;
	if((argc!=4 && argc!=6) || strcmp(argv[1],"--build") || (argc==6 && strcmp(argv[4],"--tensor"))) {
		fprintf(stderr,"Usage: %s --build ORIGINAL_QWEN38_GGUF DEDICATED_CACHE_DIR [--tensor EXACT_NAME] [--check-upload-pages]\nCPU-only LOSSLESS weights; no accelerator or inference.\n",argv[0]);return 2;
	}
	std::unique_ptr<FILE,decltype(&fclose)> file(fopen(argv[2],"rb"),&fclose);PioneerQwen38Gguf source;
	if(!file || !source.Open(file.get())) { fputs("Exact original851-tensor GGUF layout refused.\n",stderr);return 1; }
	const uint64_t epoch=source.Epoch();struct stat identity{};if(fstat(fileno(file.get()),&identity))return 1;
	auto ready=[&] {
		struct stat now{};if(!source.Stable() || source.Epoch()!=epoch || fstat(fileno(file.get()),&now))return false;
		if(now.st_dev!=identity.st_dev || now.st_ino!=identity.st_ino || now.st_size!=identity.st_size
			|| now.st_mtime!=identity.st_mtime || now.st_ctime!=identity.st_ctime)return false;
#if defined(__APPLE__)
		return now.st_mtimespec.tv_nsec==identity.st_mtimespec.tv_nsec && now.st_ctimespec.tv_nsec==identity.st_ctimespec.tv_nsec;
#else
		return now.st_mtim.tv_nsec==identity.st_mtim.tv_nsec && now.st_ctim.tv_nsec==identity.st_ctim.tv_nsec;
#endif
	};
	puts("AUTHENTICATING_SAME_OPEN_ORIGINAL_GGUF;CPU_ONLY_LOSSLESS_DISK_CACHE");fflush(stdout);
	Cache::Digest original;std::string error;
	uint64_t authenticated=0;
	auto authenticationProgress=[&](uint64_t done,uint64_t total,bool) {
		if(done-authenticated<64*1024*1024 && done!=total)return;
		authenticated=done;printf("AUTHENTICATING %llu/%llu\n",(unsigned long long)done,(unsigned long long)total);fflush(stdout);
	};
	if(!Cache::HashFile(fileno(file.get()),PioneerQwen38Gguf::FullBytes,original,error,authenticationProgress) || !ready()
		|| Cache::Hex(original)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747") {
		fprintf(stderr,"Full original authentication/stability refused: %s\n",error.c_str());return 1;
	}
	const auto recipe=PioneerQ38LosslessWeightCache::Recipe(PIONEER_Q38_LOSSLESS_CONVERSION_SHA256);
	const auto conversion=Cache::Hash(recipe.data(),recipe.size());PioneerQ38LosslessWeightCache cache;
	if(!cache.Bind(argv[3],source,original,conversion,ready,error)) { fprintf(stderr,"%s\n",error.c_str());return 1; }
	PioneerQ38LosslessWeightCache::Statistics stats;std::vector<unsigned> selected;
	for(unsigned i=0;i<source.Tensors().size();i++) {
		if(argc==6 && source.Tensors()[i].name!=argv[5])continue;
		const auto before=stats;uint64_t last=0;
		auto progress=[&](uint64_t done,uint64_t total,bool verifying) {
			if(done-last<64*1024*1024 && done!=total)return;
			last=done;printf("%s tensor%u %llu/%llu\n",verifying?"VERIFYING":"PREPARING",i,(unsigned long long)done,(unsigned long long)total);fflush(stdout);
		};
		if(!cache.PrepareTensor(i,stats,error,progress)) { fprintf(stderr,"%s: %s\n",source.Tensors()[i].name.c_str(),error.c_str());return 1; }
		printf("%s tensor%u %s bytes%llu conversion_pages%llu\n",stats.hits>before.hits?"CACHE_HIT_VERIFIED_NO_CONVERSION":"CACHE_CREATED_ATOMICALLY",i,
			source.Tensors()[i].name.c_str(),(unsigned long long)(stats.bytes-before.bytes),(unsigned long long)(stats.convertedPages-before.convertedPages));fflush(stdout);
		selected.push_back(i);
	}
	if(selected.empty() || !ready() || (argc==4 && !cache.AllPrepared()))return 1;
	unsigned checked=0;
	if(check) {
		PioneerQwen38PackedPlan plan;if(!plan.Build(source,128,true,true))return 1;
		for(unsigned part=0;part<plan.Parts().size();part++) {
			const auto p=plan.Parts()[part];if(p.kind!=PIONEER_Q38_WEIGHT || std::find(selected.begin(),selected.end(),p.tensor)==selected.end())continue;
			for(unsigned page:{0U,p.allocated/8192,p.allocated/4096-1}) {
				std::array<uint8_t,4096> actual{},expected{};
				if(!cache.ReadWeightPage(plan,part,page,actual.data(),actual.size(),error)
					|| !plan.ReadWeightPage(source,part,page,expected.data(),expected.size()) || actual!=expected || !ready()) {
					fprintf(stderr,"Exact cached upload page refused: %s\n",error.c_str());return 1;
				}
				checked++;
			}
		}
	}
	printf("PASS_LOSSLESS_WEIGHT_DISK_CACHE entries%llu hits%llu conversion_pages%llu bytes%llu checked_upload_pages%u recipe_sha256%s;NO_GPU_NO_REQUANTIZATION_NO_SPEED_CLAIM\n",
		(unsigned long long)stats.entries,(unsigned long long)stats.hits,(unsigned long long)stats.convertedPages,(unsigned long long)stats.bytes,checked,Cache::Hex(conversion).c_str());return 0;
}
}
int main(int argc,char** argv) {
	try { return Run(argc,argv); }
	catch(const std::exception& error) { fprintf(stderr,"Lossless weight cache stopped: %s\n",error.what());return 1; }
}
