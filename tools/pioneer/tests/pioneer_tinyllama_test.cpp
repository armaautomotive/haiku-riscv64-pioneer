/* SPDX-License-Identifier: MIT */
#define PIONEER_TINY_HOST_TEST
#define main pioneer_unused_native_main
#include "../tenstorrent_haiku_tinyllama.cpp"
#undef main
#include <cassert>
extern "C" {
	void* pioneer_tiny_reference_create(char*);
	const float* pioneer_tiny_reference_forward(void*,int,int);
	void pioneer_tiny_reference_free(void*);
}
int main(int argc,char** argv) {
	assert(argc==3);
	Model model; assert(model.Load(argv[1]));
	char pieces[Vocab][128]; assert(LoadVocabulary(argv[2],pieces));
	State* state=(State*)calloc(1,sizeof(State)); assert(state);
	State* bf16=(State*)calloc(1,sizeof(State)); assert(bf16);
	void* reference=pioneer_tiny_reference_create(argv[1]);
	PioneerTileBackend software; int token=1;
	for(int pos=0;pos<8;pos++) {
		const float* expected=pioneer_tiny_reference_forward(reference,token,pos);
		assert(Forward(model,*state,token,pos,NULL));
		assert(Forward(model,*bf16,token,pos,&software));
		double dot=0,a2=0,r2=0;
		for(int i=0;i<Vocab;i++) {
			assert(fabsf(expected[i]-state->logits[i])<0.0001f);
			dot+=double(expected[i])*bf16->logits[i]; a2+=double(bf16->logits[i])*bf16->logits[i]; r2+=double(expected[i])*expected[i];
		}
		assert(dot/sqrt(a2*r2)>0.999 && Argmax(expected)==Argmax(bf16->logits));
		token=Argmax(expected); printf("OFFLINE token %d id %d piece=%s\n",pos,token,pieces[token]);
	}
	assert(software.calls==8224);
	pioneer_tiny_reference_free(reference); free(state); free(bf16);
	printf("PASS: independent pretrained model layout/FP32 reference and BF16 software adapter; NO hardware accessed\n");
}
