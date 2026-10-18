/* SPDX-License-Identifier: MIT */
// CPU API/lifecycle probe ONLY. No accelerator header/FD/model/reference.
#include "ggml.h"
#include "ggml-cpu.h"
#include "ggml-backend.h"
#include <stdio.h>
#include <time.h>
#include <vector>
static double Seconds()
{
	timespec now={}; return clock_gettime(CLOCK_MONOTONIC,&now)==0?double(now.tv_sec)+double(now.tv_nsec)*1e-9:-1;
}
static bool Trial(bool persistent,double& elapsed)
{
	auto params=ggml_threadpool_params_default(32); params.poll=0;
	auto* pool=persistent?ggml_threadpool_new(&params):nullptr;
	auto backend=ggml_backend_cpu_init();
	ggml_init_params init={1024*1024,nullptr,true}; auto context=ggml_init(init);
	if ((persistent && !pool) || !backend || !context) return false;
	ggml_backend_cpu_set_n_threads(backend,32); ggml_backend_cpu_set_threadpool(backend,pool);
	auto x=ggml_new_tensor_1d(context,GGML_TYPE_F32,65536); auto y=ggml_new_tensor_1d(context,GGML_TYPE_F32,65536);
	auto z=ggml_add(context,x,y); auto buffer=ggml_backend_alloc_ctx_tensors(context,backend);
	if (!buffer) { ggml_backend_free(backend); ggml_free(context); if (pool) ggml_threadpool_free(pool); return false; }
	auto graph=ggml_new_graph(context); ggml_build_forward_expand(graph,z);
	std::vector<float> input(65536,.125f),other(65536,.25f),output(65536);
	ggml_backend_tensor_set(y,other.data(),0,other.size()*4); bool okay=true;
	double began=Seconds();
	for (unsigned trial=0;okay && trial<64;trial++) {
		for (unsigned i=0;i<input.size();i++) input[i]=float(int((i+trial)%17)-8)*.125f;
		ggml_backend_tensor_set(x,input.data(),0,input.size()*4);
		okay=ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS;
		ggml_backend_tensor_get(z,output.data(),0,output.size()*4);
		for (unsigned i=0;i<output.size();i++) okay &= output[i]==input[i]+other[i];
	}
	elapsed=Seconds()-began; okay &= began>=0 && elapsed>=0;
	ggml_backend_synchronize(backend); ggml_backend_cpu_set_threadpool(backend,nullptr);
	ggml_backend_buffer_free(buffer); ggml_backend_free(backend); ggml_free(context); if (pool) ggml_threadpool_free(pool);
	return okay;
}
int main()
{
	setvbuf(stdout,nullptr,_IOLBF,0);
	puts("CPU_POOL_PROBE_BEGIN;NO_ACCELERATOR_FD_OR_MODEL");
	double disposable=0,persistent=0;
	bool okay=Trial(false,disposable) && Trial(true,persistent);
	printf("%s_CPU_POOL_PROBE graphs64_exact_vector_add threads32 disposable_seconds%.6f persistent_poll0_seconds%.6f;CPU_ONLY_MICROBENCH_NOT_MODEL_TIMING_OR_GPU_SPEED\n",okay?"PASS":"FAIL",disposable,persistent);
	return okay?0:1;
}
