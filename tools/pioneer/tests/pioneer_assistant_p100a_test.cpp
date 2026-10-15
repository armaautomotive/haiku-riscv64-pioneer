/* SPDX-License-Identifier: MIT */
// Offline only: the explicitly substituted BF16 adapter never opens a device.
#define PIONEER_TINY_HOST_TEST
#include "../../../src/apps/assistant/P100ABackend.cpp"
#include <cassert>
extern "C" int pioneer_tiny_reference_encode(char*,char*,int*);

int main(int argc, char** argv)
{
	assert(argc==3);
	std::unique_ptr<char[][128]> pieces(new char[Vocab][128]);
	float scores[Vocab]; assert(LoadVocabulary(argv[2], pieces.get(), scores));
	const char* prompts[]={"", "Once", "Once upon a time", "A little cat", "The dog ran.", "Hello!"};
	for (const char* prompt : prompts) {
		std::vector<int> actual; assert(Encode(pieces.get(),scores,prompt,actual));
		int wanted[512]; int count=pioneer_tiny_reference_encode(argv[2],const_cast<char*>(prompt),wanted);
		assert(actual.size()==(size_t)count);
		for (int i=0;i<count;i++) assert(actual[i]==wanted[i]);
	}
	AssistantP100A backend; std::string error, answer;
	assert(backend.Load(argv[1],argv[2],error));
	assert(!backend.Load("/does-not-exist/pinned-model",argv[2],error));
	assert(!backend.Load(argv[1],"/does-not-exist/pinned-tokenizer",error));
	auto running=[]() { return false; };
	int updates=0;
	auto progress=[&](const std::string&,const char*) { updates++; };
	assert(!backend.Complete(std::string(129,'a'),8,answer,error,running,progress));
	assert(!backend.Complete("bad\ninput",8,answer,error,running,progress));
	assert(!backend.Complete("\xc3\xa9",8,answer,error,running,progress));
	assert(!backend.Complete("Once",0,answer,error,running,progress));
	assert(!backend.Complete("Once",17,answer,error,running,progress));
	assert(!backend.Complete("a a a a a a a a a a a a a a a a a a a a",8,answer,error,running,progress));
	assert(sPioneerTinyHostCalls==0 && !backend.Faulted());
	assert(backend.Complete("",8,answer,error,running,progress));
	assert(answer=="Once upon a time, there was a" && updates>0);
	assert(sPioneerTinyHostCalls==8224);
	answer.clear();
	assert(backend.Complete("Once",2,answer,error,running,progress));
	assert(!answer.empty());
	uint64_t before=sPioneerTinyHostCalls;
	assert(!backend.Complete("Once",8,answer,error,[](){return true;},progress));
	assert(sPioneerTinyHostCalls==before && !backend.Faulted());
	assert(!backend.Complete("Once",8,answer,error,[&](){return sPioneerTinyHostCalls>before;},progress));
	assert(sPioneerTinyHostCalls==before+16 && !backend.Faulted());
	sPioneerTinyFailBatch=true;
	assert(!backend.Complete("",8,answer,error,running,progress) && backend.Faulted());
	sPioneerTinyFailBatch=false; before=sPioneerTinyHostCalls;
	assert(!backend.Complete("",8,answer,error,running,progress));
	assert(sPioneerTinyHostCalls==before); // No retry after a hardware fault.
	puts("PASS: OFFLINE P100A service adapter; independent BPE, real projection path via BF16 test shim, input limits, safe cancellation and fault latch. NO card accessed.");
}
