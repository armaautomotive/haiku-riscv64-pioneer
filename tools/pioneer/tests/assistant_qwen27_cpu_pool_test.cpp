/* SPDX-License-Identifier: MIT */
// Frozen actual ggml CPU backend/pools, synthetic matrices, NO accelerator
// header/register/open/ioctl or model. Separate lifecycle test, not Work
// mode CPU validation or a performance benchmark.
#include "../../../src/apps/assistant/Qwen27Pool.h"
#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"
#include "ggml-cpu.h"
#include <assert.h>
#include <stdio.h>
#include <vector>
using Pools=Assistant27::PoolOwner<ggml_threadpool_t,ggml_threadpool_free>;
static ggml_threadpool_t Create(unsigned threads) {
 auto p=ggml_threadpool_params_default(threads); p.poll=0;
 auto pool=ggml_threadpool_new(&p); assert(pool); return pool;
}
int main() {
 ggml_init_params params={2*1024*1024,nullptr,true}; auto* context=ggml_init(params); assert(context);
 auto* weights=ggml_new_tensor_2d(context,GGML_TYPE_F32,128,128);
 auto* input=ggml_new_tensor_2d(context,GGML_TYPE_F32,128,1);
 auto* output=ggml_mul_mat(context,weights,input);
 auto* graph=ggml_new_graph_custom(context,32,false); ggml_build_forward_expand(graph,output);
 auto backend=ggml_backend_cpu_init(); assert(backend && ggml_backend_is_cpu(backend));
 auto buffer=ggml_backend_alloc_ctx_tensors(context,backend); assert(buffer);
 std::vector<float> w(128*128,.5f),x(128,.5f),y(128,-1);
 ggml_backend_tensor_set(weights,w.data(),0,w.size()*4); ggml_backend_tensor_set(input,x.data(),0,x.size()*4);
 Pools pools; assert(pools.Replace(Create(2)));
 for(unsigned iteration=0;iteration<16;iteration++) {
  if(iteration) {
   // Intermediate pools never bound to the CPU backend can be freed;
   // its last actually bound pool remains alive until the next compute.
   for(unsigned n:{1U,4U,2U}) assert(pools.Replace(Create(n)));
   assert(pools.NeedsRetirement());
  }
  ggml_backend_cpu_set_threadpool(backend,pools.Current());
  ggml_backend_cpu_set_n_threads(backend,2);
  assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS);
  ggml_backend_synchronize(backend); pools.AfterSuccessfulDecode(); assert(!pools.NeedsRetirement());
  ggml_backend_tensor_get(output,y.data(),0,y.size()*4);
  for(float value:y) assert(value==32);
 }
 // Last idle swap followed by context destruction, with no graph to bind
 // the new pool: both possible backend/context references must stay live.
 assert(pools.Replace(Create(3)) && pools.NeedsRetirement());
 ggml_backend_buffer_free(buffer); ggml_backend_free(backend); pools.AfterContextFree(); ggml_free(context);
 puts("PASS actual frozen CPU backend:16 graph computes,45 intermediate idle pool changes,old bound pool retained until rebind/sync,all128 outputs exact,final unbound-pool/context teardown;NO accelerator access");
}
