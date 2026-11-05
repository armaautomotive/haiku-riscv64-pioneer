/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_PIPELINE_MACHINE_H
#define PIONEER_Q38_Q4_FPU_PIPELINE_MACHINE_H
#include "pioneer_q38_native_bfp_machine.h"
#include "pioneer_blackhole_arithmetic_observer.h"
#include "pioneer_blackhole_sfp_load.h"
#ifdef PIONEER_Q38_COMPACT_FPU_PIPELINE_MODEL
#define PIONEER_Q38_VECTOR_Q4_METADATA_MODEL_ONLY
#include "pioneer_q38_vector_q4_metadata_test.cpp"
#else
#define PIONEER_Q38_Q4_FPU_DECODE_MODEL_ONLY
#include "pioneer_q38_q4_fpu_decode_test.cpp"
#endif
#include "../PioneerQ38Q4FpuBlock.h"
#include "../PioneerQ38Q4FpuBlockData.h"
// NEW OFFBOARD observer,not hardware timing/ownership authority. One actual
// L1 byte array feeds UNPACK. Explicit shared PHYSICAL Dst16 rows feed both
// SFPU and MOVD2A/FPU;never normalize descriptors or insert decoded weights.
struct Q4FpuPipelineMachine:NativeBfpMachine {
	struct Fault {};
	virtual ~Q4FpuPipelineMachine()=default;
	// Default preserves the original85 observer EXACTLY. New streaming
	// tests may select a disjoint actual L1 buffer and require its producer
	// completion;the bytes are NEVER normalized/copied into an old address.
	virtual unsigned OperandOffset()const{return 0;}
	virtual bool OperandReady(unsigned,unsigned)const{return true;}
	uint16_t physical[1024][16]={};bool defined[1024]={},written[1024][16]={};
	bool decoded[64][16]={},moved[64]={},enabled=false;
	unsigned dstReady[1024]={},rawLoads=0,metadataLoads=0,zeroLoads=0,inputLoads=0,moves=0,transposes=0;
	unsigned skipUnpack=0,unpackAttempts=0,skipMove=0,skipTranspose=0;bool skipZero=false,dirtyPadding=false;
#ifdef PIONEER_Q38_COMPACT_FPU_PIPELINE_MODEL
	MetadataMachine vector; // SAME physical LRegs;strict existing arithmetic
#else
	Q4FpuMachine vector;
#endif
	struct Task {bool pending=false,zero=false,publish=false,fp32=false;unsigned address=0,row=0,count=0,due=0;} task;
	static unsigned Adj(unsigned row){return ((row&0x1f8)<<1)|(row&0x207);}
	uint32_t FloatDst(unsigned row,unsigned col) {
		unsigned hi=Adj(row),lo=hi+8;
		if(!defined[hi] || !written[hi][col] || !written[lo][col] || dstReady[hi]>cycle || dstReady[lo]>cycle)throw Fault{};
		uint16_t h=physical[hi][col];unsigned ieee=(h&0x8000)|((h&255)<<7)|((h>>8)&127);
		return (ieee<<16)|physical[lo][col];
	}
	void StoreFloat(unsigned row,unsigned col,uint32_t bits,unsigned ready) {
		unsigned hi=Adj(row),lo=hi+8,h=bits>>16;
		physical[hi][col]=uint16_t((h&0x8000)|((h&127)<<8)|((h>>7)&255));physical[lo][col]=uint16_t(bits);
		written[hi][col]=written[lo][col]=true;dstReady[hi]=dstReady[lo]=ready;
	}
	bool Pending()const{return task.pending || NativeBfpMachine::Pending();}
	void CompleteTask() {
		if(!task.pending)return;
		if(dropProducer==1)throw Unknown;
		cycle=std::max(cycle,task.due);
		for(unsigned i=0;i<task.count;i++) {
			unsigned row=task.row+i/16,col=i%16;
			if(task.fp32) {
				unsigned at=task.address+i*4;assert(at+3<l1.size());
				uint32_t bits=unsigned(l1[at])|(unsigned(l1[at+1])<<8)|(unsigned(l1[at+2])<<16)|(unsigned(l1[at+3])<<24);
				if(task.zero)bits=0;
				if(task.zero && dirtyPadding && row>=128 && i%16==0)bits=0x3f800000;
				StoreFloat(row,col,bits,cycle);if(col==15)defined[Adj(row)]=defined[Adj(row)+8]=true;
			} else {
				assert(task.address+i<l1.size());unsigned byte=l1[task.address+i];
				physical[row][col]=uint16_t((byte<<5)|(byte?16:0));written[row][col]=true;dstReady[row]=cycle;
				if(col==15)defined[row]=true;
			}
		}
		if(task.publish) {
			auto& a=source[0][unpackBank[0]];if(a.owner || a.pending)throw Fault{};
			a.owner=true;unpackBank[0]^=1;
		}
		task.pending=false;
	}
	void Drain() {CompleteTask();NativeBfpMachine::Drain();}
	void Config(unsigned index,unsigned value) {if(Pending() || mathPending)throw Fault{};NativeBfpMachine::Config(index,value);}
	void UnpackToDst(unsigned op) {
		if(Pending() || source[0][unpackBank[0]].owner)throw Fault{};
		assert(thread[0]==0 && thread[5]==0 && !config[49] && config[73]==0x13 && config[86]==0x100010);
		assert(xStart[0]==0 && config[67]==0 && config[74]==0x17fff && !config[75] && !config[92]);
		const bool fp32=config[72]==0,zero=op==0x42000091,publish=op==0x420000c1;
		const unsigned row=(config[84]&65535)/16-4,address=(config[76]+1)*16,count=xEnd[0]+1;
		assert(address>=64 && address+count*(fp32?4:1)<=l1.size());
		if(!OperandReady(address,count*(fp32?4:1)))throw Fault{};
		for(unsigned h=0;h<64;h++)if(l1[address-64+h])throw Fault{};
		unpackAttempts++;if(unpackAttempts==skipUnpack || (zero && skipZero))return;
		if(fp32) {
			assert(config[1]==0x20200000 && config[64]==0x100010 && !publish);
			if(zero){assert(count==256 && config[65]==16 && address==0x97000 && row>=64 && row<=128 && !(row&15));zeroLoads++;}
			else {
				assert(count==16 && config[65]==1 && row>=128 && row<=140 && !(row&3));
				const unsigned group=(metadataLoads/4)%8,slot=metadataLoads%4;
				assert(address==0x94000+OperandOffset()+(group*4+slot)*128 && row==128+slot*4);metadataLoads++;
			}
		} else {
			assert(config[1]==0x203c8000 && config[64]==0x10001e && config[65]==16 && config[72]==14 && count==256 && !zero);
			unsigned group=(rawLoads/4)%8,face=rawLoads%4;
			assert(row==face*16 && address==0x90000+OperandOffset()+((group/2)*4+face)*320 && publish==(face==3));
			if(!face){memset(decoded,0,sizeof(decoded));memset(moved,0,sizeof(moved));}rawLoads++;
		}
		task={true,zero,publish,fp32,address,row,count,cycle+delay[0]};unpacks[0]++;
	}
	void UnpackInput() {
		if(Pending())throw Fault{};
		assert(thread[0]==0 && thread[5]==4 && config[112]==0x00200015 && config[113]==0x00010001
			&& config[114]==1 && config[115]==0 && config[120]==0x25 && config[121]==0x000f000f
			&& config[122]==0x17fff && !config[123] && !config[140] && !config[61]);
		assert(xStart[1]==0 && xEnd[1]==31);unsigned address=(config[124]+1)*16;
		assert(address==0x96000+OperandOffset()+(inputLoads%8)*128);
		if(!OperandReady(address,64))throw Fault{};
		for(unsigned h=0;h<64;h++)if(l1[address-64+h])throw Fault{};
		auto& b=source[1][unpackBank[1]];if(b.pending || b.owner)throw Fault{};
		for(unsigned i=0;i<32;i++)b.values[i/16][i%16]=Float(uint32_t(l1[address+i*2]|(unsigned(l1[address+i*2+1])<<8))<<16);
		b.pending=true;b.due=cycle+delay[1];inputLoads++;unpacks[1]++;
	}
	void Sfpu(unsigned op) {
		if(Pending() || !enabled)throw Fault{};
		vector.cycle=cycle;unsigned code=op>>24;
		if(code==0x70 || code==0x72) {
			unsigned mode=(op>>16)&15,reg=(op>>20)&15,address=op&1023;assert(reg<8 && !(address&1) && !(op&0xfc00));
			for(unsigned lane=0;lane<32;lane++) {
				unsigned row=(address&~3U)+lane/8,col=(lane&7)*2+bool(address&2);
				if(code==0x70 && mode==3) {assert((reg==0 || reg==4) && row>=128 && row<144);vector.lanes[reg][lane]=FloatDst(row,col);}
				else if(code==0x70 && mode==13) {
					assert(!reg && row<64);if(!defined[row] || !written[row][col] || dstReady[row]>cycle)throw Fault{};
					vector.lanes[0][lane]=BlackholeRawSfpLoad(physical[row][col],13);
				} else {
					assert(code==0x72 && mode==3 && !reg && row>=64 && row<128);
					if(!defined[Adj(row)] || !defined[Adj(row)+8] || decoded[row-64][col])throw Fault{};
					uint32_t bits=vector.Read(0,lane);assert(!(bits&65535));StoreFloat(row,col,bits,cycle+1);decoded[row-64][col]=true;
				}
			}
			if(code==0x70)vector.ready[reg]=cycle+1;
		} else if(code==0x8c) {
			assert(op==0x8c000000);transposes++;if(transposes==skipTranspose)return;
			uint32_t old[8][32];for(unsigned reg=0;reg<8;reg++)for(unsigned lane=0;lane<32;lane++)old[reg][lane]=vector.Read(reg,lane);
			for(unsigned base:{0U,4U})for(unsigned reg=0;reg<4;reg++)for(unsigned group=0;group<4;group++)for(unsigned col=0;col<8;col++)
				vector.lanes[base+reg][group*8+col]=old[base+group][reg*8+col];
			for(auto& ready:vector.ready)ready=cycle+1;
		} else {vector.Op(op);if(!vector.okay)throw Fault{};}
	}
	void Move(unsigned op) {
		if(Pending() || !source[0][matrixBank[0]].owner)throw Fault{};
		assert((op&~((63U<<17)|1023U))==0x08002000 && thread[2]==1 && config[0]==0x42b5 && config[1]==0x60aa0000);
		unsigned target=(op>>17)&63,row=op&1023;assert(!(target&3) && row==64+target);
		moves++;if(moves==skipMove)return;
		for(unsigned i=0;i<4;i++) {
			if(moved[target+i])throw Fault{};moved[target+i]=true;
			for(unsigned c=0;c<16;c++) {
				if(!decoded[row+i-64][c])throw Fault{};uint32_t bits=FloatDst(row+i,c);assert(!(bits&65535));
				// MOVD2A with explicit BF16 interpretation reads the shuffled
				// physical FP32 HIGH half;its low half was explicitly zero.
				source[0][matrixBank[0]].values[target+i][c]=Float(bits);
			}
		}
		mathPending=true;
	}
	void MatrixCore(unsigned op) {
		assert(thread[2]==1 && thread[3]==1 && thread[5]==4 && !config[2] && !config[3]
			&& config[0]==0x42b5 && config[1]==0x60aa0000 && !thread[11] && !thread[13] && !thread[48]
			&& ((thread[29]>>13)&3)==1 && (op&~0x00c003ffU)==0x26084000);
		if(Pending())throw Fault{};
		for(bool value:moved)if(!value)throw Fault{};
		auto& a=source[0][matrixBank[0]];auto& b=source[1][matrixBank[1]];if(!a.owner || !b.owner)throw Fault{};
		unsigned ar=rwcA&0x30,br=rwcB&63,dr=op&1023,phase=fidelity&3;
		assert(ar<=48 && br==(ar/16&1) && dr==(ar<32?32U:40U));visits[phase]++;
		// Arithmetic comes from the separate Fraction-qualified rounded
		// observer,not host ideal sums. This callback validates lifecycle only.
		if(phase!=dropPhase){valid[dr]=true;for(unsigned row=4;row<8;row++)valid[dr+row]=true;}
		if(!omitFlip)for(unsigned which=0;which<2;which++)if(op&(1U<<(22+which))){source[which][matrixBank[which]].owner=false;matrixBank[which]^=1;}
		fidelity=(fidelity+1)&3;matrixOps++;mathPending=true;
	}
	void Op(unsigned op) {
		unsigned code=op>>24;
		if(code==0x70 || code==0x72 || code==0x71 || code==0x7c || code==0x7a || code==0x7e
			|| code==0x79 || code==0x8d || code==0x90 || code==0x84 || code==0x8f || code==0x8c) {cycle++;Sfpu(op);return;}
		if(code==8){cycle++;Move(op);return;}
		if(code==0x26) {
			cycle++;if(mathPending && !(matrixOps%16))throw Fault{}; // required MOVD2A completion
			// Never let even the arithmetic observer READ an operand before
			// every source row is published. Missing MOVD2A is UNKNOWN,not NaN
			// arithmetic or a numerical result eligible for comparison.
			if(Pending())throw Fault{};
			for(bool value:moved)if(!value)throw Fault{};
			PioneerBlackholeArithmeticObserver::Matrix(*this,op,true,[&]{MatrixCore(op);});
			unsigned row=op&1023;
			if(valid[row]){for(unsigned col=0;col<16;col++)StoreFloat(row,col,Bits(destination[row][col]),cycle);defined[Adj(row)]=defined[Adj(row)+8]=true;}
			return;
		}
		if(op==0x42000091 || op==0x42000081 || op==0x420000c1){cycle++;UnpackToDst(op);return;}
		if(op==0x428000c1){cycle++;UnpackInput();return;}
		if(op==0x8a000002){cycle++;enabled=true;return;}
		if(op==0x5120000a || op==0x5420000f){if(Pending())throw Fault{};cycle++;return;} // U0 Y/Z/W ADCs stay zero
		if(Pending())throw Fault{};
		if(op==0x10180000){memset(defined,0,sizeof(defined));memset(written,0,sizeof(written));enabled=false;}
		NativeBfpMachine::Op(op);
	}
	bool Clean()const{return !task.pending && NativeBfpMachine::Clean();}
	std::array<uint32_t,32> Results() {
		if(!Clean())throw Fault{};std::array<uint32_t,32> out;
		for(unsigned i=0;i<32;i++){assert(valid[i<16?32:40]);out[i]=FloatDst(i<16?32:40,i%16);outputReads++;}return out;
	}
	void Stage(const std::array<uint8_t,6144>& prepared,const std::array<uint16_t,256>& input) {
		assert(Clean());
		for(unsigned pair=0;pair<4;pair++)for(unsigned face=0;face<4;face++) {
			unsigned address=0x90000+(pair*4+face)*320,from=(face/2)*2048+pair*512+(face%2)*256;
			memset(l1.data()+address-64,0,64);memcpy(l1.data()+address,prepared.data()+from,256);
		}
		for(unsigned slot=0;slot<32;slot++){unsigned address=0x94000+slot*128;memset(l1.data()+address-64,0,64);memcpy(l1.data()+address,prepared.data()+4096+slot*64,64);}
		for(unsigned group=0;group<8;group++) {
			unsigned address=0x96000+group*128;memset(l1.data()+address-64,0,64);
			for(unsigned c=0;c<32;c++){l1[address+c*2]=uint8_t(input[group*32+c]);l1[address+c*2+1]=uint8_t(input[group*32+c]>>8);}
		}
		memset(l1.data()+0x97000-64,0,1088);
	}
};
#endif
