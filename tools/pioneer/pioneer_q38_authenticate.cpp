/* SPDX-License-Identifier: MIT */
#include "PioneerQ38PublicModelSource.h"
#include <string.h>
int main(int argc,char** argv)
{
	if (argc!=2 || strcmp(argv[1],"--authenticate")) return 2;
	setvbuf(stdout,nullptr,_IOLBF,0);
	puts("PUBLIC27B_FULL_SAME_FD_SHA_BEGIN;NO_ACCELERATOR_ACCESS");
	PioneerQ38PublicModelSource source;
	if (!source.Open()) { puts("STOP_FULL_SAME_FD_SOURCE_AUTH;NO_ACCELERATOR_ACCESS"); return 1; }
	PioneerQwen38PackedPlan plan;
	if (!plan.Build(source.Model(),8192,true) || !source.Ready()) return 1;
	printf("PASS_PUBLIC27B_FULL_SAME_FD_SHA bytes=%llu tensors=%zu tiled_parts=%zu;NO_ACCELERATOR_ACCESS_OR_INFERENCE\n",
		(unsigned long long)PioneerQwen38Gguf::FullBytes,source.Model().Tensors().size(),plan.Parts().size());
	return 0;
}
