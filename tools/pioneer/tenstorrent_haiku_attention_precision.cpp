/* SPDX-License-Identifier: MIT */
// CPU-only precision isolation. Does not link or register a Pioneer backend.
#include <llama.h>
#include <ggml-cpu.h>
#include <ggml-backend.h>
#include "PioneerAttentionFixture.h"
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <vector>

static double Relative(const float* a, const float* b, size_t n)
{
	double difference = 0, norm = 0;
	for (size_t i = 0; i < n; i++) {
		if (!isfinite(a[i]) || !isfinite(b[i])) return INFINITY;
		double delta = double(a[i]) - b[i]; difference += delta * delta; norm += double(b[i]) * b[i];
	}
	return sqrt(difference / (norm > 0 ? norm : 1));
}

struct Inspection {
	unsigned position = 0, layer = 0;
	bool okay = true;
	std::string capture;
	static bool Eval(ggml_tensor* op, bool ask, void* opaque)
	{
		if (ask) return op->op == GGML_OP_FLASH_ATTN_EXT;
		if (op->op != GGML_OP_FLASH_ATTN_EXT) return true;
		auto& self = *static_cast<Inspection*>(opaque);
		if (!self.Replay(op)) { self.okay = false; return false; }
		self.layer++;
		return true;
	}
	static bool Write(int fd, const void* data, size_t bytes)
	{
		const char* cursor = static_cast<const char*>(data);
		while (bytes) {
			ssize_t size = write(fd, cursor, bytes);
			if (size < 0 && errno == EINTR) continue;
			if (size <= 0) return false;
			cursor += size; bytes -= size_t(size);
		}
		return true;
	}
	bool Replay(ggml_tensor* op)
	{
		const auto* sourceQ = op->src[0]; const auto* sourceK = op->src[1]; const auto* sourceV = op->src[2];
		if (!sourceQ || !sourceK || !sourceV || sourceQ->type != GGML_TYPE_F32
			|| sourceK->type != GGML_TYPE_F16 || sourceV->type != GGML_TYPE_F16
			|| sourceQ->ne[0] != 128 || sourceQ->ne[1] != 1 || sourceQ->ne[2] != 16
			|| sourceK->nb[0] != 2 || sourceK->nb[1] != 2048 || sourceK->nb[2] != 256
			|| sourceV->nb[0] != 2 || sourceV->nb[1] != 2048 || sourceV->nb[2] != 256) return false;
		unsigned n = position + 1;
		std::vector<float> query(2048), flash(2048), scalar(2048), rounded(2048);
		std::vector<uint16_t> keys(n * 1024), values(n * 1024);
		ggml_backend_tensor_get(sourceQ, query.data(), 0, query.size() * 4);
		ggml_backend_tensor_get(sourceK, keys.data(), 0, keys.size() * 2);
		ggml_backend_tensor_get(sourceV, values.data(), 0, values.size() * 2);
		ggml_backend_tensor_get(op, flash.data(), 0, flash.size() * 4);
		ggml_init_params init = {4 * 1024 * 1024, NULL, false};
		ggml_context* ctx = ggml_init(init);
		if (!ctx) return false;
		auto* q = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, 128, 1, 16);
		auto* k = ggml_new_tensor_3d(ctx, GGML_TYPE_F16, 128, n, 8);
		auto* v = ggml_new_tensor_3d(ctx, GGML_TYPE_F16, n, 128, 8);
		memcpy(q->data, query.data(), query.size() * 4);
		for (unsigned h = 0; h < 8; h++) for (unsigned t = 0; t < n; t++) for (unsigned d = 0; d < 128; d++) {
			static_cast<uint16_t*>(k->data)[h * n * 128 + t * 128 + d] = keys[t * 1024 + h * 128 + d];
			static_cast<uint16_t*>(v->data)[h * n * 128 + d * n + t] = values[t * 1024 + h * 128 + d];
		}
		auto* score = ggml_mul_mat(ctx, k, q); ggml_mul_mat_set_prec(score, GGML_PREC_F32);
		auto* probability = ggml_soft_max_ext(ctx, score, NULL, 0.0883883461356163f, 0);
		auto* output = ggml_mul_mat(ctx, v, probability);
		auto* graph = ggml_new_graph(ctx); ggml_build_forward_expand(graph, output);
		bool success = ggml_graph_compute_with_ctx(ctx, graph, 1) == GGML_STATUS_SUCCESS;
		if (success) for (unsigned h = 0; h < 16; h++) {
			success &= PioneerAttentionHead(query.data() + h * 128, keys.data() + (h / 2) * 128,
				values.data() + (h / 2) * 128, n, 1024, scalar.data() + h * 128);
			// Independent double sums, pinned runtime's half conversion, libc exp.
			std::vector<double> scores(n); double maximum = -INFINITY, denominator = 0;
			for (unsigned t = 0; t < n; t++) {
				double dot = 0;
				for (unsigned d = 0; d < 128; d++) {
					float a = ggml_fp16_to_fp32(ggml_fp32_to_fp16(query[h * 128 + d]));
					float b = ggml_fp16_to_fp32(keys[t * 1024 + (h / 2) * 128 + d]);
					dot += double(a * b);
				}
				scores[t] = float(float(dot) * 0.0883883461356163f); if (scores[t] > maximum) maximum = scores[t];
			}
			for (unsigned t = 0; t < n; t++) { scores[t] = expf(float(scores[t] - maximum)); denominator += scores[t]; }
			for (unsigned d = 0; d < 128; d++) {
				double sum = 0;
				for (unsigned t = 0; t < n; t++) {
					float p = ggml_fp16_to_fp32(ggml_fp32_to_fp16(float(scores[t] * float(1.0 / denominator))));
					sum += double(p * ggml_fp16_to_fp32(values[t * 1024 + (h / 2) * 128 + d]));
				}
				rounded[h * 128 + d] = float(sum);
			}
		}
		if (success) printf("CPU_REPLAY position=%u layer=%u tokens=%u flash/decomposed=%.9g scalar-nonflash/decomposed=%.9g rounded/decomposed=%.9g\n",
			position, layer, n, Relative(flash.data(), static_cast<float*>(output->data), 2048),
			Relative(scalar.data(), static_cast<float*>(output->data), 2048),
			Relative(rounded.data(), static_cast<float*>(output->data), 2048));
		if (success && !capture.empty()) {
			PioneerAttentionFixture header = {0x4e545450,1,layer,n,128,16,8,2048,n*1024,2048,
				PioneerAttentionBits(0.0883883461356163f),0};
			char leaf[80]; snprintf(leaf, sizeof(leaf), "/attention-p%02u-l%02u.bin", position, layer);
			int fd = open((capture + leaf).c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
			success = fd >= 0 && Write(fd, &header, sizeof(header)) && Write(fd, query.data(), query.size()*4)
				&& Write(fd, keys.data(), keys.size()*2) && Write(fd, values.data(), values.size()*2)
				&& Write(fd, output->data, 8192) && fsync(fd) == 0;
			if (fd >= 0 && close(fd)) success = false;
			if (success) printf("PUBLIC_FIXTURE position=%u layer=%u golden=CPU-nonflash-replay\n", position, layer);
		}
		ggml_free(ctx); return success;
	}
};

static void Log(ggml_log_level level, const char* message, void*)
{
	if (level >= GGML_LOG_LEVEL_WARN) fputs(message, stderr);
}

int main(int argc, char** argv)
{
	if (argc != 2 && (argc != 4 || strcmp(argv[2], "--capture"))) {
		fprintf(stderr, "Usage: attention_precision PINNED_QWEN3_0.6B_Q8_0 [--capture FRESH_DIRECTORY]\n"); return 2;
	}
	setvbuf(stdout, NULL, _IONBF, 0);
	llama_log_set(Log, NULL); llama_backend_init(); ggml_backend_register(ggml_backend_cpu_reg());
	llama_model_params modelParams = llama_model_default_params(); modelParams.n_gpu_layers = 0;
	llama_model* model = llama_model_load_from_file(argv[1], modelParams);
	char arch[64] = {};
	if (model) llama_model_meta_val_str(model, "general.architecture", arch, sizeof(arch));
	if (!model || strcmp(arch, "qwen3") || llama_model_n_embd(model) != 1024 || llama_model_n_layer(model) != 28) return 1;
	Inspection inspection;
	if (argc == 4) {
		struct stat st;
		if (*argv[3] != '/' || lstat(argv[3], &st) || !S_ISDIR(st.st_mode)) return 2;
		inspection.capture = argv[3];
	}
	llama_context_params params = llama_context_default_params();
	params.n_ctx = 256; params.n_batch = params.n_ubatch = 1;
	params.n_threads = params.n_threads_batch = 32;
	params.op_offload = params.offload_kqv = false;
	params.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_DISABLED;
	llama_context* reference = llama_init_from_model(model, params);
	params.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_ENABLED;
	params.cb_eval = Inspection::Eval; params.cb_eval_user_data = &inspection;
	llama_context* flash = llama_init_from_model(model, params);
	const char* prompt = "<|im_start|>user\nWhat is the capital of France?<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
	llama_token tokens[256];
	int count = llama_tokenize(llama_model_get_vocab(model), prompt, int(strlen(prompt)), tokens, 256, false, true);
	bool success = reference && flash && count >= 3;
	for (unsigned position = 0; success && position < 3; position++) {
		inspection.position = position; inspection.layer = 0;
		auto batch = llama_batch_get_one(tokens + position, 1);
		success = !llama_decode(reference, batch) && !llama_decode(flash, batch) && inspection.okay && inspection.layer == 28;
		if (success) printf("CPU_ONLY position=%u input-token=%d flash/nonflash-logits-L2=%.9g\n", position, tokens[position],
			Relative(llama_get_logits(flash), llama_get_logits(reference), llama_vocab_n_tokens(llama_model_get_vocab(model))));
	}
	if (flash) llama_free(flash);
	if (reference) llama_free(reference);
	llama_model_free(model); llama_backend_free();
	puts("CPU-only public-token precision diagnostic; no Pioneer registration, driver access, generated answer, or Assistant changes.");
	return success ? 0 : 1;
}
