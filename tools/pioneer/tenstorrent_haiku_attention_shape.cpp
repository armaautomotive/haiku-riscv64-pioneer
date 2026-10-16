/* SPDX-License-Identifier: MIT */
// CPU-only graph inspection. No Pioneer backend, device headers or device IO.
#include <llama.h>
#include <ggml-backend.h>
#include "PioneerAttentionFixture.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <vector>

static double Now()
{
	struct timespec value = {};
	clock_gettime(CLOCK_MONOTONIC, &value);
	return value.tv_sec + value.tv_nsec * 1e-9;
}

struct AttentionInspection {
	int position = 0;
	unsigned calls = 0;
	double started = 0, seconds = 0;
	std::string capture;
	static bool Write(int fd, const void* data, size_t bytes)
	{
		auto* cursor = static_cast<const char*>(data);
		while (bytes) {
			ssize_t written = write(fd, cursor, bytes);
			if (written < 0 && errno == EINTR) continue;
			if (written <= 0) return false;
			cursor += written; bytes -= size_t(written);
		}
		return true;
	}
	bool Capture(ggml_tensor* tensor)
	{
		if (capture.empty()) return true;
		auto* q = tensor->src[0]; auto* k = tensor->src[1]; auto* v = tensor->src[2]; auto* mask = tensor->src[3];
		const int64_t qShape[] = {128,1,16,1}, kvShape[] = {128,256,8,1}, outShape[] = {128,16,1,1};
		const size_t qStride[] = {4,8192,512,8192}, kvStride[] = {2,2048,256,524288};
		float params[3]; memcpy(params, tensor->op_params, sizeof(params));
		if (!q || !k || !v || !mask || tensor->src[4] || calls >= 28 || position < 0 || position >= 256
			|| q->type != GGML_TYPE_F32 || k->type != GGML_TYPE_F16 || v->type != GGML_TYPE_F16
			|| mask->type != GGML_TYPE_F16 || tensor->type != GGML_TYPE_F32
			|| memcmp(q->ne,qShape,sizeof(qShape)) || memcmp(k->ne,kvShape,sizeof(kvShape)) || memcmp(v->ne,kvShape,sizeof(kvShape))
			|| memcmp(tensor->ne,outShape,sizeof(outShape)) || memcmp(q->nb,qStride,sizeof(qStride))
			|| memcmp(k->nb,kvStride,sizeof(kvStride)) || memcmp(v->nb,kvStride,sizeof(kvStride))
			|| !ggml_is_contiguous(tensor) || !ggml_is_contiguous(mask) || ggml_nelements(mask) != 256
			|| params[0] != 0.0883883461356163f || params[1] != 0 || params[2] != 0) return false;
		char qName[32], kName[32], vName[32];
		snprintf(qName,sizeof(qName),"Qcur-%u",calls);
		snprintf(kName,sizeof(kName),"cache_k_l%u",calls);
		snprintf(vName,sizeof(vName),"cache_v_l%u",calls);
		if (!q->view_src || !k->view_src || !v->view_src || strcmp(q->view_src->name,qName)
			|| strcmp(k->view_src->name,kName) || strcmp(v->view_src->name,vName)) return false;
		uint16_t causal[256]; ggml_backend_tensor_get(mask, causal, 0, sizeof(causal));
		for (int i = 0; i < 256; i++) if (causal[i] != (i <= position ? 0 : 0xfc00)) return false;
		PioneerAttentionFixture header = {0x4e545450,1,calls,uint32_t(position+1),128,16,8,
			2048,uint32_t((position+1)*1024),2048,PioneerAttentionBits(params[0]),0};
		if (!PioneerAttentionFixtureValid(header)) return false;
		std::vector<float> query(2048), result(2048);
		std::vector<uint16_t> keys(header.kvCount), values(header.kvCount);
		ggml_backend_tensor_get(q, query.data(), 0, query.size()*4);
		ggml_backend_tensor_get(k, keys.data(), 0, keys.size()*2);
		ggml_backend_tensor_get(v, values.data(), 0, values.size()*2);
		ggml_backend_tensor_get(tensor, result.data(), 0, result.size()*4);
		char leaf[64]; snprintf(leaf,sizeof(leaf),"/attention-p%02d-l%02u.bin",position,calls);
		std::string path = capture + leaf;
		int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
		if (fd < 0) return false;
		bool okay = Write(fd,&header,sizeof(header)) && Write(fd,query.data(),query.size()*4)
			&& Write(fd,keys.data(),keys.size()*2) && Write(fd,values.data(),values.size()*2)
			&& Write(fd,result.data(),result.size()*4) && fsync(fd) == 0;
		if (close(fd) != 0) okay = false;
		if (okay) printf("PUBLIC_FIXTURE position=%d layer=%u tokens=%u bytes=%zu\n",position,calls,header.tokens,
			sizeof(header)+(query.size()+result.size())*4+(keys.size()+values.size())*2);
		return okay;
	}
	static bool Eval(ggml_tensor* tensor, bool ask, void* opaque)
	{
		// Force a boundary at every node: otherwise the observed attention
		// segment also includes the preceding Q/K/V projections and norms.
		if (tensor->op != GGML_OP_FLASH_ATTN_EXT) return true;
		auto& self = *static_cast<AttentionInspection*>(opaque);
		if (ask) {
			if (self.position == 0) {
				printf("attention layer-call=%u name=%s\n", self.calls, tensor->name);
				if (self.calls == 0) {
					for (int i = -1; i < 5; i++) {
						const ggml_tensor* item = i < 0 ? tensor : tensor->src[i];
						if (!item) { printf("  source=%d absent\n", i); continue; }
						printf("  source=%d name=%s type=%s ne=%lld,%lld,%lld,%lld nb=%zu,%zu,%zu,%zu\n",
							i, item->name, ggml_type_name(item->type),
							(long long)item->ne[0], (long long)item->ne[1], (long long)item->ne[2], (long long)item->ne[3],
							item->nb[0], item->nb[1], item->nb[2], item->nb[3]);
						for (auto* view = item->view_src; view; view = view->view_src)
							printf("    view-source=%s\n", view->name);
					}
					float params[3]; memcpy(params, tensor->op_params, sizeof(params));
					printf("  scale=%.9g max-bias=%.9g logit-softcap=%.9g precision=%d\n",
						params[0], params[1], params[2], (int)ggml_flash_attn_ext_get_prec(tensor));
				}
			}
			self.started = Now(); return true;
		}
		self.seconds += Now() - self.started;
		if (!self.Capture(tensor)) { fprintf(stderr,"Refused unsupported/incomplete/existing public fixture.\n"); return false; }
		self.calls++;
		return true;
	}
};

int main(int argc, char** argv)
{
	if (argc != 2 && (argc != 4 || strcmp(argv[2],"--capture"))) {
		fprintf(stderr, "Usage: attention_shape PINNED_QWEN3_0.6B_Q8_0 [--capture FRESH_ABSOLUTE_DIRECTORY]\n"); return 2;
	}
	AttentionInspection inspection;
	if (argc == 4) {
		struct stat st;
		if (*argv[3] != '/' || lstat(argv[3], &st) || !S_ISDIR(st.st_mode)) return 2;
		inspection.capture = argv[3];
	}
	llama_backend_init();
	ggml_backend_load_all();
	llama_model_params modelParams = llama_model_default_params();
	modelParams.n_gpu_layers = 0;
	llama_model* model = llama_model_load_from_file(argv[1], modelParams);
	char architecture[64] = {};
	if (model) llama_model_meta_val_str(model, "general.architecture", architecture, sizeof(architecture));
	if (!model || strcmp(architecture, "qwen3") || llama_model_n_embd(model) != 1024 || llama_model_n_layer(model) != 28) {
		fprintf(stderr, "Expected the existing Qwen3 0.6B model; no card work performed.\n");
		if (model) llama_model_free(model);
		llama_backend_free(); return 1;
	}
	llama_context_params contextParams = llama_context_default_params();
	contextParams.n_ctx = 256; contextParams.n_batch = contextParams.n_ubatch = 1;
	contextParams.n_threads = contextParams.n_threads_batch = 1;
	contextParams.op_offload = false; contextParams.offload_kqv = false;
	contextParams.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_ENABLED;
	contextParams.cb_eval = AttentionInspection::Eval; contextParams.cb_eval_user_data = &inspection;
	llama_context* context = llama_init_from_model(model, contextParams);
	llama_token tokens[16];
	int count = llama_tokenize(llama_model_get_vocab(model), "Hello", 5, tokens, 16, false, true);
	bool okay = context && count > 0 && count <= 16;
	for (int position = 0; okay && position < 2; position++) {
		inspection.position = position; inspection.calls = 0; inspection.seconds = 0;
		auto batch = llama_batch_get_one(tokens, 1);
		okay = llama_decode(context, batch) == 0 && inspection.calls == 28;
		printf("CPU_ONLY position=%d fused-attention-calls=%u observed-seconds=%.9f success=%d\n",
			position, inspection.calls, inspection.seconds, okay);
		fflush(stdout);
	}
	if (context) llama_free(context);
	llama_model_free(model); llama_backend_free();
	puts("Inspection only: single-thread CPU, two public tokens, forced per-node boundaries; timings include dispatch/synchronization overhead, not a production benchmark. No card backend or generated answer.");
	return okay ? 0 : 1;
}
