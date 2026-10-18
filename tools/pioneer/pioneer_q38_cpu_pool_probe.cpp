/* SPDX-License-Identifier: MIT */
// CPU API/lifecycle probe ONLY. No accelerator header/FD/model/reference.
#include "ggml.h"
#include "ggml-cpu.h"
#include "ggml-backend.h"
#include <stdio.h>
#include <vector>
int main()
{
	setvbuf(stdout,nullptr,_IOLBF,0);
	puts("CPU_POOL_PROBE_BEGIN;NO_ACCELERATOR_FD_OR_MODEL");
	auto params=ggml_threadpool_params_default(32); params.poll=0;
	auto* pool=ggml_threadpool_new(&params);
	auto backend=ggml_backend_cpu_init();
	ggml_init_params init={1024*1024,nullptr,true}; auto context=ggml_init(init);
	if (!pool || !backend || !context) return 1;
	ggml_backend_cpu_set_n_threads(backend,32); ggml_backend_cpu_set_threadpool(backend,pool);
	auto x=ggml_new_tensor_1d(context,GGML_TYPE_F32,65536); auto y=ggml_new_tensor_1d(context,GGML_TYPE_F32,65536);
	auto z=ggml_add(context,x,y); auto buffer=ggml_backend_alloc_ctx_tensors(context,backend);
	if (!buffer) { ggml_backend_free(backend); ggml_free(context); ggml_threadpool_free(pool); return 1; }
	auto graph=ggml_new_graph(context); ggml_build_forward_expand(graph,z);
	std::vector<float> input(65536,.125f),other(65536,.25f),output(65536);
	ggml_backend_tensor_set(y,other.data(),0,other.size()*4); bool okay=true;
	for (unsigned trial=0;okay && trial<64;trial++) {
		for (unsigned i=0;i<input.size();i++) input[i]=float(int((i+trial)%17)-8)*.125f;
		ggml_backend_tensor_set(x,input.data(),0,input.size()*4);
		okay=ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS;
		ggml_backend_tensor_get(z,output.data(),0,output.size()*4);
		for (unsigned i=0;i<output.size();i++) okay &= output[i]==input[i]+other[i];
	}
	ggml_backend_synchronize(backend); ggml_backend_cpu_set_threadpool(backend,nullptr);
	ggml_backend_buffer_free(buffer); ggml_backend_free(backend); ggml_free(context); ggml_threadpool_free(pool);
	printf("%s_CPU_POOL_PROBE persistent32_poll0_graphs64_exact_vector_add;NO_ACCELERATOR_FD_OR_MODEL_REFERENCE\n",okay?"PASS":"FAIL");
	return okay?0:1;
}
