/* SPDX-License-Identifier: MIT */
// Build SIX fixed PUBLIC model-fragment DATA fixtures after external whole
// source SHA authentication. Not inference,code generation or GPU access.
// Output must be fresh; no symlink/overwrite. Native consumers must check
// the exact whole fixture SHA/size before using ANY byte.
#include "PioneerQwen38Gguf.h"
#include "PioneerQwen38Panel.h"
#include <fcntl.h>
#include <unistd.h>
int main(int argc,char** argv)
{
	if (argc!=3) return 2;
	FILE* source=fopen(argv[1],"rb"); if (!source) return 1;
	PioneerQwen38Gguf model; if (!model.Open(source)) { fclose(source); return 1; }
	int fd=open(argv[2],O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
	if (fd<0) { fclose(source); return 1; }
	FILE* output=fdopen(fd,"wb"); if (!output) { close(fd); fclose(source); return 1; }
	bool okay=true; unsigned total=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned end=0;end<2 && okay;end++) {
		const char* name=type==8?"blk.0.attn_qkv.weight":type==12?"token_embd.weight":"output.weight";
		const auto* tensor=model.Find(name); if (!tensor || tensor->type!=type) { okay=false; break; }
		unsigned index=unsigned(tensor-model.Tensors().data()),block=PioneerQ38BlockBytes(type),bytes=PioneerQ38PanelTileBytes(type);
		unsigned rowBytes=unsigned(tensor->bytes/tensor->outputs),first=end?unsigned(tensor->outputs)-16:0;
		uint8_t tile[3392]={},raw[210]={};
		for (unsigned row=0;row<16 && okay;row++) {
			okay=model.ReadPacked(index,uint64_t(first+row)*rowBytes+(end?rowBytes-block:0),block,raw,block);
			for (unsigned byte=0;byte<block;byte++) tile[byte*16+row]=raw[byte];
		}
		if (okay) { okay=fwrite(tile,1,bytes,output)==bytes; total+=bytes; }
	}
	okay=okay && model.Stable() && total==12544 && !fflush(output) && !fsync(fileno(output));
	if (fclose(output)) okay=false;
	if (fclose(source)) okay=false;
	if (!okay) { fprintf(stderr,"STOP:generated fixture incomplete;DO_NOT_USE\n"); return 1; }
	printf("PUBLIC_PANEL_FIXTURES_DATA_ONLY bytes=%u;REQUIRE_WHOLE_FIXTURE_SHA_BEFORE_DEVICE_OPEN\n",total); return 0;
}
