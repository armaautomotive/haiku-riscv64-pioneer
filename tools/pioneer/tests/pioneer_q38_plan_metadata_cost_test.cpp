/* SPDX-License-Identifier: MIT */
// OFFLINE ordinary-file diagnostic,NO device/IOCTL/card authority.
#include <sys/stat.h>
#include <stdint.h>
static uint64_t metadataCalls=0;
static int CountMetadata(int fd,struct stat* result) { metadataCalls++; return ::fstat(fd,result); }
#define fstat CountMetadata
#ifndef PIONEER_Q38_PLAN_DIAGNOSTIC_HEADER
#define PIONEER_Q38_PLAN_DIAGNOSTIC_HEADER "../PioneerQwen38FleetPlan.h"
#endif
#include PIONEER_Q38_PLAN_DIAGNOSTIC_HEADER
#undef fstat
#include <chrono>
#include <cassert>
int main(int argc,char** argv)
{
	assert(argc==2); FILE* f=fopen(argv[1],"rb"); assert(f);
	PioneerQwen38Gguf model; assert(model.Open(f));
	PioneerQwen38PackedPlan layout; assert(layout.Build(model,8192,true));
	metadataCalls=0; unsigned matrices=0,jobs=0; uint64_t fingerprint=UINT64_C(14695981039346656037);
	auto start=std::chrono::steady_clock::now();
	for (unsigned t=0;t<model.Tensors().size();t++) {
		const auto& tensor=model.Tensors()[t];
		if (!tensor.type || tensor.name=="token_embd.weight") continue;
		PioneerQwen38FleetPlan plan; assert(plan.Build(model,layout,t,119));
		matrices++; jobs+=unsigned(plan.Jobs().size());
		for (const auto& job:plan.Jobs()) for (uint64_t v:{uint64_t(t),uint64_t(job.worker),uint64_t(job.wave),uint64_t(job.fragment),
			uint64_t(job.bank),uint64_t(job.firstRow),uint64_t(job.groups),uint64_t(job.bytes),job.proposedOffset}) {
			for (unsigned byte=0;byte<8;byte++) { fingerprint^=(v>>(byte*8))&255; fingerprint*=UINT64_C(1099511628211); }
		}
	}
	double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
	assert(matrices==497);
	assert(jobs==47101 && fingerprint==UINT64_C(0x94ab20f5488d9f1a));
#ifdef PIONEER_Q38_PLAN_PREOPTIMIZATION
	assert(metadataCalls==4322409); // explicit pinned old-header diagnostic control ONLY
#else
	assert(metadataCalls<10000); // original no-IO loop made4,322,409 syscalls
#endif
	printf("OFFLINE scheduling %u matrices/%u jobs fingerprint%016llx metadata_syscalls%llu seconds%.6f;NOT native model/card speed\n",matrices,jobs,(unsigned long long)fingerprint,(unsigned long long)metadataCalls,seconds);
	assert(fclose(f)==0);
}
