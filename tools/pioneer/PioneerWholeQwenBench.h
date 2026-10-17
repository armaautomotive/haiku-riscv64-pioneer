/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_WHOLE_QWEN_BENCH_H
#define PIONEER_WHOLE_QWEN_BENCH_H
// Public-prompt diagnostic ONLY. Full CPU references are computed BEFORE
// each timed GPU request. Never supplies reference tokens as a fallback.
static bool PioneerWholeQwenBenchmark(PioneerResidentDevice& card,llama_model* model,llama_context* cpu,ggml_tensor* embedding,bool greedy=false,bool batched=false)
{
	const char* questions[]={
		"Explain why the sky looks blue in a detailed paragraph of at least one hundred words.",
		"Write a story of at least one hundred words about a lighthouse keeper and a lost robot.",
		"Describe three practical ways to save energy at home in at least one hundred words."
	};
	const llama_vocab* vocab=llama_model_get_vocab(model); const uint64_t uploads=card.uploads;
	std::vector<float> input(1024),output(151936); uint32_t rope[128],cardToken=0,cardLogit=0; bool allFaster=true;
	if (batched && (!greedy || !card.BatchEnabled())) return false;
	auto decode=[&](llama_token token,unsigned position,bool small) {
		ggml_get_type_traits(embedding->type)->to_float(static_cast<const uint8_t*>(embedding->data)+uint64_t(token)*embedding->nb[1],input.data(),1024);
		float theta=float(position),step=powf(1000000.0f,-2.0f/128);
		for (unsigned i=0;i<64;i++) { rope[2*i]=PioneerAttentionBits(cosf(theta)); rope[2*i+1]=PioneerAttentionBits(sinf(theta)); theta*=step; }
		return small?card.DecodeGreedy(position+1,reinterpret_cast<const uint32_t*>(input.data()),rope,cardToken,cardLogit)
			:card.DecodeWhole(position+1,reinterpret_cast<const uint32_t*>(input.data()),rope,reinterpret_cast<uint32_t*>(output.data()));
	};
	puts("WHOLE WARM BENCH:three public requests/up to16 greedy tokens;CPU32 batch128 vs whole-P100A batch1;decode+sampling wall time. Model load/upload/registry/KV reset/tokenization/logging/quality checks excluded. NO CPU reference decode inside any timed GPU request.");
	if (greedy) puts("GREEDY BENCH:timed GPU path returns one128B on-card token packet,NO host vocabulary/argmax/reference validation. Complete full-logit replay and selected-bit/token/coverage checks follow EACH timed request,not inside its timers.");
	if (batched) puts("BATCH BENCH:up toTWO on-card positions per256B result;NO host call between positions,all CPU reference/quality validation outside timed GPU generation.");
	for (unsigned repeat=0;repeat<3;repeat++) {
		std::string prompt=std::string("<|im_start|>user\n")+questions[repeat]+"<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
		std::vector<llama_token> tokens(128),golden; std::vector<float> references;
		int n=llama_tokenize(vocab,prompt.c_str(),int(prompt.size()),tokens.data(),int(tokens.size()),false,true);
		if (n<=0 || n>128) return false;
		llama_memory_clear(llama_get_memory(cpu),true);
		auto batch=llama_batch_get_one(tokens.data(),n); double start=Seconds();
		if (llama_decode(cpu,batch)) return false;
		const float* values=llama_get_logits_ith(cpu,-1); int token=Argmax(values);
		double cpuPrompt=Seconds()-start,cpuGeneration=0;
		for (unsigned generated=0;generated<16;generated++) {
			if (token<0) return false;
			golden.push_back(token); references.insert(references.end(),values,values+151936);
			if (llama_vocab_is_eog(vocab,token) || generated==15) break;
			llama_token previous=token; batch=llama_batch_get_one(&previous,1); start=Seconds();
			if (llama_decode(cpu,batch)) return false;
			values=llama_get_logits_ith(cpu,-1); token=Argmax(values); cpuGeneration+=Seconds()-start;
		}
		if (golden.size()<8 || !(cpuPrompt>0) || !(cpuGeneration>0)) { fprintf(stderr,"Whole benchmark lacks enough natural generation samples/timing\n"); return false; }
		printf("WHOLE WARM request%u CPU32 prompt%d %.6fs,output%zu generation%.6fs %.6ftok/s;GPU quality pending\n",repeat,n,cpuPrompt,golden.size(),cpuGeneration,double(golden.size()-1)/cpuGeneration);
		uint64_t projections=card.calls,attention=card.attentionCalls,wide=card.wideCalls,smallCalls=card.greedyCalls,batchCalls=card.batchCalls; double cardPrompt=0,cardGeneration=0;
		std::vector<llama_token> actual; std::vector<uint32_t> bestBits;
		for (int position=0;position<n;position++) {
			start=Seconds(); if (!decode(tokens[size_t(position)],unsigned(position),greedy)) return false;
			if (position+1==n) token=greedy?int(cardToken):Argmax(output.data());
			cardPrompt+=Seconds()-start;
			if ((position+1)%8==0 || position+1==n) printf("WHOLE WARM request%u GPU prefill%d/%d %.6fs\n",repeat,position+1,n,cardPrompt);
		}
		if (batched) {
			if (token<0 || token>=151936) return false;
			actual.push_back(token); bestBits.push_back(cardLogit);
			while (actual.size()<golden.size() && !llama_vocab_is_eog(vocab,actual.back())) {
				unsigned remaining=unsigned(golden.size()-actual.size()),steps=remaining<2?remaining:2;
				unsigned first=unsigned(n)+unsigned(actual.size()); uint32_t packet[64];
				start=Seconds();
				ggml_get_type_traits(embedding->type)->to_float(static_cast<const uint8_t*>(embedding->data)+uint64_t(actual.back())*embedding->nb[1],input.data(),1024);
				if (!card.DecodeBatch(first,steps,reinterpret_cast<const uint32_t*>(input.data()),packet)) return false;
				cardGeneration+=Seconds()-start;
				for (unsigned i=0;i<packet[3];i++) { actual.push_back(int(packet[8+i])); bestBits.push_back(packet[24+i]); }
			}
			if (actual.size()!=golden.size()) { fprintf(stderr,"Autonomous generation length/EOG mismatch;NO reference substitution\n"); return false; }
		} else for (unsigned generated=0;generated<golden.size();generated++) {
			if (token<0 || token>=151936) return false;
			if (greedy) { actual.push_back(token); bestBits.push_back(cardLogit); }
			else {
				if (token!=golden[generated]) { fprintf(stderr,"Whole benchmark greedy mismatch;NO reference substitution\n"); return false; }
				double dot=0,a2=0,b2=0,d2=0; const float* reference=references.data()+size_t(generated)*151936;
				for (unsigned i=0;i<151936;i++) { double a=output[i],b=reference[i],d=a-b; dot+=a*b; a2+=a*a; b2+=b*b; d2+=d*d; }
				double cosine=dot/sqrt(a2*b2),relative=sqrt(d2/(b2+1e-30));
				if (!isfinite(cosine) || !isfinite(relative) || cosine<.999 || relative>.02) { fprintf(stderr,"Whole benchmark full-logit quality gate FAILED cosine%.9f L2%.9f\n",cosine,relative); return false; }
			}
			if (generated+1==golden.size()) break;
			start=Seconds(); if (!decode(llama_token(token),unsigned(n)+generated,greedy)) return false;
			token=greedy?int(cardToken):Argmax(output.data()); cardGeneration+=Seconds()-start;
		}
		uint64_t positions=uint64_t(n)+golden.size()-1;
		if (greedy) {
			// Untimed complete replay uses ACTUAL generated tokens,never a
			// teacher-forced correction. A mismatch stops before qualification.
			for (unsigned position=0;position<positions;position++) {
				llama_token inputToken=position<unsigned(n)?tokens[position]:actual[position-unsigned(n)];
				if (!decode(inputToken,position,false)) return false;
				if (position+1<unsigned(n)) continue;
				unsigned generated=position+1-unsigned(n); int best=Argmax(output.data());
				if (best<0 || best!=actual[generated] || best!=golden[generated] || PioneerAttentionBits(output[unsigned(best)])!=bestBits[generated]) {
					fprintf(stderr,"Greedy full-logit replay token/selected-bit mismatch;NO substitution\n"); return false; }
				double dot=0,a2=0,b2=0,d2=0; const float* reference=references.data()+size_t(generated)*151936;
				for (unsigned i=0;i<151936;i++) { double a=output[i],b=reference[i],d=a-b; dot+=a*b; a2+=a*a; b2+=b*b; d2+=d*d; }
				double cosine=dot/sqrt(a2*b2),relative=sqrt(d2/(b2+1e-30));
				if (!isfinite(cosine) || !isfinite(relative) || cosine<.999 || relative>.02) {
					fprintf(stderr,"Greedy full-logit replay quality FAILED cosine%.9f L2%.9f\n",cosine,relative); return false; }
			}
			printf("GREEDY request%u untimed full-vocabulary replay PASS tokens/selected bits/all logits\n",repeat);
		}
		if (card.uploads!=uploads || card.calls-projections!=197*positions*(greedy?2:1) || card.attentionCalls-attention!=28*positions*(greedy?2:1)
			|| card.wideCalls-wide!=positions || card.greedyCalls-smallCalls!=(greedy?positions:0)
			|| card.batchCalls-batchCalls!=(batched?golden.size()/2:0)
			|| !(cardPrompt>0) || !(cardGeneration>0) || !isfinite(cardGeneration)) return false;
		bool faster=cardGeneration<cpuGeneration; allFaster &= faster;
		printf("WHOLE WARM request%u PASS full logits/greedy/coverage;GPU prompt%.6fs generation%.6fs %.6ftok/s CPU%.6ftok/s speedup%.6fx faster%u uploads%llu\n",
			repeat,cardPrompt,cardGeneration,double(golden.size()-1)/cardGeneration,double(golden.size()-1)/cpuGeneration,cpuGeneration/cardGeneration,faster,(unsigned long long)card.uploads);
	}
	printf("WHOLE WARM three-request speed gate:%s;quality/coverage required on every request\n",allFaster?"PASS":"FAIL");
	return allFaster;
}
#endif
