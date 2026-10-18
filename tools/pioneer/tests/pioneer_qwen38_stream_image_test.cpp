/* SPDX-License-Identifier: MIT */
// Execute the actual OFFLINE full-K image. Synthetic NoC fabric ONLY;
// no installed driver/card/host-DMA/PCI/firmware access.
#define PIONEER_Q38_BLOCK_MACHINE_ONLY
#include "pioneer_qwen38_block_image_test.cpp"
#ifdef PIONEER_Q38_STREAM_PINNED
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38StreamProbeImage.h"
#endif
struct StreamMachine:BlockMachine {
	std::vector<std::vector<uint8_t>> tiles;
	std::vector<uint32_t> scales;
	bool targetRead=false;
	bool badCounter=false;
	StreamMachine(const char* root,unsigned kind,unsigned k,unsigned trial=1)
		:BlockMachine(root,kind,trial,.012345f,nullptr,true) {
#ifdef PIONEER_Q38_STREAM_PINNED
		for (unsigned w=0;w<2048;w++) assert(PioneerQ38StreamWord(w)==(w<code.size()/4?Word(code,w*4):0));
#endif
		inputs=k; vector.resize(k); unsigned blocks=k/count;
		memory[0x74000]=0x31533851; memory[0x74010]=0;
		memory[0x74014]=k; memory[0x74018]=base;
		for (unsigned b=0;b<blocks;b++) {
			auto next=tile;
			for (unsigned byte=0;byte<next.size();byte++) {
				unsigned raw=byte/16;
				if ((type==8 && raw>=2 && raw<34) || (type==12 && raw>=16) || (type==14 && raw<208)) next[byte]^=uint8_t(b*13);
			}
			tiles.push_back(next);
			scales.push_back(Bits(b%3?-.03125f:.012345f)); memory[0x94000+b*4]=scales.back();
		}
		for (unsigned w=0;w<k/2;w++) {
			uint32_t value=0;
			for (unsigned lane=0;lane<2;lane++) { unsigned at=w*2+lane; vector[at]=PioneerInt8Operand(int((at*29+trial*17)%256)-128); value|=uint32_t(vector[at])<<(lane*16); }
			memory[0xa0000+w*4]=value;
		}
	}
	unsigned StepLimit() const override { return 100000000; }
	uint32_t Load(uint32_t a) override {
		if (a==0xffb20000) { targetRead=true; return target; }
		if (a==0xffb20040) {
			doorbellRead=true;
			if (pending && ++waits>delay && requests!=faultAt) {
				for (unsigned w=0;w<tile.size()/4;w++) memory[0x90000+w*4]=Word(tiles.at(requests-1),w*4)^((corrupt && w==64)?0x01010101U:0);
				pending=false; responses++;
			}
			return pending?1:0;
		}
		if (a==0xffb20208) { assert(doorbellRead); return responses+(badCounter && responses?1U:0U); }
		if (a==0xffb20240) { assert(doorbellRead); return pending?1:0; }
		if (Range(a,0x74000,28) || Range(a,0x94000,unsigned(scales.size()*4)) || Range(a,0xa0000,inputs*2)) {
			assert(!(a&3) && memory.count(a)); return memory.at(a);
		}
		return BlockMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (a==0xffb20000) {
			assert(drained && !pending && requests<tiles.size() && value==base+requests*tile.size());
			target=value; targetRead=false; return;
		}
		if (a==0xffb20040) {
			assert(value==1 && drained && !pending && targetRead && requests<tiles.size());
			requests++; pending=true; doorbellRead=false; waits=0; return;
		}
		BlockMachine::Store(a,value);
	}
	bool RunStream() {
		while (pc!=0x72038 && pc!=0x72058) StepBlock();
		assert(minStack>=0x6e000);
		if (pc==0x72058) { assert(l1[Index(0x17ffc8)]==0x46533851); return false; }
		assert(l1[Index(0x17ffc0)]==0x7200c && l1[Index(0x17ffc8)]==0x31533851);
		return l1[Index(0x17ffc4)]==inputs/16 && requests==tiles.size() && responses==requests && !pending;
	}
	bool CorrectStream() const {
		if (matrixInstructions!=4*(inputs/16)) return false;
		float expected[16]={}; double reference[16]={},absolute[16]={};
		for (unsigned b=0;b<tiles.size();b++) {
			for (unsigned p=0;p<count/16;p++) {
				PioneerQ38Panel panel; assert(PioneerQ38UnpackPanel(type,tiles[b].data(),unsigned(tile.size()),p,&panel));
				int inputSum=0;
				for (unsigned k=0;k<16;k++) { unsigned at=b*count+p*16+k; auto a=vector[at]; inputSum+=int(a&1023)*((a&0x8000)?-1:1); }
				for (unsigned row=0;row<16;row++) {
					int dot=0;
					for (unsigned k=0;k<16;k++) {
						unsigned e=k*16+row,at=b*count+p*16+k; uint16_t w=uint16_t(panel.operand[e/2]>>((e%2)*16)),a=vector[at];
						dot+=int(w&1023)*((w&0x8000)?-1:1)*int(a&1023)*((a&0x8000)?-1:1);
					}
					float product=(Float(panel.coefficient[row])*Float(scales[b]))*float(dot),correction=(Float(panel.minimum[row])*Float(scales[b]))*float(inputSum);
					expected[row]+=product+correction;
				}
			}
			for (unsigned row=0;row<16;row++) {
				uint8_t raw[210]={}; unsigned bytes=PioneerQ38BlockBytes(type);
				for (unsigned byte=0;byte<bytes;byte++) raw[byte]=tiles[b][byte*16+row];
				float decoded[256]={}; assert(type==8?PioneerDecodeQ8_0(raw,decoded):type==12?PioneerDecodeQ4K(raw,decoded):PioneerDecodeQ6K(raw,decoded));
				for (unsigned k=0;k<count;k++) {
					auto a=vector[b*count+k]; double product=double(decoded[k])*int(a&1023)*((a&0x8000)?-1:1)*Float(scales[b]);
					reference[row]+=product; absolute[row]+=fabs(product);
				}
			}
		}
		for (unsigned row=0;row<16;row++) if (buffers[4096+row]!=Bits(expected[row]) || fabs(double(expected[row])-reference[row])>1e-6+absolute[row]*2e-6) return false;
		return true;
	}
	void BindModel(PioneerQwen38Gguf& model,unsigned tensor,unsigned firstRow) {
		const auto& t=model.Tensors().at(tensor); assert(t.type==type && t.inputs==inputs && firstRow+16<=t.outputs);
		unsigned block=PioneerQ38BlockBytes(type),rowBytes=unsigned(t.bytes/t.outputs);
		for (unsigned b=0;b<tiles.size();b++) {
			std::fill(tiles[b].begin(),tiles[b].end(),0);
			for (unsigned row=0;row<16;row++) {
				uint8_t raw[210]={}; assert(model.ReadPacked(tensor,uint64_t(firstRow+row)*rowBytes+b*block,block,raw,block));
				for (unsigned byte=0;byte<block;byte++) tiles[b][byte*16+row]=raw[byte];
			}
		}
	}
};
#ifndef PIONEER_Q38_STREAM_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2 || argc==3);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		StreamMachine m(argv[1],type,k); m.delay=3; auto before=m.memory;
		assert(m.RunStream() && m.CorrectStream());
		for (auto word:before) if (StreamMachine::Range(word.first,0x94000,4096) || StreamMachine::Range(word.first,0xa0000,k*2) || StreamMachine::Range(word.first,0x74000,28)) assert(m.memory.at(word.first)==word.second);
		printf("OFFLINE fullK type%u K%u steps%u reads%u FP32+double references PASS\n",type,k,m.steps,m.requests);
	}
	for (unsigned fault:{1U,10U,20U}) {
		StreamMachine m(argv[1],12,5120); m.faultAt=fault;
		assert(!m.RunStream() && m.pending && m.requests==fault && m.responses==fault-1 && m.matrixInstructions==(fault-1)*64);
	}
	for (unsigned fault=0;fault<3;fault++) {
		StreamMachine m(argv[1],12,5120); m.dropMatrix=fault==0; m.dropClear=fault==1; m.corrupt=fault==2;
		assert(!m.RunStream() || !m.CorrectStream());
	}
	for (unsigned at:{0U,10U,19U}) {
		StreamMachine m(argv[1],12,5120); m.memory[0x94000+at*4]=0x7f800000; assert(!m.RunStream() && !m.requests && !m.matrixInstructions);
	}
	{
		StreamMachine m(argv[1],12,5120); m.badCounter=true;
		assert(!m.RunStream() && m.requests==1 && m.responses==1 && !m.matrixInstructions);
	}
	unsigned real=0;
	if (argc==3) {
		FILE* file=fopen(argv[2],"rb"); assert(file); PioneerQwen38Gguf model; assert(model.Open(file));
		for (const char* name:{"blk.0.attn_qkv.weight","token_embd.weight","output.weight","blk.0.ffn_down.weight"}) {
			const auto* t=model.Find(name); assert(t);
			for (unsigned end=0;end<2;end++) {
				StreamMachine m(argv[1],t->type,unsigned(t->inputs)); m.delay=3;
				m.BindModel(model,unsigned(t-model.Tensors().data()),end?unsigned(t->outputs)-16:0);
				assert(m.RunStream() && m.CorrectStream() && model.Stable()); real++;
				printf("OFFLINE real fullK %s K%u end%u steps%u PASS\n",name,m.inputs,end,m.steps);
			}
		}
		assert(model.Stable()); fclose(file);
	}
	printf("OFFLINE real fullK groups=%u\n",real);
	puts("PASS OFFLINE actual fullK mixed-quant stream image,9 shapes,exact FP32+independent double references,immutable vector/scales/descriptor,bounded one-block reads and first/middle/last UNKNOWN parks WITHOUT retry;NOT installed/native/whole graph/inference/speed");
}
#endif
