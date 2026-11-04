/* SPDX-License-Identifier: MIT */
#include "../PioneerNativeModelBandSource.h"
#include <cassert>
#include <cstdio>
#include <unistd.h>
int main(int argc,char** argv) {
	assert(argc==3);const std::string temporary=argv[2];std::string error;
	{PioneerNativeModelBandSource source;assert(!source.Ready() && !source.Prepare(nullptr,error) && !source.Ready() && !source.Prepare(argv[1],error));}
	{PioneerNativeModelBandSource source;assert(!source.Prepare((temporary+"/absent").c_str(),error) && !source.Ready());}
	{const std::string path=temporary+"/link";assert(!symlink(argv[1],path.c_str()));PioneerNativeModelBandSource source;assert(!source.Prepare(path.c_str(),error) && !source.Ready());}
	{const int original=std::fegetround();assert(!std::fesetround(FE_DOWNWARD));PioneerNativeModelBandSource source;assert(!source.Prepare(argv[1],error) && !source.Ready());assert(!std::fesetround(original));}
	{PioneerNativeModelBandSource source;puts("AUTHENTICATING_REAL_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_ONLY");fflush(stdout);
		if(!source.Prepare(argv[1],error)){fprintf(stderr,"Unexpected real source refusal: %s\n",error.c_str());return 1;}
		assert(source.Ready() && !source.Prepare(argv[1],error) && source.Ready());}
	puts("PASS real learned source:5 cases;full SAME-open original SHA,three fresh payload/reference matches,same-user-safe file metadata/symlink/rounding/one-shot checks;NO card/cache mutation/Work/model quality/speed proof");return 0;
}
