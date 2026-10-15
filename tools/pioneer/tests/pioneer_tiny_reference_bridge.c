/* SPDX-License-Identifier: MIT */
// Compile only against the separately downloaded, hash-pinned MIT reference.
#define TESTING
#include PIONEER_TINY_REFERENCE_SOURCE
void* pioneer_tiny_reference_create(char* path) {
	Transformer* t=calloc(1,sizeof(Transformer)); build_transformer(t,path); return t;
}
const float* pioneer_tiny_reference_forward(void* t,int token,int position) {
	return forward((Transformer*)t,token,position);
}
void pioneer_tiny_reference_free(void* t) { free_transformer((Transformer*)t); free(t); }
int pioneer_tiny_reference_encode(char* path,char* text,int* tokens) {
	Tokenizer t; build_tokenizer(&t,path,512);
	int count=0; encode(&t,text,1,0,tokens,&count);
	free_tokenizer(&t); return count;
}
