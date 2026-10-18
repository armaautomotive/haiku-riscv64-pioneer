/* SPDX-License-Identifier: MIT */
// Actual candidate RV32 instructions;strict synthetic read-only NoC fabric.
// NO card/driver/PCI access. Root inputs are NEVER writable by this image.
#define PIONEER_Q38_SHARED_ROWS_MACHINE_ONLY
#include "pioneer_qwen38_rows_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CohortImage.h"
struct SharedRowsMachine:RowsMachine {
	struct Packet { uint32_t address,route,destination,bytes; int tile; };
	std::vector<Packet> packets;
	std::map<uint32_t,uint32_t> root;
	uint32_t command[10]={}; bool readback[10]={};
	unsigned inputReads=0; bool peer; uint32_t counterBase=0; bool badReadback=false;
	SharedRowsMachine(const char* path,unsigned type,unsigned k,unsigned groups,bool shared=true)
		:RowsMachine(path,type,k,groups),peer(shared) {
#ifndef PIONEER_Q38_PERSISTENT_IMAGE
		for (unsigned w=0;w<2048;w++) assert(PioneerQ38CohortWord(w)==(w<code.size()/4?Word(code,w*4):0));
#endif
		memory[0x74000]=0x31483851; memory[0x74020]=0x47; memory[0x74024]=peer?1:0;
		// Non-root self XY,including the harvested logical column7. The image
		// must neither guess physical coordinates nor change its destination.
		command[5]=7|(11<<6);
		for (unsigned w=0;w<k/2;w++) {
			root[0xa0000+w*4]=memory.at(0xa0000+w*4);
			if (peer) memory[0xa0000+w*4]=0xdeadbeef;
		}
		unsigned padded=(unsigned(scales.size()*4)+63)&~63U;
		for (unsigned at=0;at<padded;at+=4) {
			root[0x94000+at]=at/4<scales.size()?scales[at/4]:0;
			if (peer) memory[0x94000+at]=0x7fc00000;
		}
		if (peer) {
			for (unsigned at=0;at<k*2;) {
				unsigned bytes=std::min(16384U,k*2-at);
				packets.push_back({0xa0000+at,1|(2<<6),0xa0000+at,bytes,-1}); at+=bytes;
			}
			packets.push_back({0x94000,1|(2<<6),0x94000,padded,-1});
		}
		inputReads=unsigned(packets.size());
		for (unsigned t=0;t<tiles.size();t++) packets.push_back({base+t*unsigned(tile.size()),0x47,0x90000,unsigned(tile.size()),int(t)});
	}
	uint32_t Load(uint32_t a) override {
		if (a>=0xffb20000 && a<0xffb20028) {
			unsigned w=(a-0xffb20000)/4; assert(!(a&3)); readback[w]=true; return command[w]^(badReadback && w==2?1U:0U);
		}
		if (a==0xffb20040) {
			doorbellRead=true;
			if (pending && ++waits>delay && requests!=faultAt) {
				const auto& p=packets.at(requests-1);
				for (unsigned at=0;at<p.bytes;at+=4) {
					uint32_t value=p.tile<0?root.at(p.address+at):Word(tiles.at(unsigned(p.tile)),at);
					memory[p.destination+at]=value^((corrupt && at==256)?0x01010101U:0);
				}
				pending=false; responses++;
			}
			return pending?1:0;
		}
		if (a==0xffb20208) { assert(doorbellRead); return counterBase+responses+(badCounter && responses?1U:0U); }
		if (a==0xffb20240) { assert(doorbellRead); return pending?1:0; }
		if (Range(a,0x74000,40)) return memory.at(a);
		return RowsMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t v) override {
		if (a>=0xffb20000 && a<0xffb20028) {
			unsigned w=(a-0xffb20000)/4; assert(!(a&3) && drained && !pending);
			assert(w==0 || w==2 || w==3 || w==8); // no upper/control/XY/WRITE capability
			assert(requests<packets.size()); const auto& p=packets.at(requests);
			assert(v==(w==0?p.address:w==2?p.route:w==3?p.destination:p.bytes));
			command[w]=v; readback[w]=false; return;
		}
		if (a==0xffb20040) {
			assert(v==1 && drained && !pending && requests<packets.size());
			const auto& p=packets.at(requests);
			assert(command[0]==p.address && command[2]==p.route && command[3]==p.destination && command[8]==p.bytes);
			assert(!command[1] && !command[4] && command[5]==(7|(11<<6)) && !command[6] && !command[7] && !command[9]);
			assert(readback[0] && readback[2] && readback[3] && readback[8]);
			assert(!(p.bytes&63) && p.bytes<=16384);
			requests++; pending=true; waits=0; doorbellRead=false; return;
		}
		RowsMachine::Store(a,v); // strict old memory/MMIO whitelist,NO input/descriptor writes
	}
	bool RunShared() {
		auto before=root;
		while (pc!=0x72038 && pc!=0x72058) StepBlock();
		assert(root==before && minStack>=0x6e000);
		if (pc==0x72058) { assert(l1[Index(0x17ffc8)]==0x46483851); return false; }
		assert(l1[Index(0x17ffc0)]==0x7200c && l1[Index(0x17ffc8)]==0x31483851);
		return l1[Index(0x17ffc4)]==groups*(inputs/16) && requests==packets.size() && responses==requests && !pending;
	}
	void CheckInputs() const {
		for (const auto& word:root) assert(memory.at(word.first)==word.second);
	}
};
#ifndef PIONEER_Q38_SHARED_ROOT_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2 || argc==3);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) for (bool peer:{false,true}) {
		SharedRowsMachine m(argv[1],type,k,1,peer); m.delay=3;
		assert(m.RunShared() && m.CorrectRows()); if (peer) m.CheckInputs();
		printf("OFFLINE shared-input type%u K%u N16 peer%u reads%u PASS\n",type,k,peer,m.requests);
	}
	for (unsigned type:{8U,12U,14U}) {
		SharedRowsMachine m(argv[1],type,5120,16); assert(m.RunShared() && m.CorrectRows()); m.CheckInputs();
	}
	// ALL input packet positions and first/middle/last weight block. Faults
	// leave the command pending,no later command mutation/retry/restore.
	for (unsigned fault:{1U,2U,3U,4U,5U,40U,208U}) {
		SharedRowsMachine m(argv[1],12,17408,3); assert(m.inputReads==4 && m.packets.size()==208);
		m.faultAt=fault; assert(!m.RunShared() && m.pending && m.requests==fault && m.responses==fault-1);
		assert(m.matrixInstructions==(fault<=5?0:(fault-5)*64));
	}
	{
		SharedRowsMachine m(argv[1],12,17408,1); m.badCounter=true;
		assert(!m.RunShared() && m.requests==1 && m.responses==1 && !m.matrixInstructions);
	}
	{
		SharedRowsMachine m(argv[1],12,5120,1); m.counterBase=0xfffffffe;
		assert(m.RunShared() && m.CorrectRows());
	}
	for (bool busy:{false,true}) {
		SharedRowsMachine m(argv[1],12,5120,1); m.pending=busy; m.badReadback=!busy;
		assert(!m.RunShared() && !m.requests && !m.matrixInstructions);
	}
	for (unsigned bad:{0U,17U}) {
		SharedRowsMachine m(argv[1],12,5120,3); m.memory[0x7401c]=bad;
		assert(!m.RunShared() && !m.requests && !m.matrixInstructions);
	}
	for (unsigned at:{0U,4U,8U,12U,16U,20U,36U}) {
		SharedRowsMachine m(argv[1],12,5120,3); m.memory[0x74000+at]=0xffffffff;
		assert(!m.RunShared() && !m.requests && !m.matrixInstructions);
	}
	// Corrupt received inputs/scales must fail input validation BEFORE math.
	for (unsigned address:{0xa0000U,0x94000U}) {
		SharedRowsMachine m(argv[1],12,5120,1); m.root[address]=0x7f800000;
		assert(!m.RunShared() && m.requests==m.inputReads && !m.matrixInstructions);
	}
	unsigned real=0;
	if (argc==3) {
		FILE* file=fopen(argv[2],"rb"); assert(file); PioneerQwen38Gguf model; assert(model.Open(file));
		for (const char* name:{"blk.0.attn_qkv.weight","blk.0.ffn_down.weight","output.weight"}) {
			const auto* t=model.Find(name); assert(t);
			for (unsigned end=0;end<2;end++) {
				SharedRowsMachine m(argv[1],t->type,unsigned(t->inputs),3);
				m.BindRows(model,unsigned(t-model.Tensors().data()),end?unsigned(t->outputs)-48:0);
				assert(m.RunShared() && m.CorrectRows() && model.Stable()); m.CheckInputs(); real++;
				printf("OFFLINE shared-input REAL %s K%u N48 end%u PASS\n",name,m.inputs,end);
			}
		}
		assert(model.Stable()); fclose(file);
	}
	printf("PASS OFFLINE shared-root input actual instructions:21 shapes,real%u,root immutable,peer destinations private,exact FP32+double,no host intermediates,UNKNOWN parks;NOT native/fleet/graph/inference/speed\n",real);
}
#endif
