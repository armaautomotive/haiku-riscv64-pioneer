/* SPDX-License-Identifier: MIT
 * Fixed-shape TinyStories bootstrap harness, not a Qwen runtime.
 * Model layout/architecture reference: karpathy/llama2.c at
 * 350e04fe35433e6d2941dce5a1f53308f87058eb (MIT, copyright 2023 Andrej).
 * Learned linear projections run on the P100A; norms, attention/KV,
 * nonlinearities, embedding lookup and partial-tile sums run on the host.
 * An independent FP32 CPU forward pass checks logits; never a fallback.
 */
#ifdef PIONEER_TINY_HOST_TEST
#include "tests/PioneerTinyHostBackend.h"
#else
#include "TenstorrentHaikuTile.h"
#endif
#include <math.h>
#include <stdlib.h>
#include <time.h>

enum { Dim=64, Hidden=172, Layers=5, Heads=8, KvHeads=4, Kv=32, Head=8, Vocab=512, Seq=512 };
[[noreturn]] static void Fail(const char* text) { fprintf(stderr,"STOP: %s\n",text); exit(1); }
static double Seconds() { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
struct Model {
	float* storage;
	const float *embedding,*attentionNorm,*q,*k,*v,*o,*ffnNorm,*up,*down,*gate,*finalNorm;
	Model() : storage(NULL) {}
	~Model() { free(storage); }
	void Load(const char* path) {
		FILE* file = fopen(path,"rb"); if (!file) Fail("cannot open pinned model");
		int32_t config[7];
		const int32_t wanted[] = {Dim,Hidden,Layers,Heads,KvHeads,Vocab,Seq};
		if (fread(config,sizeof(config),1,file) != 1 || memcmp(config,wanted,sizeof(config))) Fail("unsupported model header");
		if (fseek(file,0,SEEK_END) || ftell(file) != 1056540 || fseek(file,28,SEEK_SET)) Fail("wrong model length");
		storage = (float*)malloc(1056512); if (!storage) Fail("model allocation");
		if (fread(storage,1056512,1,file) != 1 || fclose(file)) Fail("model read");
		for (unsigned i=0;i<264128;i++) if (!isfinite(storage[i])) Fail("nonfinite model weight");
		const float* cursor=storage;
		embedding=cursor; cursor+=Vocab*Dim;
		attentionNorm=cursor; cursor+=Layers*Dim;
		q=cursor; cursor+=Layers*Dim*Dim;
		k=cursor; cursor+=Layers*Dim*Kv;
		v=cursor; cursor+=Layers*Dim*Kv;
		o=cursor; cursor+=Layers*Dim*Dim;
		ffnNorm=cursor; cursor+=Layers*Dim;
		up=cursor; cursor+=Layers*Dim*Hidden;
		down=cursor; cursor+=Layers*Hidden*Dim;
		gate=cursor; cursor+=Layers*Dim*Hidden;
		finalNorm=cursor; cursor+=Dim+Seq*Head; // legacy rotary tables are unused
		if (cursor != storage+264128) Fail("weight layout bounds");
	}
};
struct State {
	float x[Dim], branch[Dim], projected[Dim], query[Dim], up[Hidden], gate[Hidden], logits[Vocab];
	float keys[Layers][Seq][Kv], values[Layers][Seq][Kv];
};
static void Norm(float* out,const float* x,const float* weight,int count)
{
	float squares=0; for(int i=0;i<count;i++) squares+=x[i]*x[i];
	float inverse=1.0f/sqrtf(squares/count+0.00001f);
	for(int i=0;i<count;i++) out[i]=x[i]*inverse*weight[i];
}
static void Gemv(PioneerTileBackend* device,float* out,const float* w,const float* x,int n,int d)
{
	if (n<1 || n>Hidden || d<1 || d>Vocab) Fail("unsupported projection shape");
	memset(out,0,d*sizeof(float));
	if (device == NULL) {
		for(int row=0;row<d;row++) for(int k=0;k<n;k++) out[row]+=w[row*n+k]*x[k];
		return;
	}
	for(int row=0;row<d;row+=16) for(int k=0;k<n;k+=16) {
		tt_pioneer_data_tile tile={}; tile.input.version=1;
		for(int inner=0;inner<16;inner++) {
			if(k+inner<n) tile.input.activations[inner]=PioneerTileBf16(x[k+inner]);
			for(int col=0;col<16;col++) if(row+col<d && k+inner<n)
				tile.input.weights[inner*16+col]=PioneerTileBf16(w[(row+col)*n+k+inner]);
		}
		if(!device->Run(tile)) Fail("accelerator projection failed; no CPU fallback");
		for(int col=0;col<16 && row+col<d;col++) {
			float value=PioneerTileFloat(tile.output[col]);
			if(!isfinite(value)) Fail("nonfinite accelerator projection");
			out[row+col]+=value;
		}
	}
}
static void Rotate(float* vector,int count,int position)
{
	for(int i=0;i<count;i+=2) {
		float angle=position/powf(10000.0f,float(i%Head)/Head),c=cosf(angle),s=sinf(angle);
		float first=vector[i],second=vector[i+1];
		vector[i]=first*c-second*s; vector[i+1]=first*s+second*c;
	}
}
static void Forward(const Model& m,State& s,int token,int position,PioneerTileBackend* device)
{
	if(token<0 || token>=Vocab || position<0 || position>=Seq) Fail("forward bounds");
	memcpy(s.x,m.embedding+token*Dim,sizeof(s.x));
	for(int layer=0;layer<Layers;layer++) {
		Norm(s.branch,s.x,m.attentionNorm+layer*Dim,Dim);
		Gemv(device,s.query,m.q+layer*Dim*Dim,s.branch,Dim,Dim);
		Gemv(device,s.keys[layer][position],m.k+layer*Dim*Kv,s.branch,Dim,Kv);
		Gemv(device,s.values[layer][position],m.v+layer*Dim*Kv,s.branch,Dim,Kv);
		Rotate(s.query,Dim,position); Rotate(s.keys[layer][position],Kv,position);
		for(int head=0;head<Heads;head++) {
			float scores[Seq],maximum=-INFINITY,total=0;
			int kvHead=head/(Heads/KvHeads);
			for(int time=0;time<=position;time++) {
				float dot=0;
				for(int i=0;i<Head;i++) dot+=s.query[head*Head+i]*s.keys[layer][time][kvHead*Head+i];
				scores[time]=dot/sqrtf(float(Head)); if(scores[time]>maximum) maximum=scores[time];
			}
			for(int time=0;time<=position;time++) { scores[time]=expf(scores[time]-maximum); total+=scores[time]; }
			for(int i=0;i<Head;i++) {
				float value=0;
				for(int time=0;time<=position;time++) value+=(scores[time]/total)*s.values[layer][time][kvHead*Head+i];
				s.branch[head*Head+i]=value;
			}
		}
		Gemv(device,s.projected,m.o+layer*Dim*Dim,s.branch,Dim,Dim);
		for(int i=0;i<Dim;i++) s.x[i]+=s.projected[i];
		Norm(s.branch,s.x,m.ffnNorm+layer*Dim,Dim);
		Gemv(device,s.up,m.up+layer*Dim*Hidden,s.branch,Dim,Hidden);
		Gemv(device,s.gate,m.gate+layer*Dim*Hidden,s.branch,Dim,Hidden);
		for(int i=0;i<Hidden;i++) s.up[i]=s.up[i]/(1.0f+expf(-s.up[i]))*s.gate[i];
		Gemv(device,s.projected,m.down+layer*Dim*Hidden,s.up,Hidden,Dim);
		for(int i=0;i<Dim;i++) s.x[i]+=s.projected[i];
		if(device) fprintf(stderr,"P100A token-position %d layer %d/%d complete, device tiles %llu\n",position,layer+1,Layers,(unsigned long long)device->calls);
	}
	Norm(s.branch,s.x,m.finalNorm,Dim);
	Gemv(device,s.logits,m.embedding,s.branch,Dim,Vocab);
}
static int Argmax(const float* x)
{
	int best=0;
	for(int i=0;i<Vocab;i++) { if(!isfinite(x[i])) Fail("nonfinite logits"); if(x[i]>x[best]) best=i; }
	return best;
}
static void LoadVocabulary(const char* path,char pieces[Vocab][128])
{
	FILE* file=fopen(path,"rb"); if(!file) Fail("cannot open tokenizer");
	uint32_t maximum;
	if(fread(&maximum,4,1,file)!=1 || maximum>=128) Fail("tokenizer header");
	for(int i=0;i<Vocab;i++) {
		float score; int32_t length;
		if(fread(&score,4,1,file)!=1 || fread(&length,4,1,file)!=1 || length<0 || length>=128
			|| fread(pieces[i],1,length,file)!=size_t(length)) Fail("tokenizer record");
		pieces[i][length]=0;
	}
	if(fgetc(file)!=EOF || fclose(file)) Fail("tokenizer length");
}
int main(int argc,char** argv)
{
	if(argc!=5 || strcmp(argv[1],"--run-p100a-with-cpu-reference")) {
		fprintf(stderr,"Usage: %s --run-p100a-with-cpu-reference PINNED_STORIES260K PINNED_TOK512 TOKENS_1_TO_32\n",argv[0]); return 2;
	}
	char* end=NULL; long count=strtol(argv[4],&end,10);
	if(!end || *end || count<1 || count>32) return 2;
	setvbuf(stdout,NULL,_IONBF,0); setvbuf(stderr,NULL,_IONBF,0);
	Model model; model.Load(argv[2]); char pieces[Vocab][128]; LoadVocabulary(argv[3],pieces);
	State* actual=(State*)calloc(1,sizeof(State)); State* reference=(State*)calloc(1,sizeof(State));
	if(!actual || !reference) Fail("state allocation");
	PioneerTileBackend backend; if(!backend.Open()) return 1;
	printf("TinyStories-260K: real P100A BF16 learned projections; host norm/attention/KV/activations. Independent CPU FP32 verification; no fallback.\n");
	int token=1; double started=Seconds();
	for(int position=0;position<count;position++) {
		Forward(model,*reference,token,position,NULL);
		Forward(model,*actual,token,position,&backend);
		int chosen=Argmax(actual->logits),expected=Argmax(reference->logits);
		double dot=0,a2=0,r2=0,diff2=0; float maxError=0;
		for(int i=0;i<Vocab;i++) {
			double a=actual->logits[i],r=reference->logits[i],delta=a-r;
			dot+=a*r; a2+=a*a; r2+=r*r; diff2+=delta*delta;
			if(fabs(delta)>maxError) maxError=fabs(delta);
		}
		double cosine=dot/sqrt(a2*r2),relative=sqrt(diff2/r2);
		printf("TOKEN %d id %d CPU-id %d cosine %.9f rel-L2 %.9f max-error %.6g tiles %llu elapsed %.3fs piece=",position,chosen,expected,cosine,relative,maxError,(unsigned long long)backend.calls,Seconds()-started);
		for(const unsigned char* p=(const unsigned char*)pieces[chosen];*p;p++)
			if(*p>=32 && *p!=127) putchar(*p); else printf("<%02x>",*p);
		putchar('\n');
		if(chosen!=expected || !isfinite(cosine) || cosine<0.999 || relative>0.02) Fail("end-to-end model reference mismatch");
		token=chosen; if(token==1 || token==2) break;
	}
	printf("PASS: pretrained model generated tokens with P100A learned projections and independent CPU agreement; %llu device tiles, %.3fs. Not Qwen3.8-27B or resident-GDDR inference.\n",(unsigned long long)backend.calls,Seconds()-started);
	free(actual); free(reference); return 0;
}
