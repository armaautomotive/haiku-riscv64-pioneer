/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_HAS_ROWS_MACHINE
#define PIONEER_Q38_SHARED_ROWS_MACHINE_ONLY
#include "pioneer_qwen38_rows_image_test.cpp"
#endif
#include <memory>
#include "../PioneerBlackholeFma.h"
#include "../PioneerQ38VectorFixedFormat.h"
#include "../PioneerQ38BFloatPanel.h"
#include "pioneer_blackhole_sfp_load.h"
// ACTUAL compiled RV32/FPU image plus independent exact integer SFPU subset.
// Not a silicon cycle model or permission to select a new native image.
template<class Base,bool Persistent=false,bool BFloat=false,bool Accumulate=false> struct VectorRows:Base {
	using Base::memory; using Base::config; using Base::thread; using Base::ownerA;
	using Base::pending; using Base::drained; using Base::tile; using Base::tiles; using Base::backendWait;
	using Base::adcX0; using Base::adcX1; using Base::pc; using Base::configWrites;
	using Base::Range; using Base::buffers; using Base::groups; using Base::inputs;
	using Base::requests; using Base::responses; using Base::minStack; using Base::l1;
	using Base::Index; using Base::StepBlock; using Base::Operand16;
	using Base::moveInstructions; using Base::a;
	using Base::dstInt; using Base::outputReads;
	using Base::type;
	uint16_t physical[1024][16]={}; uint32_t lr[8][32]={};
	bool wrote[1024][16]={},sources[2]={},rawPending=false,publish=false,enabled=false;
	bool defined[1024]={},decoded[16][16]={};
	unsigned unpackBank=0,mathBank=0,rawAddress=0,rawRow=0,adc0=0,adc1=255;
	unsigned rawReads=0,vectorOps=0,panelMoves=0,rawUnpacks=0;
	unsigned metadataWrites=0,instructionBufferStores=0;
	unsigned dotProbeReads=0;
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
	struct BlockAffineHazard {};
	unsigned blockScratchWords=0,blockImports=0,blockImportAttempts=0,blockInitializations=0;
	uint32_t blockImportMask=0;
	bool omitBlockInitialization=false;
	unsigned dropBlockImport=0;
	bool BlockAffineActive() const { return Persistent && !BFloat && Accumulate && type==12 && memory.at(0x74038)==1; }
#endif
#ifdef PIONEER_Q38_NATIVE_OPERAND_STORE
	struct NativeStoreHazard {};
	bool poisonNativeLow=false;
	unsigned nativeStoreOps=0;
#endif
	// Independent frontend replay expander; poison/undefined entries must
	// never be read. Only register-only arithmetic may enter this prototype.
	struct ReplayAccessFault {};
	uint32_t replay[32]={}; bool replayDefined[32]={};
	unsigned recordNext=0,recordRemaining=0,replayRecords=0,replayExpansions=0;
	bool recordExecute=false,dropReplayRecord=false;
	unsigned cacheNext=0,cacheJobs=0;
	unsigned setupConfiguration=25;
	bool corruptMove=false,corruptDotRange=false,negativeDotZero=false;
	int corruptDotValue=262145;
	bool corruptDecodedProbe=false; unsigned decodedProbeReads=0;
	unsigned sourceProbeRows=0,sourceProbeReads=0;
	uint32_t badFloatDot=0;
	bool rawFloat=false,rawInitial=false,affineEligible=false;
	// Negative controls for the independent pinned-ISA observer only.
	bool omitDstInitialization=false,allRowsClear=false,wormholeRawByte=false;
#ifdef PIONEER_Q38_TEST_PIPELINED_MATRIX
	struct PendingMatrixAccess {};
	std::vector<uint32_t> phaseQueue;
	bool executingPhases=false,dropPhaseFence=false;
	unsigned maximumPhaseBatch=0,queuedMatrices=0;
#endif
	VectorRows(const char* path,unsigned type,unsigned k,unsigned groups):Base(path,type,k,groups) { if (!Persistent) memory[0x74000]=0x31563851; }
	VectorRows(const char* path,unsigned type,unsigned k,unsigned groups,unsigned worker):Base(path,type,k,groups,worker) { static_assert(Persistent,"worker routing ONLY in the persistent model"); }
	bool InlineTensix() const override { return true; }
#ifdef PIONEER_Q38_TEST_PREDECODED_Q4
	virtual bool DirectQ4Panel() const { return false; }
#ifdef PIONEER_Q38_TEST_NATIVE_Q8
	virtual bool DirectNativeQ8Sfpu() const { return false; }
#endif
#endif
	bool Int8Arithmetic() const override { return BFloat?false:Base::Int8Arithmetic(); }
	bool Int8ConvertedRead() const override { return BFloat?false:Base::Int8ConvertedRead(); }
	bool Fp16Operands() const override { return BFloat?false:Base::Fp16Operands(); }
	bool FloatFidelityArithmetic() const override { return BFloat?true:Base::FloatFidelityArithmetic(); }
	unsigned MatrixPhasesPerPanel() const override {
#ifdef PIONEER_Q38_Q4_TWO_PHASE
		static_assert(!BFloat,"two-phase candidate keeps integer formats");
		return type==12?2:4;
#else
		return BFloat?(type==8?2U:1U):Base::MatrixPhasesPerPanel();
#endif
	}
	unsigned MatrixFidelityStep() const override {
#ifdef PIONEER_Q38_Q4_TWO_PHASE
		return type==12?2:1;
#else
		return Base::MatrixFidelityStep();
#endif
	}
	float Operand16(uint16_t half) const override { return BFloat?Float(uint32_t(half)<<16):Base::Operand16(half); }
	uint32_t MatrixArithmeticConfig1() const override {
		// Read-only model of the pinned MVMUL fields,NOT a change to CSR
		// state:only the two proven ignored fields may differ under forced
		// A/B FP16 interpretation and disabled implied formats. All source
		// ownership,integer phases and input/output bits stay unmodified.
		if (BFloat) return PioneerQ38BFloatMatrixEquivalent(config[0],config[1],thread[2],thread[3])?0x20aa0000:config[1];
		return PioneerQ38VectorFixedMatrixEquivalent(config[0],config[1],thread[2],thread[3])?0xa0220000:config[1];
	}
	static unsigned Adj(unsigned row) { return ((row&0x1f8)<<1)|(row&0x207); }
	uint32_t FloatDst(unsigned row,unsigned col) const {
		unsigned hi=Adj(row),lo=hi+8; assert(defined[hi] && wrote[hi][col] && wrote[lo][col]);
		uint16_t h=physical[hi][col]; unsigned ieee=(h&0x8000)|((h&255)<<7)|((h>>8)&127);
		return (ieee<<16)|physical[lo][col];
	}
	void StoreFloatDst(unsigned row,unsigned col,uint32_t value) {
		unsigned hi=Adj(row),lo=hi+8,h=value>>16;
		physical[hi][col]=uint16_t((h&0x8000)|((h&127)<<8)|((h>>7)&255));
		physical[lo][col]=uint16_t(value); wrote[hi][col]=wrote[lo][col]=true;
	}
	void Instruction(uint32_t op) override {
		if (recordRemaining) {
			unsigned code=op>>24;
			if (code!=0x71 && code!=0x7c && code!=0x7a && code!=0x8d && code!=0x79 && code!=0x7e) throw ReplayAccessFault{};
			assert(recordNext<32);
			replay[recordNext]=op; replayDefined[recordNext++]=!dropReplayRecord;
			recordRemaining--;
			if (!recordExecute) return;
		} else if ((op>>24)==4) {
			unsigned index=(op>>14)&31,count=(op>>4)&63;
			if ((op&~0x0407c3f3U) || !count || index+count>32) throw ReplayAccessFault{};
			if (op&1) { recordNext=index; recordRemaining=count; recordExecute=bool(op&2); replayRecords++; }
			else {
				for (unsigned i=index;i<index+count;i++) {
					if (!replayDefined[i]) throw ReplayAccessFault{};
					Instruction(replay[i]);
				}
				replayExpansions++;
			}
			return;
		}
#ifdef PIONEER_Q38_TEST_PIPELINED_MATRIX
		if (!executingPhases && ((op&~0x4000U)==0x26000020 || (op&~0x4000U)==0x26c00020)) {
			if (phaseQueue.size()==4) throw PendingMatrixAccess{};
			phaseQueue.push_back(op); queuedMatrices++; drained=false;
			maximumPhaseBatch=std::max(maximumPhaseBatch,unsigned(phaseQueue.size())); return;
		}
		if (!phaseQueue.empty()) {
			if (op==0xa2101816) {
				if (dropPhaseFence) { Base::gpr63=1; return; } // deliberately false completion negative control
				auto batch=phaseQueue; phaseQueue.clear(); executingPhases=true;
				for (auto command:batch) Instruction(command);
				executingPhases=false;
			} else if (dropPhaseFence && op==0x4500017e) { Base::gpr63=1; return; }
			else throw PendingMatrixAccess{}; // source/RWC/SFPU change before real matrix completion
		}
#endif
		if (omitDstInitialization && op==0x42000091) return;
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
		if (BlockAffineActive() && (op==0x42000091 || op==0x42000081) && config[72]==0) {
			unsigned row=((config[84]&65535)/16)-4;
			if (row>=128 && row<192) {
				if (op==0x42000091) { if (omitBlockInitialization) return; blockInitializations++; }
				else { blockImportAttempts++; if (dropBlockImport==blockImportAttempts) return; }
			}
		}
#endif
		if (allRowsClear && (op==0x10280002 || op==0x10080004)) op=0x10180000;
		unsigned code=op>>24;
		if (op==0x10080004 || op==0x10080005) {
			assert(Base::sfpuSmoke>=16 && !rawPending && !pending);
			// Independent ZEROACC encoding:UseDst32b=0,mode16_ROWS=1,
			// physical row range Imm10*16..+15. Logical FP32 validity
			// consults ONLY physical Adj32(row);decoded rows stay defined.
			unsigned first=(op&1023)*16;
			if (!Base::dropClear) {
				for (unsigned row=first;row<first+16;row++) defined[row]=false;
				for (unsigned row=0;row<64;row++) if (Adj(row)>=first && Adj(row)<first+16) Base::valid[row]=false;
			}
			drained=false; return;
		}
		if (op==0x10180000) for (auto& value:defined) value=false; // ALL1024 physical row-valid bits
		if (op==0x10280002) for (unsigned row=32;row<48;row++) defined[Adj(row)]=false;
		if (code==0x42 && !(op&0x800000)) {
			assert(!rawPending && !pending && !sources[unpackBank] && !ownerA
				&& !config[49] && config[73]==0x13 && thread[5]==0 && !adc0);
			rawFloat=config[72]==0;
			rawInitial=op==0x42000091;
			rawAddress=(config[76]+1)*16; rawRow=((config[84]&65535)/16)-4;
			if (rawFloat) {
				assert(config[1]==(BFloat?0x20200000U:0xa0200000U) && config[64]==0x100010);
				if (rawInitial) {
					assert(config[65]==16 && adc1==255 && rawAddress==0xa0000 && (rawRow==64 || rawRow==96
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
						|| (BlockAffineActive() && rawRow>=128 && rawRow<=176 && !(rawRow&15))
#endif
					));
				} else {
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
					if (rawRow>=128 && rawRow<192) {
						assert(BlockAffineActive() && blockScratchWords==256 && !(rawRow&3)
							&& rawAddress==0x97000+(rawRow-128)*16 && config[65]==1 && adc1==15 && op==0x42000081);
					} else
#endif
					assert(affineEligible && config[65]==1 && adc1==15 && op==0x42000081 && (rawRow==96 || rawRow==100 || rawRow==104));
					assert(rawAddress==(rawRow==96?0x91200U:rawRow==100?0x91240U:rawAddress));
					if (rawRow==104) assert(!(rawAddress&63) && Range(rawAddress,0x84000,groups*64));
				}
			} else {
				assert(config[1]==(BFloat?0x203c8000U:0xa03c8000U) && config[64]==0x10001e && config[65]==16 && config[72]==14
					&& adc1==255 && (op==0x42000081 || op==0x420000c1));
				assert(rawRow==0 || rawRow==16);
				assert(rawAddress>=0x90000 && rawAddress-0x90000+256<=tile.size());
				if (!rawRow) {
					memset(decoded,0,sizeof(decoded)); // generation tracking,NOT a hardware valid-bit reset
					panelMoves=0; affineEligible=false;
					sourceProbeRows=0;
				}
			}
			rawPending=true; publish=op&64; rawUnpacks++; drained=false; return;
		}
		if (op==0x5e23fc00) { adc0=adcX0[0]=0; adc1=adcX1[0]=255; return; }
		if (op==0x5e203c00) { adc0=adcX0[0]=0; adc1=adcX1[0]=15; return; }
		if (op==0x5120000a || op==0x5420000f) { assert(!rawPending); return; }
		if (op==0x4500017e && rawPending) {
			assert(backendWait && !pending);
			if (rawFloat) {
				for (unsigned i=0;i<(rawInitial?256U:16U);i++) {
					uint32_t data=
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
						(!rawInitial && rawRow>=128 && rawRow<192)?memory.at(rawAddress+i*4):
#endif
						Base::Load(rawAddress+i*4);
					if (rawInitial) { assert(PioneerInt8OperandValid(uint16_t(data)) && PioneerInt8OperandValid(uint16_t(data>>16))); data=0; }
					StoreFloatDst(rawRow+i/16,i%16,data); rawReads+=4;
				}
				for (unsigned row=rawRow;row<rawRow+(rawInitial?16U:1U);row++) defined[Adj(row)]=true;
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
				if (!rawInitial && rawRow>=128 && rawRow<192) { blockImports++; blockImportMask|=1U<<((rawRow-128)/4); }
#endif
			}
			else for (unsigned i=0;i<256;i++) {
				unsigned address=rawAddress+i; uint32_t word=memory.at(address&~3U),byte=(word>>(8*(address&3)))&255;
				physical[rawRow+i/16][i%16]=uint16_t((byte<<5)|(byte?16:0)); wrote[rawRow+i/16][i%16]=true; rawReads++;
				if (i%16==15) defined[rawRow+i/16]=true;
			}
			if (publish) { sources[unpackBank]=true; unpackBank^=1; ownerA=sources[mathBank]; }
			rawPending=false;
		}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS
		if (code==0x12) {
			assert(type==8 && !BFloat && ownerA && sources[mathBank] && panelMoves==4 && !rawPending
				&& config[0]==0x4231 && config[1]==0xa03c8000 && thread[2]==1 && sourceProbeRows<16);
			assert(op==(0x12002060U|(sourceProbeRows<<17)|sourceProbeRows));
			// Independent pinned MOVA2D semantics:FP32 forces TF32 source
			// interpretation,retains ALL10 source mantissa bits,no clipping.
			for (unsigned row=sourceProbeRows;row<sourceProbeRows+8;row++) {
				for (unsigned col=0;col<16;col++) {
					float value=a[row][col]; assert(value==float(int(value)) && value>=-128 && value<=127);
					unsigned magnitude=unsigned(value<0?-value:value);
					StoreFloatDst(96+row,col,(value<0?0x80000000U:0U)|0x08000000U|(magnitude<<13));
				}
				defined[Adj(96+row)]=true;
			}
			sourceProbeRows+=8; drained=false; return;
		}
#endif
		if (op==0x8f000000) { vectorOps++; drained=false; return; }
		if ((code>=0x70 && code<=0x8d) || code==8 || code==0x90) {
			bool registerOnly=false;
#ifdef PIONEER_Q38_TEST_NATIVE_Q8
			if(DirectNativeQ8Sfpu() && op==0x8a000002) {
				for(unsigned row=96;row<112;row++)for(unsigned col=0;col<16;col++)
					assert(defined[Adj(row)] && wrote[Adj(row)][col] && wrote[Adj(row)+8][col]);
				registerOnly=true; // ONLY explicit lane enable,NOT Dst/source data consumption
			}
#endif
#ifdef PIONEER_Q38_TEST_BLOCK_SCALE_REUSE
			registerOnly=registerOnly || (BlockAffineActive() && blockScratchWords==256 && blockImportMask==65535
				&& (op==0x8a000002 || (code==0x71 && ((op>>20)&7)==3)));
#endif
			assert(!rawPending && !pending && ((ownerA && sources[mathBank]) || affineEligible || registerOnly) && thread[2]==1
				&& (BFloat?PioneerQ38BFloatMatrixEquivalent(config[0],config[1],thread[2],thread[3]):
					(config[1]==0xa0220000 || PioneerQ38VectorFixedMatrixEquivalent(config[0],config[1],thread[2],thread[3])
#ifdef PIONEER_Q38_TEST_NATIVE_Q8
						|| (DirectNativeQ8Sfpu() && config[0]==0x423e && config[1]==0xa03c0000 && thread[2]==1 && thread[3]==1)
#endif
					))
				&& config[220]==0 && thread[5]==4);
			vectorOps++;
			if (op==0x8a000002) { enabled=true; return; }
			assert(enabled);
			if (code==0x70 || code==0x72) {
				unsigned mode=(op>>16)&15,reg=(op>>20)&15,addr=op&1023;
				assert(reg<8 && !(op&0xfc00) && !(addr&1));
				for (unsigned lane=0;lane<32;lane++) {
					unsigned row=(addr&~3U)+lane/8,col=(lane&7)*2+((addr&2)!=0);
					if (code==0x70 && (mode==5 || mode==13)) {
						assert(ownerA && reg<2 && row<32 && defined[row] && wrote[row][col]); uint16_t v=physical[row][col];
						lr[reg][lane]=BlackholeRawSfpLoad(v,mode);
						if (wormholeRawByte) lr[reg][lane]&=0x8000007fU; // deliberately WRONG architecture negative control
					}
					else if (code==0x70 && mode==4) { assert(affineEligible && !reg && row>=32 && row<36); int v=dstInt[row][col]; lr[0][lane]=(v<0 || (!v && negativeDotZero)?0x80000000U:0)|unsigned(v<0?-v:v); }
					else if (code==0x70 && mode==3) {
						if (Accumulate && !reg) { assert(affineEligible && row>=32 && row<36 && Base::valid[row]); lr[0][lane]=Bits(Base::dst[row][col]); }
						else {
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
							if (row>=128 && row<192) {
								unsigned panel=(Base::matrixInstructions/MatrixPhasesPerPanel()-1)%16;
								unsigned first=128+(panel/2)*8+(reg==2?4:0);
								assert(BlockAffineActive() && affineEligible && (reg==1 || reg==2) && row>=first && row<first+4);
								if (blockImportMask!=65535 || !defined[Adj(row)] || !wrote[Adj(row)][col] || !wrote[Adj(row)+8][col]) throw BlockAffineHazard{};
							} else
#endif
							assert(affineEligible && (reg==1 || reg==2 || reg==5) && row>=96 && row<(Accumulate?112U:108U));
							lr[reg][lane]=FloatDst(row,col);
						}
					}
#ifdef PIONEER_Q38_NATIVE_OPERAND_STORE
					else if (code==0x72 && mode==5) {
						assert(!BFloat && ownerA && !reg && ((row>=128 && row<136) || (row>=144 && row<152)));
						unsigned logical=64+row-128-(row>=144?8:0),hi=Adj(logical),lo=hi+8;
						if (poisonNativeLow) physical[lo][col]=1;
						if (hi!=row || !defined[hi] || !wrote[lo][col] || physical[lo][col]) throw NativeStoreHazard{};
						assert(!decoded[logical-64][col]);
						uint32_t sm=lr[0][lane],mag=sm&0x7fffffff;
						assert(mag<=(type==8?128U:type==12?15U:32U) && sm!=0x80000000);
						// Independent Blackhole mode5:DST16 FP16-format sign,
						// ten magnitude bits,constant exponent16. Preserve the
						// already defined ZERO low physical half;NO new mapping.
						physical[row][col]=uint16_t(((sm>>16)&0x8000)|(mag<<5)|16);
						wrote[row][col]=true; decoded[logical-64][col]=true;
					}
#endif
					else if (code==0x72 && mode==3) {
						assert(defined[Adj(row)] && "partial FP32 store to undefined Dst row");
						if (BFloat && !affineEligible) assert(ownerA && !reg && row>=64 && row<80 && !(lr[reg][lane]&65535));
						else assert(affineEligible && !reg && row>=108 && row<112);
						StoreFloatDst(row,col,lr[reg][lane]);
						if (row>=64 && row<80) { assert(!decoded[row-64][col]); decoded[row-64][col]=true; }
					}
					else {
						assert(code==0x72 && mode==7); unsigned hi=Adj(row),lo=hi+8;
						assert(defined[hi] && "partial opaque store to undefined Dst row");
						if (!reg) { assert(ownerA && row>=64 && row<80 && hi>=128 && hi<160 && !decoded[row-64][col]); decoded[row-64][col]=true; }
						else assert(affineEligible && reg==7 && row>=96 && row<(Accumulate?112U:108U) && !lr[reg][lane]);
						physical[hi][col]=uint16_t(lr[reg][lane]>>16); physical[lo][col]=uint16_t(lr[reg][lane]); wrote[hi][col]=wrote[lo][col]=true;
					}
				}
#ifdef PIONEER_Q38_NATIVE_OPERAND_STORE
				if (code==0x72 && mode==5) nativeStoreOps++;
#endif
				drained=false; return;
			}
			if (code==8) {
				assert(ownerA && sources[mathBank]);
				unsigned source=(op>>17)&63,row=op&1023;
				assert(!(op&0x0081dc00) && (op&0x2000) && source==panelMoves*4 && row==64+source && source<=12);
				for (unsigned k=0;k<4;k++) for (unsigned col=0;col<16;col++) {
					assert(defined[Adj(row+k)] && decoded[row+k-64][col]);
					if (BFloat) {
						uint32_t value=FloatDst(row+k,col); assert(!(value&65535));
						a[source+k][col]=Float(value)+(corruptMove && !source && !k && !col?1:0); continue;
					}
					unsigned hi=Adj(row+k),lo=hi+8; assert(wrote[hi][col] && wrote[lo][col] && !physical[lo][col]);
					uint16_t v=physical[hi][col];
					unsigned src=((uint32_t(v)&0xffe0)<<3)|(v&31);
					uint16_t overlay=uint16_t(((src&0x40000)>>3)|((src&31)<<10)|((src>>8)&1023));
					a[source+k][col]=Operand16(overlay)+(corruptMove && !source && !k && !col?1:0);
				}
				panelMoves++; moveInstructions++; drained=false; return;
			}
			unsigned vc=(op>>8)&15,vd=(op>>4)&15,mod=op&15;
			for (unsigned lane=0;lane<32;lane++) {
				if (code==0x71) {
					unsigned reg=(op>>20)&15,mode=(op>>16)&15; assert(reg<8); auto& v=lr[reg][lane];
					if (!mode) v=(op&65535)<<16; else if (mode==10) v=(v&0xffff0000)|(op&65535);
					else { assert(mode==2); v=op&65535; } continue;
				}
				if (code==0x86 || code==0x85) {
					unsigned va=(op>>16)&15,vb=(op>>12)&15;
					assert(affineEligible && vd<8 && ((code==0x86 && vc==9 && mod==2) || (code==0x85 && va==10 && !mod)));
					auto value=[&](unsigned r) { assert(r<8 || r==9 || r==10); return r==9?0U:r==10?0x3f800000U:lr[r][lane]; };
					uint32_t c=value(vc); if (mod&2) c^=0x80000000;
					lr[vd][lane]=PioneerBlackholeFmaBits(value(va),value(vb),c); continue;
				}
				assert(vc<8 && vd<8); auto& v=lr[vd][lane];
				switch (code) {
				case 0x90: { assert(!mod && ((BFloat && !affineEligible)?vc==0:affineEligible && (vc==4 || (!BFloat && vc==0)))); uint32_t sm=lr[vc][lane],mag=sm&0x7fffffff;
					unsigned bound=(BFloat && !affineEligible)?128U:vc==0?262144U:2048U;
#ifdef PIONEER_Q38_TEST_GROUP32
					if (vc==4 && BlockAffineActive()) bound=4096;
#endif
#ifdef PIONEER_Q38_WORK_SKIP_DOT_CHECK
					// Full hardware SM32 cast for explicit Work numeric-fault
					// controls. Healthy dots remain independently bounded.
					if (Persistent && vc==0 && memory.at(0x74038)==1) bound=0x7fffffff;
#endif
					assert(mag<=bound && (mag || !sm)); v=Bits(float(int(mag)*((sm>>31)?-1:1))); break; }
				case 0x7c: assert(!mod); v=lr[vc][lane]; break;
				case 0x7e: { unsigned vb=(op>>12)&15; assert(mod==1 && vb<8); v=lr[vb][lane]&lr[vc][lane]; break; }
				case 0x8d: assert(!mod); v^=lr[vc][lane]; break;
				case 0x79: assert(mod==4 || mod==5); v=lr[vc][lane]+(mod==5?uint32_t(Sign(op>>12,12)):v); break;
				case 0x7a: { assert(mod==1 || mod==3); int shift=Sign(op>>12,12);
					if (shift>=0) v<<=unsigned(shift)&31;
					else if (mod==3) v=uint32_t(int32_t(v)>>(unsigned(-shift)&31)); else v>>=unsigned(-shift)&31; break; }
				default: fprintf(stderr,"UNMODELED VECTOR WORD %08x PC%08x\n",op,pc); assert(false);
				}
			}
			drained=false; return;
		}
		if (code==0x26) assert(
#ifdef PIONEER_Q38_TEST_PREDECODED_Q4
			(DirectQ4Panel()?panelMoves==0:panelMoves==4)
#else
			panelMoves==4
#endif
			&& sources[mathBank] && !rawPending);
		Base::Instruction(op);
		if (code==0x26) for (unsigned row=32;row<40;row++) defined[Adj(row)]=true;
		if ((op&~0x4000U)==0x26c00020) {
			sources[mathBank]=false; mathBank^=1; affineEligible=true; assert(unpackBank==mathBank);
			if (corruptDotRange) { if (BFloat) Base::dst[32][0]=float(corruptDotValue); else dstInt[32][0]=corruptDotValue; }
			if (BFloat && badFloatDot) Base::dst[32][0]=Float(badFloatDot);
			if (BFloat && negativeDotZero && !Base::dst[32][0]) Base::dst[32][0]=-0.0f;
		}
	}
	void Store(uint32_t address,uint32_t value) override {
		if (address==0xffe40000) instructionBufferStores++;
		if (recordRemaining && !Range(address,0x6e000,8192) && address!=0xffe40000) throw ReplayAccessFault{};
#ifdef PIONEER_Q38_TEST_PIPELINED_MATRIX
		if (!phaseQueue.empty() && !Range(address,0x6e000,8192) && address!=0xffe000fc && address!=0xffe40000)
			throw PendingMatrixAccess{};
#endif
		if (Range(address,0xffef0000,224*4)) {
			assert(!rawPending && drained); unsigned index=(address-0xffef0000)/4;
			if (!index) { assert(setupConfiguration==25); setupConfiguration=0; }
			if (setupConfiguration<25) {
				// Independent exact trace,not read from the candidate table.
				const unsigned indices[]={0,1,2,3,6,7,220,20,24,28,50,56,59,64,65,72,73,76,84,86,112,113,120,121,124};
				const uint32_t values[]={BFloat?0x42b5U:0x4231U,BFloat?0x203c8000U:0xa0220000U,0,BFloat?0U:0x10000U,0,0,0,0,0xffff,0x100,0x100,0x100000,0x100,
					BFloat?0x10001eU:0x100011U,16,BFloat?14U:1U,BFloat?0x13U:3U,0x7070,0x400040,0x100010,BFloat?0x100015U:0x100011U,8,BFloat?5U:1U,3,0x7091};
				assert(index==indices[setupConfiguration] && value==values[setupConfiguration]); setupConfiguration++;
			}
			config[index]=value; configWrites++; return;
		}
		assert(!Range(address,0x70710,512)); // NO decoded operand staging/copy
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
		if (Range(address,0x97000,1024)) {
			assert(BlockAffineActive() && !(address&3) && !pending && !rawPending && drained && (value&0x7f800000)!=0x7f800000);
			if (address==0x97000) { assert(!blockScratchWords || blockScratchWords==256); blockScratchWords=0; blockImportMask=0; blockImportAttempts=0; }
			unsigned group=blockScratchWords/32,lane=(blockScratchWords%32)/2,kind=blockScratchWords%2;
			assert(blockScratchWords<256 && address==0x97000+(group*32+kind*16+lane)*4);
			memory[address]=value; blockScratchWords++; return;
		}
#endif
#ifdef PIONEER_Q38_WORK_INPUT_SUM_CACHE
		if (Range(address,0x95000,4352)) {
			assert(Persistent && !BFloat && Accumulate && groups>1 && memory.at(0x74038)==1
				&& Range(address,0x95000,(inputs/16)*4) && !(address&3) && !pending && drained
				&& int(value)>=-2048 && int(value)<=2048);
			memory[address]=value; return;
		}
#endif
		if (BFloat && Range(address,0x95000,34816)) {
			assert(Range(address,0x95000,inputs*2) && !(address&3));
			if (address==0x95000) { assert(!cacheNext || cacheNext==inputs*2); cacheNext=0; cacheJobs++; }
			assert(address==0x95000+cacheNext); uint32_t raw=Base::Load(0xa0000+cacheNext),expected=0;
			for (unsigned lane=0;lane<2;lane++) { uint16_t v=uint16_t(raw>>(lane*16)); assert(PioneerInt8OperandValid(v));
				int exact=int(v&1023)*((v&0x8000)?-1:1); expected|=(Bits(float(exact))>>16)<<(lane*16); }
			assert(value==expected); memory[address]=value; cacheNext+=4; return;
		}
		if (BFloat && Range(address,0x70920,32)) {
			assert(drained && !(address&3)); for (unsigned lane=0;lane<2;lane++) {
				float f=Float(uint32_t(uint16_t(value>>(lane*16)))<<16); assert(f>=-128 && f<=127 && f==float(int(f))); }
			l1[Index(address)]=value; return;
		}
		if (Range(address,0x91200,128)) metadataWrites++;
		Base::Store(address,value);
	}
	uint32_t Load(uint32_t address) override {
		if (Range(address,0xffbd8800,64)) dotProbeReads++;
		if (recordRemaining && !Range(address,0x6e000,8192)) throw ReplayAccessFault{};
#ifdef PIONEER_Q38_TEST_PIPELINED_MATRIX
		if (!phaseQueue.empty() && !Range(address,0x6e000,8192) && address!=0xffe000fc)
			throw PendingMatrixAccess{};
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS
		if (Range(address,0xffbd9800,1024)) {
			assert(type==8 && config[3]==0x10000 && !rawPending && drained && sourceProbeRows==16);
			unsigned i=(address-0xffbd9800)/4; sourceProbeReads++;
			uint32_t bits=FloatDst(96+i/16,i%16);
			return bits&0x80000000?0U-(bits&0x7fffffff):bits;
		}
		if (Range(address,0xffbd9000,1024)) {
			assert(type==8 && config[3]==0x10000 && !rawPending && drained && panelMoves==4);
			unsigned i=(address-0xffbd9000)/4; decodedProbeReads++;
			if (corruptDecodedProbe && i==37) return 0x13572468;
			uint32_t bits=FloatDst(64+i/16,i%16);
			return bits&0x80000000?0U-(bits&0x7fffffff):bits;
		}
#endif
#ifdef PIONEER_Q38_WORK_INPUT_SUM_CACHE
		if (Range(address,0x95000,4352)) {
			assert(Persistent && !BFloat && Accumulate && groups>1 && memory.at(0x74038)==1
				&& Range(address,0x95000,(inputs/16)*4) && !(address&3) && !pending && drained);
			return memory.at(address);
		}
#endif
		if (BFloat && Range(address,0x95000,34816)) { assert(cacheNext==inputs*2 && Range(address,0x95000,inputs*2)); return memory.at(address); }
		if (Range(address,0xffef0000,224*4)) { assert(!rawPending && drained); return config[(address-0xffef0000)/4]; }
		if (Range(address,0xffbd9b00,64)) { assert(affineEligible && !config[3] && !rawPending && drained); outputReads++; return FloatDst(108,(address-0xffbd9b00)/4); }
		return Base::Load(address);
	}
	bool RunVector() {
		while (pc!=0x72038 && pc!=0x72058) StepBlock();
		if (pc==0x72058) { assert(l1[Index(0x17ffc8)]==0x46563851); return false; }
		assert(l1[Index(0x17ffc8)]==0x31563851 && minStack>=0x6e000 && !rawPending
			&& !sources[0] && !sources[1] && unpackBank==mathBank && drained);
		return l1[Index(0x17ffc4)]==groups*(inputs/16) && requests==tiles.size() && requests==responses && !pending;
	}
};
#ifdef PIONEER_Q38_TEST_ACCUMULATOR
using VectorRowsMachine=VectorRows<RowsMachine,false,true,true>;
#elif defined(PIONEER_Q38_TEST_INT_ACCUMULATOR)
using VectorRowsMachine=VectorRows<RowsMachine,false,false,true>;
#else
#ifdef PIONEER_Q38_TEST_BFLOAT
using VectorRowsMachine=VectorRows<RowsMachine,false,true>;
#else
using VectorRowsMachine=VectorRows<RowsMachine>;
#endif
#endif
#ifndef PIONEER_Q38_VECTOR_ROWS_MODEL_ONLY
int main(int argc,char** argv)
{
	assert(argc>=2 && argc<=4);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		VectorRowsMachine m(argv[1],type,k,k==17408?3:1); m.delay=3;
		assert(m.RunVector() && m.CorrectRows());
		for (unsigned phase=0;phase<4;phase++) assert(m.phaseVisits[phase]==(phase<m.MatrixPhasesPerPanel()?m.groups*(m.inputs/16):0));
		printf("OFFLINE VECTOR actual type%u K%u N%u steps%u rawBytes%u SFPUops%u metadataWrites%u exact FP32+double PASS\n",type,k,m.groups*16,m.steps,m.rawReads,m.vectorOps,m.metadataWrites);
		if (argc==4) {
			RowsMachine scalar(argv[3],type,k,m.groups); scalar.delay=3;
			assert(scalar.RunRows() && scalar.CorrectRows());
			for (unsigned row=0;row<m.groups*16;row++) assert(m.buffers[4096+row]==scalar.buffers[4096+row]);
			printf("OFFLINE SAME synthetic workload scalar_RV_steps%u vector_RV_steps%u ratio%.6f;INSTRUCTION_COUNTS_NOT_NATIVE_SPEED\n",scalar.steps,m.steps,double(scalar.steps)/m.steps);
		}
	}
	for (unsigned type:{8U,12U,14U}) { VectorRowsMachine m(argv[1],type,5120,1); m.corruptMove=true; assert(m.RunVector() && !m.CorrectRows()); }
	for (unsigned type:{8U,12U,14U}) { VectorRowsMachine m(argv[1],type,5120,1);
#ifdef PIONEER_Q38_TEST_BFLOAT
		m.dropMatrix=true;
#else
		m.dropLowFidelity=true;
#endif
		assert(m.RunVector() && !m.CorrectRows()); }
	for (unsigned type:{8U,12U,14U}) { VectorRowsMachine m(argv[1],type,5120,1); m.corruptDotRange=true; assert(!m.RunVector()); }
	for (unsigned type:{8U,12U,14U}) { VectorRowsMachine m(argv[1],type,5120,1); m.negativeDotZero=true; assert(m.RunVector() && m.CorrectRows()); }
#ifdef PIONEER_Q38_TEST_BFLOAT
	for (unsigned bad:{0x7f800000U,0xff800000U,0x7fc00001U,0xffc01234U}) {
		VectorRowsMachine m(argv[1],12,5120,1); m.badFloatDot=bad; assert(!m.RunVector());
	}
	for (unsigned k:{5120U,17408U}) for (unsigned at:{0U,k/2,k-1}) for (unsigned bad:{0U,0xffffU,0x4080U,0xc081U}) {
		VectorRowsMachine m(argv[1],12,k,1); unsigned address=0xa0000+(at/2)*4,shift=(at%2)*16;
		m.memory[address]=(m.memory.at(address)&~(65535U<<shift))|(bad<<shift);
		assert(!m.RunVector() && !m.matrixInstructions && !m.requests);
		for (unsigned row=0;row<4096;row++) assert(m.buffers[4096+row]==0x7fc00000);
	}
#endif
	for (unsigned fault:{1U,20U,60U}) {
		VectorRowsMachine m(argv[1],12,5120,3); m.faultAt=fault;
		assert(!m.RunVector() && m.pending && m.requests==fault && m.responses==fault-1);
		unsigned requests=m.requests; for (unsigned i=0;i<1000;i++) m.StepBlock(); assert(m.requests==requests && m.pending);
	}
	if (argc>=3) {
		FILE* f=fopen(argv[2],"rb"); assert(f); PioneerQwen38Gguf model; assert(model.Open(f));
		for (const char* name:{"blk.0.attn_qkv.weight","blk.0.ffn_down.weight","output.weight"}) {
			const auto* t=model.Find(name); assert(t);
			for (unsigned end=0;end<2;end++) {
				VectorRowsMachine m(argv[1],t->type,unsigned(t->inputs),3);
				m.BindRows(model,unsigned(t-model.Tensors().data()),end?unsigned(t->outputs)-48:0);
				assert(m.RunVector() && m.CorrectRows());
				printf("OFFLINE VECTOR REAL %s K%u firstRow%u exact FP32+double PASS\n",name,m.inputs,end?unsigned(t->outputs)-48:0);
			}
		}
		assert(model.Stable()); fclose(f);
	}
#ifdef PIONEER_Q38_TEST_ACCUMULATOR
	puts("PASS OFFLINE ACTUAL resident SFPU accumulator:EXACT all-K/real-edge FP32+double,disjoint retained Dst metadata/accumulators,initialized group padding,no scalar per-panel affine/output staging,ALL per-panel dot bounds/NaN guards,final finite export,activation-cache lifetime/UNKNOWN;NO native/driver/cycle speed qualification");
#else
#ifdef PIONEER_Q38_TEST_BFLOAT
	puts("PASS OFFLINE ACTUAL BF16-row image:EXACT all-K Q8/Q4/Q6 FP32+double math,Q8 TWO/Q4+Q6 ONE matrix phases,whole-job private activation cache overwritten before read,raw/Dst/matrix ownership,dot bounds/canonical zero,UNKNOWN inert park;NOT native/persistent/coordinator/cycle/speed qualification");
#else
	puts("PASS OFFLINE ACTUAL vector-row image:exact all-K Q8/Q4/Q6 math,raw Dst/vector/Matrix source-bank ownership,zero scalar operand writes,all4 fidelity phases,source bounds,UNKNOWN inert park;NOT native/persistent/coordinator/cycle/speed qualification");
#endif
#endif
}
#endif
