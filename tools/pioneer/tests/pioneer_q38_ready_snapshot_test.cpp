/* SPDX-License-Identifier: MIT */
#include <sys/stat.h>
#include <cassert>
#include <cstdio>
#define B_DEVICE_OP_CODES_END 1000
static unsigned statCalls=0,failStat=0;
static int SnapshotFstat(int fd,struct stat* value) {
	if (++statCalls==failStat) return -1;
	return ::fstat(fd,value);
}
#define fstat SnapshotFstat
#include "../PioneerQwen38Gguf.h"
#ifndef PIONEER_Q38_READY_SNAPSHOT_HEADER
#define PIONEER_Q38_READY_SNAPSHOT_HEADER "../PioneerQwen38CohortHost.h"
#endif
#include PIONEER_Q38_READY_SNAPSHOT_HEADER
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38WorkSession.h"
#undef fstat
int main()
{
	Fixture fixture; PioneerQwen38Gguf model; assert(model.Open(fixture.file));
	PioneerQwen38PackedPlan layout; assert(layout.Build(model,8192,true));
	const auto* tensor=model.Find("blk.0.ffn_down.weight"); assert(tensor);
	const unsigned selected=unsigned(tensor-model.Tensors().data());
	PioneerQwen38FleetPlan plan; statCalls=0; assert(plan.Build(model,layout,selected,119));
	const unsigned buildStats=statCalls;
	PioneerQ38CohortWave wave; statCalls=0; assert(wave.Build(model,layout,plan,0));
	const unsigned waveStats=statCalls;
	statCalls=0; assert(wave.Ready()); const unsigned readyStats=statCalls;
	std::vector<float> x(17408,.125f),output(5120,-1),before=output;
	PioneerQ38CohortInput input; assert(PioneerQ38CohortQuantize(12,17408,x.data(),input));
	statCalls=0; auto request=PioneerQ38DispatchRequest(wave,input); assert(request);
	const unsigned requestStats=statCalls;
	auto& r=request->result; r.version=1; r.state=TT_RUNTIME_PASSED; r.nonce=1; r.peers=wave.workers;
	r.root_started=r.root_completed=r.root_held=1; r.root_marker=0x314d3851; r.root_count=wave.workers;
	for (unsigned w=0;w<wave.workers;w++) for (unsigned row=0;row<wave.part[w].groups*16;row++)
		request->output[w][row]=PioneerAttentionBits(float(wave.firstRow[w]+row));
	PioneerQ38WorkSessionMode mode; statCalls=0;
	assert(mode.Collect(wave,input,*request,output.data(),unsigned(output.size())));
	const unsigned collectStats=statCalls;
	for (unsigned row=0;row<output.size();row++) assert(output[row]==float(row));
	printf("OFFLINE_HOST_METADATA_FSTATS fleet_build%u wave_build%u wave_ready%u request%u collect%u;NO_DEVICE_OR_TOKEN_RATE\n",
		buildStats,waveStats,readyStats,requestStats,collectStats);
#ifndef PIONEER_Q38_READY_SNAPSHOT_LEGACY
	assert(buildStats==2 && waveStats==2 && readyStats==2 && requestStats==4 && collectStats==4);
#endif
	// EVERY fresh identity check, including the AFTER-traversal check,
	// must fail closed. No lifetime file cache or learned-data substitution.
	for (unsigned at=1;at<=readyStats;at++) {
		statCalls=0; failStat=at; assert(!wave.Ready()); failStat=0;
	}
	for (unsigned at=1;at<=requestStats;at++) {
		statCalls=0; failStat=at; assert(!PioneerQ38DispatchRequest(wave,input)); failStat=0;
	}
	for (unsigned at=1;at<=collectStats;at++) {
		PioneerQ38WorkSessionMode fresh; output=before; statCalls=0; failStat=at;
		assert(!fresh.Collect(wave,input,*request,output.data(),unsigned(output.size())) && output==before); failStat=0;
	}
	assert(model.Open(fixture.file) && !wave.Ready() && !plan.Ready());
	puts("PASS private host metadata snapshots:fresh file identity before/after traversal,all syscall failures refuse before publication,all output rows exact,source epoch invalidates;NO hardware capability/inference/speed claim");
}
