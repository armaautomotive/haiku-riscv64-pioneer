/* SPDX-License-Identifier: MIT */
#include "TenstorrentHaikuTile.h"
#include "TenstorrentGgufQuant.h"
#include <stdlib.h>
#include <time.h>
enum { MaxInputs=6144, Outputs=16, MaxSampleBytes=87040 };
int main(int argc,char** argv)
{
	bool legacy=argc==3 && strcmp(argv[1],"--run-qwen-q4-projection-and-reset")==0;
	if(!legacy && (argc!=4 || strcmp(argv[1],"--run-qwen-projection-and-reset"))) {
		fprintf(stderr,"Usage: %s --run-qwen-projection-and-reset Q4_K|Q8_0|Q6_K PINNED_SAMPLE\nNot full-model inference: 16 real weight rows, synthetic activation, host block decode, device BF16 products.\n",argv[0]); return 2;
	}
	const char* format=legacy?"Q4_K":argv[2]; const char* path=legacy?argv[2]:argv[3];
	unsigned inputs=5120,blockElements=256,blockBytes=144;
	if(strcmp(format,"Q8_0")==0) { blockElements=32; blockBytes=34; }
	else if(strcmp(format,"Q6_K")==0) { inputs=6144; blockBytes=210; }
	else if(strcmp(format,"Q4_K")!=0) return 2;
	unsigned blocks=inputs/blockElements,sampleBytes=Outputs*blocks*blockBytes;
	setvbuf(stdout,NULL,_IONBF,0);
	FILE* source=fopen(path,"rb"); if(!source) return 1;
	uint8_t sample[MaxSampleBytes];
	if(fread(sample,sampleBytes,1,source)!=1 || fgetc(source)!=EOF || fclose(source)) return 1;
	float* weights=(float*)malloc(inputs*Outputs*sizeof(float)); if(!weights) return 1;
	for(unsigned row=0;row<Outputs;row++) for(unsigned block=0;block<blocks;block++) {
		const uint8_t* data=sample+(row*blocks+block)*blockBytes; float* out=weights+row*inputs+block*blockElements;
		bool valid=strcmp(format,"Q4_K")==0?PioneerDecodeQ4K(data,out):strcmp(format,"Q8_0")==0?PioneerDecodeQ8_0(data,out):PioneerDecodeQ6K(data,out);
		if(!valid) { free(weights); return 1; }
	}
	float activation[MaxInputs],reference[Outputs]={},bf16[Outputs]={},sumAbsolute[Outputs]={},actual[Outputs]={};
	for(unsigned k=0;k<inputs;k++) activation[k]=float(int((k*17)%257)-128)/256.0f;
	for(unsigned row=0;row<Outputs;row++) for(unsigned k=0;k<inputs;k++) {
		float w=weights[row*inputs+k],x=activation[k];
		reference[row]+=w*x;
		float product=PioneerTileFloat(uint32_t(PioneerTileBf16(w))<<16)*PioneerTileFloat(uint32_t(PioneerTileBf16(x))<<16);
		bf16[row]+=product; sumAbsolute[row]+=fabsf(product);
	}
	PioneerTileBackend device; if(!device.Open()) { free(weights); return 1; }
	struct timespec start,end; clock_gettime(CLOCK_MONOTONIC,&start);
	for(unsigned base=0;base<inputs;base+=256) {
		tt_tensix_tile_data input[16]={}; uint32_t output[16][256];
		for(unsigned job=0;job<16;job++) {
			input[job].version=1;
			for(unsigned k=0;k<16;k++) {
				input[job].activations[k]=PioneerTileBf16(activation[base+job*16+k]);
				for(unsigned row=0;row<Outputs;row++) input[job].weights[k*16+row]=PioneerTileBf16(weights[row*inputs+base+job*16+k]);
			}
		}
		if(!device.RunBatch(input,output,16)) { free(weights); return 1; }
		for(unsigned job=0;job<16;job++) for(unsigned row=0;row<Outputs;row++) actual[row]+=PioneerTileFloat(output[job][row]);
	}
	clock_gettime(CLOCK_MONOTONIC,&end);
	double dot=0,a2=0,r2=0,delta2=0; unsigned failures=0;
	for(unsigned row=0;row<Outputs;row++) {
		float error=fabsf(actual[row]-bf16[row]);
		if(!isfinite(actual[row]) || error>0.000001f+sumAbsolute[row]/1024.0f) failures++;
		dot+=double(actual[row])*reference[row]; a2+=double(actual[row])*actual[row]; r2+=double(reference[row])*reference[row];
		double difference=actual[row]-reference[row]; delta2+=difference*difference;
		printf("Qwen %s row %u P100A %.9g FP32-reference %.9g BF16-reference %.9g normalized-device-error %.9g\n",format,row,actual[row],reference[row],bf16[row],error/sumAbsolute[row]);
	}
	double cosine=dot/sqrt(a2*r2),relative=sqrt(delta2/r2);
	double seconds=end.tv_sec-start.tv_sec+(end.tv_nsec-start.tv_nsec)*1e-9;
	printf("Qwen %s real-weight projection: cosine %.9f relative-L2 %.9f failures %u tiles %llu transactions %llu time %.3fs\n",format,cosine,relative,failures,(unsigned long long)device.calls,(unsigned long long)device.transactions,seconds);
	free(weights);
	if(failures || !isfinite(cosine) || cosine<0.9999 || relative>0.01) return 1;
	printf("PASS: real Qwen3.8-27B %s projection weights, host block decode and P100A multiplication; NOT full Qwen inference or resident GDDR weights\n",format);
	return 0;
}
