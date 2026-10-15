/* SPDX-License-Identifier: MIT
 * Reuses the independently checked TinyStories projection runtime. See its
 * attribution for karpathy/llama2.c. No downloaded code or chat templates.
 */
#include "P100ABackend.h"
#include <memory>
#include <vector>
#define PIONEER_TINY_LIBRARY
#include "../../../tools/pioneer/tenstorrent_haiku_tinyllama.cpp"

struct AssistantP100A::Data {
	Model model;
	char pieces[Vocab][128];
	float scores[Vocab];
};

static int Lookup(const char pieces[Vocab][128], const std::string& text)
{
	for (int i = 0; i < Vocab; i++) if (text == pieces[i]) return i;
	return -1;
}

// Bounded ASCII subset of the pinned llama2.c tokenizer. Reject unsupported
// input instead of silently truncating, byte-falling back, or using ChatML.
static bool Encode(const char pieces[Vocab][128], const float* scores,
	const std::string& prompt, std::vector<int>& tokens)
{
	if (prompt.size() > 128) return TinyFailure("Story prompt limit: 128 printable ASCII characters / 16 tokens.");
	tokens.assign(1, 1);
	if (!prompt.empty()) {
		int space = Lookup(pieces, " ");
		if (space < 0) return TinyFailure("Tokenizer lacks dummy prefix.");
		tokens.push_back(space);
	}
	for (unsigned char c : prompt) {
		if (c < 32 || c > 126) return TinyFailure("Experimental story mode accepts printable ASCII only.");
		int token = Lookup(pieces, std::string(1, c));
		if (token < 0) return TinyFailure("Prompt character is outside this tiny tokenizer.");
		tokens.push_back(token);
	}
	for (;;) {
		int best = -1, replacement = -1;
		float score = -INFINITY;
		for (size_t i = 1; i + 1 < tokens.size(); i++) {
			int id = Lookup(pieces, std::string(pieces[tokens[i]]) + pieces[tokens[i+1]]);
			if (id >= 0 && scores[id] > score) { score = scores[id]; best = i; replacement = id; }
		}
		if (best < 0) break;
		tokens[best] = replacement; tokens.erase(tokens.begin() + best + 1);
	}
	if (tokens.size() > 17) return TinyFailure("Story prompt exceeds 16 tokens. Try a shorter beginning.");
	return true;
}

AssistantP100A::AssistantP100A() : fData(NULL), fFaulted(false) {}
AssistantP100A::~AssistantP100A() { delete fData; }

bool AssistantP100A::Available()
{
	PioneerTileBackend backend;
	return backend.Open() && backend.CheckBatchSupport();
}

bool AssistantP100A::Load(const char* model, const char* tokenizer, std::string& error)
{
	std::unique_ptr<Data> data(new Data);
	if (!data->model.Load(model) || !LoadVocabulary(tokenizer, data->pieces, data->scores)) {
		error = sTinyError; return false;
	}
	delete fData; fData = data.release();
	return true;
}

bool AssistantP100A::Complete(const std::string& prompt, int count,
	std::string& answer, std::string& error, const std::function<bool()>& cancelled,
	const std::function<void(const std::string&, const char*)>& progress)
{
	if (fFaulted || fData == NULL) { error = "P100A unavailable; no CPU fallback."; return false; }
	sTileFailure = false;
	// Explicit error returns: the current RISC-V runtime did not reliably
	// unwind a nested device-path exception in native testing. Never make a
	// safety stop or ordinary user cancellation depend on exception unwinding.
	bool success = [&]() -> bool {
		if (count < 1 || count > 16) return TinyFailure("P100A output limit is 1–16 tokens.");
		std::vector<int> promptTokens;
		if (!Encode(fData->pieces, fData->scores, prompt, promptTokens)) return false;
		std::unique_ptr<State> actual(new State()), reference(new State());
		PioneerTileBackend backend;
		if (!backend.Open() || !backend.CheckBatchSupport()) return TinyFailure("P100A driver ABI unavailable; no CPU fallback.");
		double start = Seconds(), last = 0;
		sTileBoundary = [&]() {
			if (cancelled()) return false;
			if (Seconds() - last >= 1) {
				char status[180];
				snprintf(status, sizeof(status), "P100A experimental hybrid: %llu tiles; %.1fs (host attention + FP32 check)",
					(unsigned long long)backend.calls, Seconds() - start);
				progress(answer, status); last = Seconds();
			}
			return true;
		};
		int token = promptTokens[0], generated = 0;
		for (int position = 0; generated < count; position++) {
			if (cancelled()) return TinyFailure("Stopped safely before the next P100A batch.");
			// The first forward is verification only; it never supplies output.
			if (!Forward(fData->model, *reference, token, position, NULL)
				|| !Forward(fData->model, *actual, token, position, &backend)) return false;
			double dot=0, a2=0, r2=0, delta2=0;
			for (int i=0; i<Vocab; i++) {
				double a=actual->logits[i], r=reference->logits[i];
				dot+=a*r; a2+=a*a; r2+=r*r; delta2+=(a-r)*(a-r);
			}
			double cosine=dot/sqrt(a2*r2), relative=sqrt(delta2/r2);
			if (!isfinite(cosine) || !isfinite(relative) || cosine < .999 || relative > .02) {
				sTileFailure = true; return TinyFailure("P100A numerical verification failed; backend disabled, no fallback.");
			}
			if (position + 1 < (int)promptTokens.size()) { token=promptTokens[position+1]; continue; }
			int next = Argmax(actual->logits); // Never substitute the reference's token.
			if (next < 0) { sTileFailure=true; return false; }
			if (next == 1 || next == 2) break;
			const char* piece = fData->pieces[next];
			if (token == 1 && *piece == ' ') piece++;
			// Never emit control bytes into the UI.
			for (const unsigned char* p=(const unsigned char*)piece; *p; p++)
				if (*p < 32 || *p > 126) return TinyFailure("Tiny model generated unsupported text.");
			answer += piece; generated++; token=next;
			progress(answer, "Generating on P100A — experimental TinyStories hybrid, not Qwen chat");
		}
		char status[200];
		snprintf(status, sizeof(status), "P100A: %d output tokens, %llu tiles, %.2fs total (includes prompt + CPU verification).",
			generated, (unsigned long long)backend.calls, Seconds()-start);
		progress(answer, status);
		return true;
	}();
	sTileBoundary = std::function<bool()>();
	fFaulted = fFaulted || sTileFailure;
	if (!success) error = sTinyError;
	return success;
}
