/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN_WARM_BENCH_H
#define PIONEER_QWEN_WARM_BENCH_H
#include <string>
#include <vector>
// Standalone public-prompt benchmark only, never Assistant state. The CPU
// context MUST have been created before registering P100A, with batch 128;
// P100A uses batch 1 and an explicitly reported host-thread count. The CPU
// reference keeps 32 threads and identical quantized weights.
// Weight upload is already warm and must not recur in any measured request.
// Timings sum synchronous decode+greedy-sampling CPU wall time; logit checks,
// tokenization, KV reset, logging, model load and cold weight upload excluded.
static bool PioneerQwenWarmBenchmark(llama_model* model,llama_context* cpu,llama_context* card,uint64_t expectedTensors,unsigned cardThreads=32)
{
	if (cardThreads!=1 && cardThreads!=32) return false;
	const llama_vocab* vocab=llama_model_get_vocab(model); int vocabulary=llama_vocab_n_tokens(vocab);
	const char* questions[]={
		"Explain why the sky looks blue in a detailed paragraph of at least one hundred words.",
		"Write a story of at least one hundred words about a lighthouse keeper and a lost robot.",
		"Describe three practical ways to save energy at home in at least one hundred words."
	};
	PioneerGgmlStats warm={};
	if (!PioneerGgmlSnapshot(warm) || warm.failed || warm.tensors!=expectedTensors || !warm.bytes || !warm.matvecs || warm.uploads!=warm.bytes/4096) return false;
	bool allFaster=true;
	printf("WARM BENCH: three public requests, up to 16 greedy output tokens each; CPU32 batch128 vs P100A/CPU%u batch1. Cold load/upload, KV reset, tokenization and diagnostics excluded; not device-only throughput.\n",cardThreads);
	for (unsigned repeat=0;repeat<3;repeat++) {
		std::string prompt=std::string("<|im_start|>user\n")+questions[repeat]+"<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
		std::vector<llama_token> input(128),golden;
		std::vector<float> logits;
		int n=llama_tokenize(vocab,prompt.c_str(),int(prompt.size()),input.data(),int(input.size()),false,true);
		if (n<=0 || n>128) return false;
		input.resize(size_t(n));
		llama_memory_clear(llama_get_memory(cpu),true);
		llama_batch prefill=llama_batch_get_one(input.data(),n);
		double start=Seconds();
		if (llama_decode(cpu,prefill)) return false;
		const float* values=llama_get_logits(cpu); if (!values) return false;
		int token=Argmax(values,vocabulary); double cpuPrompt=Seconds()-start,cpuGeneration=0;
		if (token<0 || !isfinite(cpuPrompt) || cpuPrompt<=0) return false;
		for (unsigned generated=0;generated<16;generated++) {
			golden.push_back(token); logits.insert(logits.end(),values,values+vocabulary);
			if (llama_vocab_is_eog(vocab,token) || generated==15) break;
			llama_token previous=token; llama_batch batch=llama_batch_get_one(&previous,1); start=Seconds();
			if (llama_decode(cpu,batch)) return false;
			values=llama_get_logits(cpu); if (!values) return false;
			token=Argmax(values,vocabulary); cpuGeneration+=Seconds()-start; if (token<0) return false;
		}
		// Never pad an early EOS or force CPU tokens into the card trajectory
		// to manufacture a generation rate. A short reply is not this test.
		if (golden.size()<8 || !isfinite(cpuGeneration) || cpuGeneration<=0) { fprintf(stderr,"Warm benchmark lacks enough natural generation samples/timing\n"); return false; }
		printf("WARM request%u CPU32 baseline: prompt%d output%zu samples%zu generation%.6fs (%.6f tok/s); quality comparison still pending\n",
			repeat,n,golden.size(),golden.size()-1,cpuGeneration,double(golden.size()-1)/cpuGeneration);
		llama_memory_clear(llama_get_memory(card),true);
		double cardPrompt=0,cardGeneration=0;
		for (int position=0;position<n;position++) {
			llama_token t=input[size_t(position)]; llama_batch batch=llama_batch_get_one(&t,1); start=Seconds();
			if (llama_decode(card,batch)) return false;
			if (position+1==n) { values=llama_get_logits(card); if (!values) return false; token=Argmax(values,vocabulary); }
			cardPrompt+=Seconds()-start;
			if ((position+1)%8==0 || position+1==n) printf("WARM request%u P100A prefill%d/%d decoded; cumulative%.6fs\n",repeat,position+1,n,cardPrompt);
		}
		for (unsigned generated=0;generated<golden.size();generated++) {
			if (token<0 || token!=golden[generated]) { fprintf(stderr,"Warm benchmark greedy mismatch; no reference substitution\n"); return false; }
			const float* reference=logits.data()+size_t(generated)*vocabulary;
			double dot=0,a2=0,r2=0,delta=0;
			for (int i=0;i<vocabulary;i++) { double a=values[i],r=reference[i]; dot+=a*r; a2+=a*a; r2+=r*r; delta+=(a-r)*(a-r); }
			double cosine=dot/sqrt(a2*r2),relative=sqrt(delta/r2);
			if (!isfinite(cosine) || !isfinite(relative) || cosine<.999 || relative>.02) { fprintf(stderr,"Warm benchmark full-logit gate failed\n"); return false; }
			if (generated+1==golden.size()) break;
			llama_token previous=token; llama_batch batch=llama_batch_get_one(&previous,1); start=Seconds();
			if (llama_decode(card,batch)) return false;
			values=llama_get_logits(card); if (!values) return false;
			token=Argmax(values,vocabulary); cardGeneration+=Seconds()-start;
			if ((generated+1)%8==0) printf("WARM request%u P100A generation%u/%zu decoded; cumulative%.6fs; checks outside timing\n",repeat,generated+1,golden.size()-1,cardGeneration);
		}
		PioneerGgmlStats after={};
		if (!PioneerGgmlSnapshot(after) || after.failed || after.tensors!=warm.tensors || after.uploads!=warm.uploads
			|| after.bytes!=warm.bytes || after.matvecs<=warm.matvecs || !isfinite(cardPrompt) || !isfinite(cardGeneration) || cardPrompt<=0 || cardGeneration<=0) return false;
		double cpuTotal=cpuPrompt+cpuGeneration,cardTotal=cardPrompt+cardGeneration;
		bool faster=cardTotal<cpuTotal*.95 && cardGeneration<cpuGeneration*.95; allFaster &= faster;
		printf("{\"warm_request\":%u,\"cpu_threads\":32,\"card_host_threads\":%u,\"cpu_prompt_batch\":128,\"card_prompt_batch\":1,\"prompt_tokens\":%d,\"output_tokens\":%zu,\"generation_samples\":%zu,\"cpu_ttft_s\":%.9f,\"p100a_ttft_s\":%.9f,\"cpu_generation_s\":%.9f,\"p100a_generation_s\":%.9f,\"cpu_total_s\":%.9f,\"p100a_total_s\":%.9f,\"cpu_tps\":%.9f,\"p100a_tps\":%.9f,\"total_speedup\":%.9f,\"generation_speedup\":%.9f,\"quality_passed\":true,\"no_weight_reupload\":true,\"faster_by_5_percent\":%s}\n",
			repeat,cardThreads,n,golden.size(),golden.size()-1,cpuPrompt,cardPrompt,cpuGeneration,cardGeneration,cpuTotal,cardTotal,
			double(golden.size()-1)/cpuGeneration,double(golden.size()-1)/cardGeneration,cpuTotal/cardTotal,cpuGeneration/cardGeneration,faster?"true":"false");
	}
	printf("WARM SPEEDUP %s: all three requests must improve both generation and total decode+sampling latency by at least 5%%; not an Assistant/27B qualification\n",allFaster?"PASS":"FAIL");
	return allFaster;
}
#endif
