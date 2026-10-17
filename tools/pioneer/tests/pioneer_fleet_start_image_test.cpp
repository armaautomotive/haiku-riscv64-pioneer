/* SPDX-License-Identifier: MIT */
#define PIONEER_BULK_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_bulk_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeFleetStartImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerComputeFleet.h"
struct Prelude {
	BulkQ8Machine& math;
	uint32_t regs[32]={},pc=TT_FLEET_START,command[10]={},counter[4]={},source[4096]={};
	unsigned peer,xy,steps=0,requests=0,poisoned=0,delay=0,polls=0,fault=0;
	bool pending=false,failed=false,doorbellRead=false;
	Prelude(BulkQ8Machine& m,unsigned worker):math(m),peer(worker!=0),xy(PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6)) {
		memcpy(source,m.buffers,sizeof(source));
		if (peer) for (unsigned i=0;i<4096;i++) m.buffers[i]=0xcafebabe;
		for (unsigned i=0;i<m.outputs;i++) m.buffers[4096+i]=0x12345678;
		command[0]=m.base; command[2]=786; command[3]=0x88000; command[5]=xy; command[8]=9216;
	}
	uint32_t Load(uint32_t a) {
		if (a>=0x70500 && a<=0x70510) {
			const uint32_t d[]={math.base,math.inputs/16,math.outputs/16,peer,xy}; return d[(a-0x70500)/4];
		}
		if (a>=0xffb20000 && a<=0xffb20024) return command[(a-0xffb20000)/4];
		if (a==0xffb20040) {
			doorbellRead=true;
			if (pending && ++polls>delay && fault!=1) {
				memcpy(math.buffers,source,sizeof(source)); pending=false;
				for (unsigned i=0;i<4;i++) counter[i]+=i==1?256:1;
				if (fault>=2 && fault<=5) counter[fault-2]--;
			}
			return pending;
		}
		if (a>=0xffb20208 && a<=0xffb20214) return counter[(a-0xffb20208)/4];
		if (a==0xffb20240) { assert(doorbellRead); return pending || fault==6; }
		if (a==0x17ffc8) return 0x4e4f4346;
		assert(false); return 0;
	}
	void Store(uint32_t a,uint32_t v) {
		if (a>=0xffb20000 && a<=0xffb20024) { assert(!pending); command[(a-0xffb20000)/4]=v; return; }
		if (a==0xffb20040) {
			const uint32_t expected[]={0x80000,0,129,0x80000,0,xy,0,0,16384,0};
			assert(peer && v==1 && !pending && !requests && !memcmp(command,expected,sizeof(expected)));
			requests++; pending=true; return;
		}
		if (a>=0x84000 && a<0x84000+math.outputs*4) {
			assert(!pending && v==0x7fc00000 && a==0x84000+poisoned*4); math.buffers[4096+poisoned++]=v; return;
		}
		assert(a==0x17ffc8 && v==0x4e4f4346); failed=true;
	}
	bool Run() {
		while (pc!=0x70000 && pc!=TT_FLEET_FAULT_PARK) {
			assert(++steps<2000000 && pc>=TT_FLEET_START && pc<TT_FLEET_START+TT_FLEET_START_BYTES && !(pc&3));
			uint32_t instruction=BlackholeFleetStartCodeWord((pc-TT_FLEET_START)/4),next=pc+4,result=0;
			unsigned op=instruction&127,rd=(instruction>>7)&31,f3=(instruction>>12)&7,rs1=(instruction>>15)&31,rs2=(instruction>>20)&31;
			int32_t imm=Sign(instruction>>20,12); bool write=false;
			if (op==0x37) { result=instruction&0xfffff000; write=true; }
			else if (op==0x13) {
				if (!f3) result=regs[rs1]+imm;
				else if (f3==7) result=regs[rs1]&imm;
				else { assert(f3==1); result=regs[rs1]<<(instruction>>20&31); } write=true;
			} else if (op==3) { assert(f3==2); result=Load(regs[rs1]+imm); write=true; }
			else if (op==0x23) { assert(f3==2); imm=Sign(((instruction>>25)<<5)|(instruction>>7&31),12); Store(regs[rs1]+imm,regs[rs2]); }
			else if (op==0x63) {
				assert(f3<=1); imm=Sign(((instruction>>31)<<12)|((instruction>>7&1)<<11)|((instruction>>25&63)<<5)|((instruction>>8&15)<<1),13);
				if ((regs[rs1]==regs[rs2])==(f3==0)) next=pc+imm;
			} else if (op==0x6f) {
				imm=Sign(((instruction>>31)<<20)|((instruction>>12&255)<<12)|((instruction>>20&1)<<11)|((instruction>>21&1023)<<1),21);
				result=next; next=pc+imm; write=true;
			} else if (op==0x67) { assert(!f3); result=next; next=(regs[rs1]+imm)&~1U; write=true; }
			else assert(instruction==0x0ff0000f);
			if (write && rd) regs[rd]=result;
			regs[0]=0; pc=next;
		}
		if (failed) { assert(pc==TT_FLEET_FAULT_PARK && !poisoned); return false; }
		assert(pc==0x70000 && poisoned==math.outputs && requests==peer && !pending);
		assert(!memcmp(math.buffers,source,sizeof(source)));
		const uint32_t expected[]={math.base,0,786,0x88000,0,xy,0,0,9216,0};
		assert(!memcmp(command,expected,sizeof(expected))); return true;
	}
};
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==TT_FLEET_START_BYTES);
	for (unsigned w=0;w<code.size()/4;w++) assert(Word(code,w*4)==BlackholeFleetStartCodeWord(w));
	assert(!BlackholeFleetStartCodeWord(88) && !BlackholeFleetStartCodeWord(1023));
	for (unsigned worker=0;worker<120;worker++) {
		BulkQ8Machine math(1024,32,worker%2); Prelude p(math,worker); p.delay=3; assert(p.Run());
		math.Run(); assert(math.CorrectIeee() && math.Guards());
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) for (unsigned n:{32U,64U,4096U}) {
		BulkQ8Machine math(k,n,1); Prelude p(math,119);
		for (auto& v:p.counter) v=0xfffffff0; assert(p.Run()); math.Run(); assert(math.CorrectIeee() && math.Guards());
	}
	for (unsigned fault=1;fault<=6;fault++) {
		BulkQ8Machine math(1024,32,0); Prelude p(math,1); p.fault=fault; assert(!p.Run());
		assert(!math.requests && !math.matrixInstructions);
	}
	puts("PASS OFFLINE assembled fleet input pull + device-local poison,120 coordinates,all K/N4096 IEEE outputs,counter wrap/fault park; NOT hardware or throughput qualification");
}
