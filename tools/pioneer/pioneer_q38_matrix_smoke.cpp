/* SPDX-License-Identifier: MIT */
// Fixed public FULL learned matrix qualification. NOT model inference.
#include "TenstorrentHaikuQwen38.h"
#include <time.h>
static double Seconds() { timespec t={}; if (clock_gettime(CLOCK_MONOTONIC,&t)) return -1; return double(t.tv_sec)+double(t.tv_nsec)*1e-9; }
int main(int argc,char** argv)
{
	if (argc!=2 || strcmp(argv[1],"--full-ffn-repeat")) return 2;
	setvbuf(stdout,nullptr,_IOLBF,0);
	puts("FULL_MATRIX_SOURCE_SHA_BEGIN;NO_ACCELERATOR_FD_YET");
	PioneerQ38PublicModelSource source;
	if (!source.Open()) { puts("STOP_FULL_SAME_FD_SOURCE_AUTH;NO_ACCELERATOR_ACCESS"); return 1; }
	const auto* tensor=source.Model().Find("blk.0.ffn_down.weight");
	if (!tensor || tensor->type!=12 || tensor->inputs!=17408 || tensor->outputs!=5120) return 1;
	unsigned index=unsigned(tensor-source.Model().Tensors().data());
	std::vector<float> input(17408),first(5120),second(5120);
	for (unsigned k=0;k<input.size();k++) input[k]=float(int((k*29+17)%255)-127)*.00390625f;
	PioneerQ38NativeTransport io; PioneerQ38NativeSession session(io,source);
	if (!session.Open()) { puts("STOP_SESSION_OPEN;CLOSE_ONLY_NO_RETRY"); return 1; }
	puts("FULL_MATRIX_BEGIN type=12 K=17408 N=5120 max_workers=120;PRIVATE_AUTHENTIC_MODEL_WEIGHTS;NOT_TOKENS");
	double start=Seconds(); if (start<0) return 1;
	if (!session.Multiply(index,input.data(),17408,first.data(),5120)) { puts("STOP_FULL_MATRIX_FIRST;CLOSE_ONLY_NO_END_RETRY"); return 1; }
	double warm=Seconds(); uint64_t pages=session.pagesUploaded,bytes=session.bytesRetained;
	printf("FULL_MATRIX_FIRST seconds=%.6f uploaded_pages=%llu retained_bytes=%llu calls=%llu workers=%u;INCLUDES_UPLOAD_PREPARE_PROOFS\n",
		warm-start,(unsigned long long)pages,(unsigned long long)bytes,(unsigned long long)session.calls,session.peakWorkers);
	if (!session.Multiply(index,input.data(),17408,second.data(),5120)) { puts("STOP_FULL_MATRIX_REPEAT;CLOSE_ONLY_NO_END_RETRY"); return 1; }
	double end=Seconds(); unsigned differences=0;
	for (unsigned row=0;row<5120;row++) differences+=PioneerAttentionBits(first[row])!=PioneerAttentionBits(second[row]);
	bool retained=pages==session.pagesUploaded && bytes==session.bytesRetained;
	bool clean=session.Finish();
	printf("%s_FULL_MATRIX_REPEAT warm_seconds=%.6f exact_repeat_errors=%u weights_reused=%u known_clean_end=%u;REPEATABILITY_NOT_INDEPENDENT_NUMERIC_REFERENCE_OR_LLM_INFERENCE_OR_TOKEN_RATE\n",
		!differences && retained && clean?"PASS":"FAIL",end-warm,differences,unsigned(retained),unsigned(clean));
	return differences || !retained || !clean || end<warm || warm<start?1:0;
}
