/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
static FILE* fixtureFile;
static unsigned fault,stats;
static bool mutated;
static FILE* MockFopen(const char* path,const char* mode) {
	assert(!strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));
	return fdopen(dup(fileno(fixtureFile)),mode);
}
static int MockFstat(int fd,struct stat* out) {
	int result=fstat(fd,out); stats++;
	if (!result && (mutated || (fault==3 && stats==2))) out->st_ctime++;
	if (!result && fault==7) out->st_size--;
	return result;
}
static int MockExecv(const char* path,char* const* argv) {
	assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
	// Test-only trusted-hasher stand-in,NOT a real authenticity claim. The
	// production class has no injected digest/path callback or fake branch.
	if (fault==4) _exit(12);
	const char* digest=fault==1?"0000000000000000000000000000000000000000000000000000000000000000  -\n"
		:"c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747  -\n";
	assert(write(1,digest,fault==2?67:68)==(fault==2?67:68));
	if (fault==5) { const char extra[]="EXTRA HASHER OUTPUT"; assert(write(1,extra,sizeof(extra))==sizeof(extra)); }
	_exit(0);
}
#define fopen MockFopen
#define fstat MockFstat
#define execv MockExecv
#include "../PioneerQ38PublicModelSource.h"
#undef execv
#undef fstat
#undef fopen
int main()
{
	Fixture fixture; fixtureFile=fixture.file;
	for (unsigned failure:{1U,2U,3U,4U,5U,7U}) {
		fault=failure; stats=0; PioneerQ38PublicModelSource source;
		assert(!source.Ready() && !source.Open() && !source.Ready() && !source.Open());
	}
	fault=0; stats=0;
	{
		PioneerQ38PublicModelSource source; assert(source.Open() && source.Ready() && !source.Open());
		FILE* borrowed=source.BorrowForModelLoad(); assert(borrowed && !ftello(borrowed) && !source.BorrowForModelLoad() && source.Ready());
		PioneerQwen38PackedPlan plan; assert(plan.Build(source.Model(),8192,true));
		uint8_t page[4096]={}; assert(source.ReadWeightPage(plan,0,0,page,sizeof(page)));
		memset(page,0xa5,sizeof(page)); uint8_t before[4096]; memcpy(before,page,sizeof(page));
		mutated=true; assert(!source.Ready() && !source.ReadWeightPage(plan,0,0,page,sizeof(page)) && !memcmp(page,before,sizeof(page))); mutated=false;
		assert(source.Ready());
		// Test-only mutation of an otherwise const public model observation.
		assert(const_cast<PioneerQwen38Gguf&>(source.Model()).Open(fixture.file));
		assert(!source.Ready() && !source.ReadWeightPage(plan,0,0,page,sizeof(page)));
	}
	puts("PASS OFFLINE public-source owner:exact same opened FD/digest length/child status/stat+epoch guards,one-shot refusal,atomic page publication;HASHER IS MOCKED,NOT real authentication/FD accelerator/inference");
}
