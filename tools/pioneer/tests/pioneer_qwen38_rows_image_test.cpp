/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_STREAM_MACHINE_ONLY
#include "pioneer_qwen38_stream_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RowsProbeImage.h"
struct RowsMachine:StreamMachine {
	unsigned groups;
	RowsMachine(const char* root,unsigned type,unsigned inputs,unsigned rows)
		:StreamMachine(root,type,inputs),groups(rows) {
#ifndef PIONEER_Q38_BINARY_SCALE_IMAGE
		for (unsigned w=0;w<2048;w++) assert(PioneerQ38RowsWord(w)==(w<code.size()/4?Word(code,w*4):0));
#endif
		memory[0x74000]=0x31523851; memory[0x7401c]=groups;
		auto initial=tiles;
		for (unsigned group=1;group<groups;group++) for (auto tile:initial) {
			for (unsigned byte=0;byte<tile.size();byte++) {
				unsigned raw=byte/16;
				if ((type==8 && raw>=2 && raw<34) || (type==12 && raw>=16) || (type==14 && raw<208)) tile[byte]^=uint8_t(group*23);
			}
			tiles.push_back(tile);
		}
	}
	unsigned StepLimit() const override { return 1000000000; }
	uint32_t Load(uint32_t a) override { if (a==0x7401c) return memory.at(a); return StreamMachine::Load(a); }
	void Store(uint32_t a,uint32_t v) override {
		if (Range(a,0x84000,16384)) { assert(drained && !(a&3) && Range(a,0x84000,groups*64)); buffers[4096+(a-0x84000)/4]=v; return; }
		StreamMachine::Store(a,v);
	}
	bool RunRows() {
		while (pc!=0x72038 && pc!=0x72058) StepBlock();
		assert(minStack>=0x6e000);
		if (pc==0x72058) { assert(l1[Index(0x17ffc8)]==0x46523851); return false; }
		assert(l1[Index(0x17ffc0)]==0x7200c && l1[Index(0x17ffc8)]==0x31523851);
		return l1[Index(0x17ffc4)]==groups*(inputs/16) && requests==tiles.size() && responses==requests && !pending;
	}
	bool CorrectRows() const {
		if (matrixInstructions!=4*groups*(inputs/16)) return false;
		unsigned blocks=inputs/count;
		for (unsigned group=0;group<groups;group++) {
			float expected[16]={}; double reference[16]={},absolute[16]={};
			for (unsigned b=0;b<blocks;b++) {
				const auto& tile=tiles[group*blocks+b];
				for (unsigned p=0;p<count/16;p++) {
					PioneerQ38Panel panel; assert(PioneerQ38UnpackPanel(type,tile.data(),unsigned(tile.size()),p,&panel));
					int inputSum=0;
					for (unsigned k=0;k<16;k++) { auto a=vector[b*count+p*16+k]; inputSum+=int(a&1023)*((a&0x8000)?-1:1); }
					for (unsigned row=0;row<16;row++) {
						int dot=0;
						for (unsigned k=0;k<16;k++) {
							unsigned e=k*16+row; uint16_t w=uint16_t(panel.operand[e/2]>>((e%2)*16)),a=vector[b*count+p*16+k];
							dot+=int(w&1023)*((w&0x8000)?-1:1)*int(a&1023)*((a&0x8000)?-1:1);
						}
						float product=(Float(panel.coefficient[row])*Float(scales[b]))*float(dot),correction=(Float(panel.minimum[row])*Float(scales[b]))*float(inputSum);
						expected[row]+=product+correction;
					}
				}
				for (unsigned row=0;row<16;row++) {
					uint8_t raw[210]={}; unsigned bytes=PioneerQ38BlockBytes(type);
					for (unsigned byte=0;byte<bytes;byte++) raw[byte]=tile[byte*16+row];
					float decoded[256]={}; assert(type==8?PioneerDecodeQ8_0(raw,decoded):type==12?PioneerDecodeQ4K(raw,decoded):PioneerDecodeQ6K(raw,decoded));
					for (unsigned k=0;k<count;k++) { auto a=vector[b*count+k]; double v=double(decoded[k])*int(a&1023)*((a&0x8000)?-1:1)*Float(scales[b]); reference[row]+=v; absolute[row]+=fabs(v); }
				}
			}
			for (unsigned row=0;row<16;row++) if (buffers[4096+group*16+row]!=Bits(expected[row]) || fabs(double(expected[row])-reference[row])>1e-6+absolute[row]*2e-6) return false;
		}
		for (unsigned row=groups*16;row<4096;row++) if (buffers[4096+row]!=0x7fc00000) return false;
		return true;
	}
	void BindRows(PioneerQwen38Gguf& model,unsigned tensor,unsigned firstRow) {
		const auto& t=model.Tensors().at(tensor); assert(t.type==type && t.inputs==inputs && firstRow+groups*16<=t.outputs);
		unsigned block=PioneerQ38BlockBytes(type),rowBytes=unsigned(t.bytes/t.outputs),blocks=inputs/count;
		for (unsigned group=0;group<groups;group++) for (unsigned b=0;b<blocks;b++) {
			auto& tile=tiles[group*blocks+b]; std::fill(tile.begin(),tile.end(),0);
			for (unsigned row=0;row<16;row++) {
				uint8_t raw[210]={}; assert(model.ReadPacked(tensor,uint64_t(firstRow+group*16+row)*rowBytes+b*block,block,raw,block));
				for (unsigned byte=0;byte<block;byte++) tile[byte*16+row]=raw[byte];
			}
		}
	}
};
int main(int argc,char** argv)
{
	assert(argc==2 || argc==3);
	for (unsigned type:{8U,12U,14U}) for (unsigned groups:{1U,3U,16U}) {
		RowsMachine m(argv[1],type,type==12?17408:5120,groups); m.delay=3; auto before=m.memory;
		assert(m.RunRows() && m.CorrectRows());
		for (auto word:before) if (RowsMachine::Range(word.first,0x94000,4096) || RowsMachine::Range(word.first,0xa0000,m.inputs*2) || RowsMachine::Range(word.first,0x74000,32)) assert(m.memory.at(word.first)==word.second);
		printf("OFFLINE multiple-row type%u K%u N%u steps%u reads%u PASS\n",type,m.inputs,groups*16,m.steps,m.requests);
	}
	for (unsigned fault:{1U,21U,60U}) {
		RowsMachine m(argv[1],12,5120,3); m.faultAt=fault;
		assert(!m.RunRows() && m.pending && m.requests==fault && m.responses==fault-1 && m.matrixInstructions==(fault-1)*64);
	}
	for (unsigned bad:{0U,17U}) {
		RowsMachine m(argv[1],12,5120,3); m.memory[0x7401c]=bad;
		assert(!m.RunRows() && !m.requests && !m.matrixInstructions);
	}
	unsigned real=0;
	if (argc==3) {
		FILE* file=fopen(argv[2],"rb"); assert(file); PioneerQwen38Gguf model; assert(model.Open(file));
		for (const char* name:{"blk.0.attn_qkv.weight","blk.0.ffn_down.weight","output.weight"}) {
			const auto* t=model.Find(name); assert(t);
			for (unsigned end=0;end<2;end++) {
				RowsMachine m(argv[1],t->type,unsigned(t->inputs),3);
				m.BindRows(model,unsigned(t-model.Tensors().data()),end?unsigned(t->outputs)-48:0);
				assert(m.RunRows() && m.CorrectRows() && model.Stable()); real++;
				printf("OFFLINE real multiple-row %s K%u N48 end%u PASS\n",name,m.inputs,end);
			}
		}
		assert(model.Stable()); fclose(file);
	}
	printf("OFFLINE real multiple-row cases=%u\n",real);
	puts("PASS OFFLINE actual mixed-quant fullK/multiple16-row groups through N256,no host intermediates,exact FP32+independent double references,immutable inputs/scales,bounds/canaries and cross-group UNKNOWN parks;NOT installed/native/multiworker/graph/inference/speed");
}
