/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen38Panel.h"
#include "../PioneerQwen38Gguf.h"
#include <cassert>
int main(int argc,char** argv)
{
	assert(argc==3); FILE* source=fopen(argv[1],"rb"),*fixtures=fopen(argv[2],"rb"); assert(source && fixtures);
	PioneerQwen38Gguf model; assert(model.Open(source));
	struct stat identity={}; assert(!fstat(fileno(fixtures),&identity) && identity.st_size==725504);
	unsigned total=0,checked=0,groups=0;
	for (const char* name:{"blk.0.attn_qkv.weight","token_embd.weight","output.weight","blk.0.ffn_down.weight"}) {
		const auto* t=model.Find(name); assert(t); unsigned columns=t->type==8?32:256,bytes=PioneerQ38PanelTileBytes(t->type),block=PioneerQ38BlockBytes(t->type);
		unsigned index=unsigned(t-model.Tensors().data());
		for (unsigned end=0;end<2;end++) {
			std::vector<std::vector<float>> full(16,std::vector<float>(size_t(t->inputs)));
			for (unsigned row=0;row<16;row++) assert(model.ReadRow(index,(end?unsigned(t->outputs)-16:0)+row,full[row].data(),unsigned(full[row].size())));
			for (unsigned b=0;b<t->inputs/columns;b++) {
				alignas(4) uint8_t tile[3392]={}; assert(fread(tile,1,bytes,fixtures)==bytes); total+=bytes;
				for (unsigned row=0;row<16;row++) {
					uint8_t raw[210]={}; assert(model.ReadPacked(index,uint64_t((end?unsigned(t->outputs)-16:0)+row)*(t->bytes/t->outputs)+b*block,block,raw,block));
					for (unsigned byte=0;byte<block;byte++) assert(tile[byte*16+row]==raw[byte]);
				}
				for (unsigned p=0;p<columns/16;p++) {
					PioneerQ38Panel panel; assert(PioneerQ38UnpackPanel(t->type,tile,bytes,p,&panel));
					for (unsigned row=0;row<16;row++) for (unsigned k=0;k<16;k++) {
						unsigned e=k*16+row; uint16_t w=uint16_t(panel.operand[e/2]>>((e%2)*16)); int q=int(w&1023)*((w&0x8000)?-1:1);
						float value=PioneerAttentionFloat(panel.coefficient[row])*q+PioneerAttentionFloat(panel.minimum[row]);
						assert(value==full[row][b*columns+p*16+k]); checked++;
					}
				}
			}
			groups++;
		}
	}
	assert(total==725504 && checked==1048576 && groups==8 && fgetc(fixtures)==EOF && !ferror(fixtures) && model.Stable());
	fclose(fixtures); fclose(source);
	puts("PASS OFFLINE:all725504 full-row fixture bytes/eight first-last source groups/1048576 decoded values match independent FULL GGUF rows;DATA ONLY");
}
