/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen38Panel.h"
#include "../PioneerQwen38Gguf.h"
#include <cassert>
int main(int argc,char** argv)
{
	assert(argc==3); FILE* source=fopen(argv[1],"rb"),*fixtures=fopen(argv[2],"rb"); assert(source && fixtures);
	PioneerQwen38Gguf model; assert(model.Open(source));
	struct stat identity={}; assert(!fstat(fileno(fixtures),&identity) && identity.st_size==12544);
	unsigned checked=0,total=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned end=0;end<2;end++) {
		const char* name=type==8?"blk.0.attn_qkv.weight":type==12?"token_embd.weight":"output.weight";
		const auto* tensor=model.Find(name); assert(tensor && tensor->type==type);
		unsigned index=unsigned(tensor-model.Tensors().data()),bytes=PioneerQ38PanelTileBytes(type),count=type==8?32:256;
		uint8_t tile[3392]={}; assert(fread(tile,1,bytes,fixtures)==bytes); total+=bytes;
		for (unsigned row=0;row<16;row++) {
			float values[5120]; assert(model.ReadRow(index,end?unsigned(tensor->outputs)-16+row:row,values,5120));
			for (unsigned p=0;p<count/16;p++) {
				PioneerQ38Panel panel; assert(PioneerQ38UnpackPanel(type,tile,bytes,p,&panel));
				for (unsigned k=0;k<16;k++) {
					unsigned element=k*16+row; uint16_t word=uint16_t(panel.operand[element/2]>>((element%2)*16));
					int q=int(word&1023)*((word&0x8000)?-1:1);
					float value=PioneerAttentionFloat(panel.coefficient[row])*q+PioneerAttentionFloat(panel.minimum[row]);
					assert(value==values[(end?5120-count:0)+p*16+k]); checked++;
				}
			}
		}
	}
	assert(total==12544 && checked==17408 && fgetc(fixtures)==EOF && !ferror(fixtures) && model.Stable());
	fclose(fixtures); fclose(source);
	puts("PASS: all12544 public fixture bytes/six exact first-last source extents/17408 dequantized values match independent FULL GGUF rows;OFFLINE data only");
}
