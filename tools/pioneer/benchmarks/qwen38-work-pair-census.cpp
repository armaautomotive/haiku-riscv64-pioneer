/* SPDX-License-Identifier: MIT */
// Read-only model metadata census. This counts structural pair candidates,
// not actual GGML graph aliases, device launches, or performance.
#include "PioneerQwen38Gguf.h"
#include <cstdio>

static bool Compatible(const PioneerQ38Tensor* a, const PioneerQ38Tensor* b)
{
	return a && b && a != b && a->rank == 2 && b->rank == 2
		&& a->type == b->type && a->inputs == b->inputs
		&& a->outputs && b->outputs;
}

static const PioneerQ38Tensor* FindLayer(const PioneerQwen38Gguf& model,
	unsigned layer, const char* role)
{
	char name[96];
	std::snprintf(name, sizeof(name), "blk.%u.%s.weight", layer, role);
	return model.Find(name);
}

int main(int argc, char** argv)
{
	if (argc != 2) return 2;
	FILE* file = std::fopen(argv[1], "rb");
	if (!file) return 3;
	PioneerQwen38Gguf model;
	if (!model.Open(file)) { std::fclose(file); return 4; }

	unsigned ffnPairs = 0, recurrentGatePairs = 0;
	unsigned recurrentAlphaBetaPairs = 0, attentionQkvPairs = 0;
	for (unsigned layer = 0; layer < 64; ++layer) {
		if (!Compatible(FindLayer(model, layer, "ffn_up"),
			FindLayer(model, layer, "ffn_gate"))) return 5;
		ffnPairs++;
		if (layer % 4 == 3) {
			const auto* q = FindLayer(model, layer, "attn_q");
			const auto* k = FindLayer(model, layer, "attn_k");
			const auto* v = FindLayer(model, layer, "attn_v");
			if (!Compatible(q, k) || !Compatible(q, v) || !Compatible(k, v)) return 6;
			// The GGML backend pairs at most two projection nodes per call.
			attentionQkvPairs++;
		} else {
			if (!Compatible(FindLayer(model, layer, "attn_qkv"),
				FindLayer(model, layer, "attn_gate"))) return 7;
			if (!Compatible(FindLayer(model, layer, "ssm_alpha"),
				FindLayer(model, layer, "ssm_beta"))) return 8;
			recurrentGatePairs++;
			recurrentAlphaBetaPairs++;
		}
	}
	if (!model.Stable() || model.Tensors().size() != 851
		|| ffnPairs != 64 || recurrentGatePairs != 48
		|| recurrentAlphaBetaPairs != 48 || attentionQkvPairs != 16) return 9;
	const unsigned pairs = ffnPairs + recurrentGatePairs
		+ recurrentAlphaBetaPairs + attentionQkvPairs;
	// The independently measured static fleet plan has 505 matrix-root
	// dispatches/token: 497 one-wave matrices plus eight extra output.weight
	// waves. Pairing and wave batching reduce host ioctls, not root dispatches.
	// This upper bound assumes every structural candidate appears in the graph
	// with the exact same activation pointer and is paired once.
	if (pairs >= 505) return 10;
	std::printf("HEADER_ONLY_EXACT851_NO_FULL_SHA_RECHECK_NO_WEIGHT_READ_NO_GRAPH_OR_DEVICE\n");
	std::printf("structural_ffn_up_gate_pairs %u\n", ffnPairs);
	std::printf("structural_recurrent_qkv_gate_pairs %u\n", recurrentGatePairs);
	std::printf("structural_recurrent_alpha_beta_pairs %u\n", recurrentAlphaBetaPairs);
	std::printf("structural_attention_qkv_pairs %u\n", attentionQkvPairs);
	std::printf("structural_pair_upper_bound %u\n", pairs);
	std::printf("matrix_root_dispatches_per_token_baseline 505\n");
	std::printf("matrix_root_dispatches_per_token_with_batch_candidate 505\n");
	std::printf("host_ioctl_calls_per_token_unbatched 505\n");
	std::printf("host_ioctl_calls_per_token_if_all_pairs_match_pair_only %u\n", 505 - pairs);
	std::printf("host_ioctl_calls_per_token_if_pairs_and_two_wave_output_match %u\n", 501 - pairs);
	std::printf("avoided_host_ioctl_calls_upper_bound_pair_only %u\n", pairs);
	std::printf("NOT_ACTUAL_GRAPH_PAIR_COUNT_NOT_DEVICE_CYCLES_NOT_TOKENS_PER_SECOND\n");
	const bool closed = std::fclose(file) == 0;
	return closed ? 0 : 11;
}
