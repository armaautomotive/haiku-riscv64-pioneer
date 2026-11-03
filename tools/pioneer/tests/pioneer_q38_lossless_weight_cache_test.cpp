/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY 1
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38LosslessWeightCache.h"
#include <fcntl.h>
using Cache=PioneerModelWeightCache;
using Lossless=PioneerQ38LosslessWeightCache;
static unsigned cases=0;
static void UnchangedFailure(Lossless& cache,const PioneerQwen38PackedPlan& plan,unsigned part,unsigned page) {
	std::array<uint8_t,4096> out;out.fill(0xa9);const auto before=out;std::string error;
	assert(!cache.ReadWeightPage(plan,part,page,out.data(),out.size(),error) && out==before && !error.empty());cases++;
}
int main(int argc,char** argv) {
	assert(argc==2);const std::string directory=std::string(argv[1])+"/lossless";
	Fixture fixture;std::vector<unsigned> selected;
	for(unsigned type:{0U,8U,12U,14U}) {
		unsigned index=0;while(fixture.expected.at(index).type!=type)index++;
		selected.push_back(index);const auto& t=fixture.expected[index];
		const unsigned rowBytes=unsigned(t.bytes/t.outputs),rows=unsigned(std::min(UINT64_C(64),t.outputs));
		std::vector<uint8_t> raw(size_t(rows)*rowBytes);
		for(unsigned half=0;half<2;half++) {
			for(size_t i=0;i<raw.size();i++)raw[i]=uint8_t((i*71)^(i>>7)^(index*29)^(half*0x93));
			fixture.Write(t.offset+(half?t.outputs-rows:0)*rowBytes,raw);
		}
	}
	PioneerQwen38Gguf source;assert(source.Open(fixture.file));PioneerQwen38PackedPlan plan,untiled;
	assert(plan.Build(source,128,true,true) && untiled.Build(source,128,false,true));
	const auto original=Cache::Hash("FIXTURE-not-authenticated-public-model",38);
	const std::string recipe=Lossless::Recipe("test-source-fingerprint");const auto conversion=Cache::Hash(recipe.data(),recipe.size());
	bool ready=true;auto sourceReady=[&]{return ready && source.Stable();};std::string error;
	Lossless first;assert(first.Bind(directory.c_str(),source,original,conversion,sourceReady,error));
	assert(!first.Bind(directory.c_str(),source,original,conversion,sourceReady,error));cases++;
	Lossless::Statistics stats;
	for(unsigned index:selected) {
		assert(first.PrepareTensor(index,stats,error));
		assert(!first.PrepareTensor(index,stats,error));cases++;
	}
	assert(stats.entries==4 && !stats.hits && stats.convertedPages>10000 && !first.AllPrepared());
	Lossless reopened;assert(reopened.Bind(directory.c_str(),source,original,conversion,sourceReady,error));
	Lossless::Statistics warm;
	for(unsigned index:selected)assert(reopened.PrepareTensor(index,warm,error));
	assert(warm.entries==4 && warm.hits==4 && !warm.convertedPages && warm.bytes==stats.bytes);cases++;
	unsigned tested=0,partial=0;
	for(unsigned part=0;part<plan.Parts().size();part++) {
		const auto& p=plan.Parts()[part];
		if(p.kind!=PIONEER_Q38_WEIGHT || std::find(selected.begin(),selected.end(),p.tensor)==selected.end())continue;
		for(unsigned page=0;page<p.allocated/4096;page++) {
			std::array<uint8_t,4096> expected{},actual;
			assert(plan.ReadWeightPage(source,part,page,expected.data(),expected.size()));
			actual.fill(0x8b);assert(reopened.ReadWeightPage(plan,part,page,actual.data(),actual.size(),error));
			assert(actual==expected);tested++;
			if(page+1==p.allocated/4096 && p.bytes%4096) {
				for(unsigned i=p.bytes%4096;i<4096;i++)assert(!actual[i]);partial++;
			}
		}
		UnchangedFailure(reopened,plan,part,p.allocated/4096);cases++;
	}
	assert(tested==stats.convertedPages+partial /* separately rounded fragment tails */ || tested>stats.convertedPages);
	assert(partial>0);cases++;
	UnchangedFailure(reopened,untiled,0,0);UnchangedFailure(reopened,plan,UINT32_MAX,0);
	unsigned absent=0;while(plan.Parts().at(absent).kind!=PIONEER_Q38_WEIGHT
		|| std::find(selected.begin(),selected.end(),plan.Parts()[absent].tensor)!=selected.end())absent++;
	UnchangedFailure(reopened,plan,absent,0);
	ready=false;UnchangedFailure(reopened,plan,0,0);ready=true;
	// Mutation also revokes an already buffered64KiB snapshot. The upload
	// phase reports failure without repairing/replacing that cache file.
	unsigned damaged=0;while(plan.Parts()[damaged].kind!=PIONEER_Q38_WEIGHT || plan.Parts()[damaged].tensor!=selected[0])damaged++;
	std::array<uint8_t,4096> accepted{};
	assert(reopened.ReadWeightPage(plan,damaged,0,accepted.data(),accepted.size(),error));
	Cache::Key damagedKey;assert(reopened.KeyFor(selected[0],damagedKey));
	const std::string damagedPath=directory+"/"+Cache::EntryName(damagedKey);
	struct stat before{},after{};assert(!stat(damagedPath.c_str(),&before) && !chmod(damagedPath.c_str(),0600));
	int corrupt=open(damagedPath.c_str(),O_RDWR|O_NOFOLLOW);assert(corrupt>=0);uint8_t byte=0x57;
	assert(pwrite(corrupt,&byte,1,before.st_size-1)==1 && !fsync(corrupt) && !close(corrupt) && !chmod(damagedPath.c_str(),0400));
	UnchangedFailure(reopened,plan,damaged,0);assert(!stat(damagedPath.c_str(),&after) && before.st_ino==after.st_ino);cases++;
	// A read-only upload never repairs or recreates a missing prepared entry.
	Cache::Key key;assert(reopened.KeyFor(selected[1],key));const std::string path=directory+"/"+Cache::EntryName(key);
	assert(!unlink(path.c_str()));
	unsigned victim=0;while(plan.Parts()[victim].tensor!=selected[1] || plan.Parts()[victim].kind!=PIONEER_Q38_WEIGHT)victim++;
	UnchangedFailure(reopened,plan,victim,0);assert(access(path.c_str(),F_OK) && errno==ENOENT);cases++;
	// Prepared data is also revoked by source epoch changes, not merely size.
	assert(source.Open(fixture.file));PioneerQwen38PackedPlan fresh;assert(fresh.Build(source,128,true,true));
	UnchangedFailure(reopened,fresh,0,0);assert(!reopened.AllPrepared());cases++;
	printf("PASS lossless weight-cache adapter:%u checks,%u exact upload pages,%u fragment tails;4 formats;fresh/hit ZERO warm conversion;no GPU/no requantization/no speed claim\n",cases,tested,partial);
}
