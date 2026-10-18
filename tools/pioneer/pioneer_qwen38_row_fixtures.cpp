/* SPDX-License-Identifier: MIT */
// Read-only fixed public model data extractor;fresh O_EXCL file ONLY.
// Caller authenticates whole model SHA before running this OFFLINE utility.
#include "PioneerQwen38Gguf.h"
#include "PioneerQwen38Panel.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char** argv)
{
	if (argc!=3) return 2;
	FILE* source=fopen(argv[1],"rb"); if (!source) return 1;
	PioneerQwen38Gguf model; if (!model.Open(source)) { fclose(source); return 1; }
	std::vector<uint8_t> output;
	for (const char* name:{"blk.0.attn_qkv.weight","token_embd.weight","output.weight","blk.0.ffn_down.weight"}) {
		const auto* t=model.Find(name); if (!t || !t->type || t->outputs<16) { fclose(source); return 1; }
		unsigned index=unsigned(t-model.Tensors().data()),rowBytes=unsigned(t->bytes/t->outputs);
		unsigned block=PioneerQ38BlockBytes(t->type),tileBytes=PioneerQ38PanelTileBytes(t->type),columns=t->type==8?32:256;
		if (!block || t->inputs%columns) { fclose(source); return 1; }
		for (unsigned end=0;end<2;end++) {
			unsigned first=end?unsigned(t->outputs)-16:0;
			for (unsigned b=0;b<t->inputs/columns;b++) {
				alignas(4) uint8_t tile[3392]={};
				for (unsigned row=0;row<16;row++) {
					uint8_t raw[210]={};
					if (!model.ReadPacked(index,uint64_t(first+row)*rowBytes+b*block,block,raw,block)) { fclose(source); return 1; }
					for (unsigned byte=0;byte<block;byte++) tile[byte*16+row]=raw[byte];
				}
				output.insert(output.end(),tile,tile+tileBytes);
			}
			printf("DATA_ONLY %s type%u K%llu end%u\n",name,t->type,(unsigned long long)t->inputs,end);
		}
	}
	bool okay=model.Stable() && output.size()==725504; if (fclose(source)) okay=false;
	if (!okay) return 1;
	int fd=open(argv[2],O_WRONLY|O_CREAT|O_EXCL,0600); if (fd<0) return 1;
	unsigned at=0;
	while (at<output.size()) { ssize_t n=write(fd,output.data()+at,output.size()-at); if (n<=0) { okay=false; break; } at+=unsigned(n); }
	if (fsync(fd) || close(fd)) okay=false;
	return okay?0:1;
}
