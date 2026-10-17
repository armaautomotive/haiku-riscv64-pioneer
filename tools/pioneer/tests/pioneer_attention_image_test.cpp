/* SPDX-License-Identifier: MIT */
// Executes the actual linked RV32IMF bytes, not the C++ source as host code.
// Strict memory/MMIO model; no device API, no hardware operation.
#include "../PioneerAttentionFixture.h"
#include "../PioneerBlackholeFma.h"
#include "../PioneerFloat16.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeAttentionTileImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeResidentAttentionImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeCoordinatedAttentionImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeCoordinatedGluImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeCachedQueryGluImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholePullInputGluImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRopeGluImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeSfpuDivideProbeImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeVectorGluImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeInlineAttentionImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeOptimizedAttentionImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeFiniteHalfAttentionImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTokenLaneAttentionImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeFastSoftmaxImage.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

static std::vector<uint8_t> Read(const std::string& path)
{
	std::ifstream f(path,std::ios::binary); assert(f.good());
	return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),{});
}
static uint32_t Word(const std::vector<uint8_t>& b,unsigned at)
{
	assert(at+4<=b.size()); return b[at]|(uint32_t(b[at+1])<<8)|(uint32_t(b[at+2])<<16)|(uint32_t(b[at+3])<<24);
}
static int32_t Sign(uint32_t value,unsigned width) { return int32_t(value<<(32-width))>>(32-width); }
static float F(uint32_t value) { return PioneerAttentionFloat(value); }
static uint32_t B(float value) { return PioneerAttentionBits(value); }
static float Flush(float value) { return (B(value)&0x7f800000)?value:F(B(value)&0x80000000); }

struct Image {
	std::vector<uint8_t> boot,code,rope;
	std::map<std::string,uint32_t> symbols;
	bool resident,coordinated,glu,cachedQuery=false,inputPull=false,parallelRope=false,sfpuDivideProbe=false,vectorGlu=false,fastSoftmax=false,tokenLanes=false,inlineSfpu=false,optimizedAttention=false,finiteHalf=false;
	explicit Image(const char* path,bool kv=false,bool group=false,bool activation=false):boot(Read(std::string(path)+"/boot.bin")),code(Read(std::string(path)+"/code.bin")),resident(kv),coordinated(group),glu(activation) {
		assert(!glu || coordinated);
		std::ifstream file(std::string(path)+"/program.symbols"); assert(file.good());
		std::string name; char type; uint32_t address;
		while (file>>std::hex>>address>>type>>name) symbols[name]=address;
		assert(boot.size()==4 && Word(boot,0)==0x0006c06f && code.size()<=8192);
		assert(Word(boot,0)==TT_ATTENTION_TILE_BOOT);
		sfpuDivideProbe=getenv("PIONEER_SFPU_DIVIDE_PROBE")!=nullptr;
		if (sfpuDivideProbe) {
			assert(!resident && !coordinated && code.size()==TT_SFPU_DIVIDE_PROBE_IMAGE_WORDS*4);
			for (unsigned w=0;w<TT_ATTENTION_TILE_CODE_WORDS;w++) assert(BlackholeSfpuDivideProbeCodeWord(w)==(w<code.size()/4?Word(code,w*4):0));
		} else if (!coordinated) {
			assert(code.size()==unsigned(resident?TT_RESIDENT_ATTN_IMAGE_WORDS:TT_ATTENTION_TILE_IMAGE_WORDS)*4);
			for (unsigned w=0;w<TT_ATTENTION_TILE_CODE_WORDS;w++)
				assert((resident?BlackholeResidentAttentionCodeWord(w):BlackholeAttentionTileCodeWord(w))==(w<code.size()/4?Word(code,w*4):0));
		} else {
			inputPull=glu && getenv("PIONEER_COORDINATED_PULL_INPUT");
			cachedQuery=glu && (inputPull || getenv("PIONEER_ATTENTION_CACHED_QUERY"));
			parallelRope=inputPull && getenv("PIONEER_ATTENTION_PARALLEL_ROPE");
			vectorGlu=parallelRope && getenv("PIONEER_GLU_VECTOR");
			fastSoftmax=vectorGlu && getenv("PIONEER_ATTENTION_FAST_SOFTMAX");
			tokenLanes=vectorGlu && getenv("PIONEER_ATTENTION_TOKEN_LANES");
			inlineSfpu=vectorGlu && getenv("PIONEER_ATTENTION_INLINE_SFPU");
			optimizedAttention=inlineSfpu && getenv("PIONEER_ATTENTION_OPTIMIZED");
			finiteHalf=optimizedAttention && getenv("PIONEER_ATTENTION_FAST_HALF");
			assert(!inlineSfpu || (!tokenLanes && !fastSoftmax));
			assert(!(tokenLanes && fastSoftmax));
			assert(code.size()==unsigned(finiteHalf?TT_FINITE_HALF_ATTN_IMAGE_WORDS:optimizedAttention?TT_OPTIMIZED_ATTN_IMAGE_WORDS:inlineSfpu?TT_INLINE_ATTN_IMAGE_WORDS:tokenLanes?TT_TOKEN_LANE_ATTN_IMAGE_WORDS:fastSoftmax?TT_FAST_SOFTMAX_IMAGE_WORDS:vectorGlu?TT_VECTOR_GLU_IMAGE_WORDS:parallelRope?TT_ROPE_GLU_IMAGE_WORDS:inputPull?TT_PULL_INPUT_GLU_IMAGE_WORDS:cachedQuery?TT_CACHED_QUERY_GLU_IMAGE_WORDS:glu?TT_COORDINATED_GLU_IMAGE_WORDS:TT_COORDINATED_ATTENTION_IMAGE_WORDS)*4);
			for (unsigned w=0;w<TT_COORDINATED_ATTENTION_CODE_WORDS;w++)
				assert((finiteHalf?BlackholeFiniteHalfAttentionCodeWord(w):optimizedAttention?BlackholeOptimizedAttentionCodeWord(w):inlineSfpu?BlackholeInlineAttentionCodeWord(w):tokenLanes?BlackholeTokenLaneAttentionCodeWord(w):fastSoftmax?BlackholeFastSoftmaxCodeWord(w):vectorGlu?BlackholeVectorGluCodeWord(w):parallelRope?BlackholeRopeGluCodeWord(w):inputPull?BlackholePullInputGluCodeWord(w):cachedQuery?BlackholeCachedQueryGluCodeWord(w):glu?BlackholeCoordinatedGluCodeWord(w):BlackholeCoordinatedAttentionCodeWord(w))==(w<code.size()/4?Word(code,w*4):0));
			if (parallelRope) {
				rope=Read(std::string(path)+"/rope.bin"); assert(rope.size()==unsigned(finiteHalf?TT_FINITE_HALF_ATTN_EXTRA_WORDS:optimizedAttention?TT_OPTIMIZED_ATTN_EXTRA_WORDS:inlineSfpu?TT_INLINE_ATTN_EXTRA_WORDS:tokenLanes?TT_TOKEN_LANE_ATTN_EXTRA_WORDS:fastSoftmax?TT_FAST_SOFTMAX_EXTRA_WORDS:vectorGlu?TT_VECTOR_GLU_EXTRA_WORDS:TT_ROPE_GLU_EXTRA_WORDS)*4 && rope.size()<=0x1fe0);
				assert(symbols.at("attention_rope_end")==0x74020+rope.size());
				for (unsigned w=0;w<TT_ROPE_GLU_EXTRA_CAPACITY;w++) assert((finiteHalf?BlackholeFiniteHalfAttentionExtraWord(w):optimizedAttention?BlackholeOptimizedAttentionExtraWord(w):inlineSfpu?BlackholeInlineAttentionExtraWord(w):tokenLanes?BlackholeTokenLaneAttentionExtraWord(w):fastSoftmax?BlackholeFastSoftmaxExtraWord(w):vectorGlu?BlackholeVectorGluExtraWord(w):BlackholeRopeGluExtraWord(w))==(w<rope.size()/4?Word(rope,w*4):0));
			}
		}
		assert(symbols.at("attention_entry")==0x72000 && symbols.at("attention_image_end")==0x72000+code.size());
		assert(symbols.at("attention_done")==TT_ATTENTION_TILE_DONE
			&& symbols.at("attention_park")==unsigned(TT_ATTENTION_TILE_PARK)+(coordinated?12:0));
		if (sfpuDivideProbe) {
			assert(!symbols.count("attention_exact_begin") && !symbols.count("attention_exact_end"));
			symbols["attention_exact_begin"]=symbols.at("attention_image_end");
		} else if (!coordinated) assert(symbols.at("attention_exact_end")-symbols.at("attention_exact_begin")==37*4);
		else { // Standalone exact half products need no general FMA table.
			assert(!symbols.count("attention_exact_begin") && !symbols.count("attention_exact_end"));
			symbols["attention_exact_begin"]=symbols.at("attention_image_end");
		}
	}
};

struct Machine {
	const Image& image;
	std::vector<uint32_t> mem;
	uint32_t x[32]={},fp[32]={},lanes[11][32]={},dst[12][16]={},config[224]={},thread[68]={};
	bool flags[32]={},conditional=false,drained=true,waiting=false;
	uint32_t pc=0x6000,ack=0;
	unsigned tokens,steps=0,mads=0,drains=0,mmioWrites=0,outputWrites=0,minStack=0x78000,queryCacheWrites=0,directInstructions=0;
	bool dropMultiply=false,dropValue=false,omitAccumulation=false,dropAck=false,transposeLanes=false;
	explicit Machine(const Image& i,unsigned n):image(i),mem(0x180000/4,0xdeadbeef),tokens(n) {
		mem[0x6000/4]=Word(i.boot,0);
		for (unsigned at=0;at<i.code.size();at+=4) mem[(0x72000+at)/4]=Word(i.code,at);
		for (unsigned at=0;at<i.rope.size();at+=4) mem[(0x74020+at)/4]=Word(i.rope,at);
		mem[0x74000/4]=0x314e5441; mem[0x74004/4]=n; mem[0x74008/4]=1; mem[0x7400c/4]=0;
		for (unsigned lane=0;lane<32;lane++) lanes[10][lane]=B(1);
		for (unsigned j=0;j<3;j++) mem[0x17ffc0/4+j]=0;
	}
	void Inputs(const std::vector<float>& q,const std::vector<uint16_t>& k,const std::vector<uint16_t>& v) {
		assert(q.size()==256 && k.size()==tokens*128 && k.size()==v.size());
		for (unsigned j=0;j<q.size();j++) mem[0x90000/4+j]=B(q[j]);
		for (unsigned j=0;j<k.size();j+=2) {
			mem[0x92000/4+j/2]=k[j]|(uint32_t(k[j+1])<<16);
			mem[0xa2000/4+j/2]=v[j]|(uint32_t(v[j+1])<<16);
		}
	}
	static bool Range(uint32_t a,uint32_t start,uint32_t size) { return a>=start && a-start<size; }
	void Sfp(uint32_t op) {
		unsigned code=op>>24;
		if (code==0xb2) { unsigned i=(op>>16)&255; assert(i<68); thread[i]=op&65535; }
		else if (op==0x3700000f) { assert(thread[0]==0); }
		else if (op==0xa2101816) { assert(!ack); waiting=true; }
		else if (op==0x4500017e) { assert(waiting && !ack); if (!dropAck) ack=1; waiting=false; drained=true; drains++; return; }
		else if (code==0x70 || code==0x72) {
			unsigned reg=(op>>20)&15,mode=(op>>16)&15,address=op&1023;
			assert(reg<8 && !(address&1) && !(op&0xfc00) && !thread[1] && !thread[36]);
			assert(config[3]==0 && config[6]==0 && config[7]==0);
			for (unsigned lane=0;lane<32;lane++) {
				assert(!conditional);
				unsigned row=(address&~3U)+lane/8,col=(lane&7)*2+((address&2)!=0);
				assert(row<12);
				if (code==0x72) { assert(mode==3 && row>=8); dst[row][col]=B(Flush(F(lanes[reg][lane]))); }
				else if (mode==11) lanes[reg][lane]=0;
				else { assert(mode==3 && row<8); lanes[reg][lane]=dst[row][col]; }
			}
		} else {
			unsigned a=(op>>16)&15,b=(op>>12)&15,c=(op>>8)&15,d=(op>>4)&15,mod=op&15;
			assert(d<8 || code==0x71);
			for (unsigned lane=0;lane<32;lane++) {
				if (code==0x8a) { assert(mod==2); conditional=(op&0x1000)!=0; flags[lane]=true; continue; }
				if (conditional && !flags[lane]) continue;
				uint32_t& r=lanes[code==0x71?0:d][lane];
				switch (code) {
					case 0x71: { unsigned reg=(op>>20)&15,mode=(op>>16)&15; assert(reg<8);
						if (!mode) lanes[reg][lane]=(op&65535)<<16;
						else if (mode==10) lanes[reg][lane]=(lanes[reg][lane]&0xffff0000)|(op&65535);
						else { assert(mode==2); lanes[reg][lane]=op&65535; } break; }
					case 0x84: case 0x85: {
						assert(a<8 || a==10); assert(b<8 || b==10); assert(c<8 || c==9); assert(!(mod&~3));
						float av=F(lanes[a][lane]),bv=F(lanes[b][lane]),cv=F(lanes[c][lane]);
						if (mod&1) av=-av; if (mod&2) cv=-cv;
						if (dropMultiply && code==0x84) av=0;
						if (omitAccumulation && op==0x850a2450) lanes[2][lane]=0;
						r=B(PioneerBlackholeFma(av,bv,cv)); mads++; break; }
					case 0x7e: assert(mod==1 && b<8 && c<8); r=lanes[b][lane]&lanes[c][lane]; break;
					case 0x7c: assert(!mod && c<8); r=lanes[c][lane]; break;
					case 0x8d: assert(!mod && c<8); r^=lanes[c][lane]; break;
					case 0x7d: assert(mod==1 && c<8); r=lanes[c][lane]&0x7fffffff; break;
					case 0x7a: { assert(mod==1 || mod==3); int amount=Sign((op>>12)&4095,12);
						if (amount>=0) r<<=unsigned(amount)&31;
						else if (mod&2) r=uint32_t(int32_t(r)>>(unsigned(-amount)&31));
						else r>>=unsigned(-amount)&31; break; }
					case 0x79: assert(mod==4 || mod==5); r=lanes[c][lane]+((mod&1)?uint32_t(Sign((op>>12)&4095,12)):r); break;
					case 0x7b: assert(conditional && (mod==2 || mod==6)); flags[lane]=mod==2?lanes[c][lane]!=0:lanes[c][lane]==0; break;
					default: fprintf(stderr,"unqualified SFPU %08x\n",op); assert(false);
				}
			}
		}
		drained=false;
	}
	virtual ~Machine() = default;
	virtual uint32_t Load(uint32_t a) {
		assert(!(a&3));
		if (a==0xffe000fc) return ack;
		if (Range(a,0xffef0000,224*4)) { assert(drained); return config[(a-0xffef0000)/4]; }
		if (Range(a,0xffbd8000,12*64)) { assert(drained && !config[3]); return dst[(a-0xffbd8000)/64][((a-0xffbd8000)/4)%16]; }
		assert(Range(a,0x72000,unsigned(image.code.size())) || (image.parallelRope && Range(a,0x74020,unsigned(image.rope.size()))) || Range(a,0x74000,16)
			|| Range(a,0x76000,8192) || Range(a,0x90000,1024) || (image.cachedQuery && Range(a,0x90800,1024))
			|| (image.inputPull && Range(a,0x91800,2048))
			|| Range(a,0x92000,tokens*256) || Range(a,0xa2000,tokens*256) || Range(a,0x17ffc0,12));
		return mem[a/4];
	}
	virtual void Store(uint32_t a,uint32_t value) {
		assert(!(a&3));
		if (a==0xffe40000) { mmioWrites++; Sfp(value); return; }
		if (a==0xffe000fc) { assert(!value); mmioWrites++; ack=0; return; }
		if (Range(a,0xffef0000,224*4)) {
			assert(drained); unsigned i=(a-0xffef0000)/4;
			assert(i==0 || i==1 || i==2 || i==3 || i==6 || i==7 || i==220);
			const uint32_t expected=i==0?0x4231:i==1?0x20220000:0;
			assert(value==expected); config[i]=value; mmioWrites++; return;
		}
		if (Range(a,0xffbd8000,8*64)) {
			assert(drained && !config[3]); unsigned row=(a-0xffbd8000)/64,col=((a-0xffbd8000)/4)%16;
			assert(!(col&1)); if (transposeLanes) row=(row/4)*4+(3-row%4);
			dst[row][col]=dropValue?0:value; mmioWrites++; return;
		}
		assert(Range(a,0x76000,8192) || Range(a,0xb2000,1024) || Range(a,0x17ffc0,12) || (image.cachedQuery && Range(a,0x90800,1024)));
		if (Range(a,0xb2000,1024)) outputWrites++;
		if (image.cachedQuery && Range(a,0x90800,1024)) queryCacheWrites++;
		if (Range(a,0x76000,8192) && a<minStack) minStack=a;
		mem[a/4]=value;
	}
	void Run(bool fault=false) {
		uint32_t park=image.coordinated?0x71200:image.symbols.at("attention_park"),faultPark=image.symbols.at("attention_fault_park")+28;
		while (pc!=(fault?faultPark:park)) {
			assert(++steps<(image.resident?500000000U:30000000U));
			assert(pc==0x6000 || Range(pc,0x72000,image.symbols.at("attention_exact_begin")-0x72000) || (image.parallelRope && Range(pc,0x74020,unsigned(image.rope.size()))));
			uint32_t i=mem[pc/4],next=pc+4,r=0;
			// Standard32-bit RISC instructions end in11. The SAME strict
			// SFPU decoder handles only this frozen image's inline words;
			// no instruction semantics or permitted opcode is added.
			if (image.inlineSfpu && (i&3)!=3) { directInstructions++; Sfp((i>>2)|(i<<30)); pc=next; continue; }
			unsigned op=i&127,d=(i>>7)&31,s=(i>>15)&31,t=(i>>20)&31,f=(i>>12)&7,hi=i>>25;
			int32_t imm=Sign(i>>20,12); bool write=false;
			switch (op) {
				case 0x37: r=i&0xfffff000; write=true; break;
				case 0x17: r=pc+(i&0xfffff000); write=true; break;
				case 0x13:
					switch (f) {
						case 0: r=x[s]+imm; break; case 2: r=int32_t(x[s])<imm; break; case 3: r=x[s]<uint32_t(imm); break;
						case 4: r=x[s]^imm; break; case 6: r=x[s]|imm; break; case 7: r=x[s]&imm; break;
						case 1: assert(!hi); r=x[s]<<t; break;
						case 5: assert(hi==0 || hi==32); r=hi?uint32_t(int32_t(x[s])>>t):x[s]>>t; break;
						default: assert(false);
					} write=true; break;
				case 0x33:
					if (hi==1) { assert(f==0); r=uint32_t(uint64_t(x[s])*x[t]); }
					else { assert(hi==0 || hi==32);
						switch (f) {
							case 0: r=hi?x[s]-x[t]:x[s]+x[t]; break; case 1: assert(!hi); r=x[s]<<(x[t]&31); break;
							case 2: assert(!hi); r=int32_t(x[s])<int32_t(x[t]); break; case 3: assert(!hi); r=x[s]<x[t]; break;
							case 4: assert(!hi); r=x[s]^x[t]; break; case 6: assert(!hi); r=x[s]|x[t]; break; case 7: assert(!hi); r=x[s]&x[t]; break;
							case 5: r=hi?uint32_t(int32_t(x[s])>>(x[t]&31)):x[s]>>(x[t]&31); break; default: assert(false);
						}
					} write=true; break;
				case 3: { uint32_t a=x[s]+imm; r=Load(a&~3U)>>(8*(a&3));
					if (f==2) assert(!(a&3));
					else if (f==5) { assert(!(a&1)); r&=65535; }
					else assert(false); write=true; break; }
				case 0x23: assert(f==2); imm=Sign((hi<<5)|d,12); Store(x[s]+imm,x[t]); break;
				case 0x63: { imm=Sign(((i>>31)<<12)|(((i>>7)&1)<<11)|(((i>>25)&63)<<5)|(((i>>8)&15)<<1),13); bool take=false;
					switch(f) { case 0: take=x[s]==x[t]; break; case 1: take=x[s]!=x[t]; break;
						case 4: take=int32_t(x[s])<int32_t(x[t]); break; case 5: take=int32_t(x[s])>=int32_t(x[t]); break;
						case 6: take=x[s]<x[t]; break; case 7: take=x[s]>=x[t]; break; default: assert(false); }
					if (take) next=pc+imm; break; }
				case 0x6f: imm=Sign(((i>>31)<<20)|(((i>>12)&255)<<12)|(((i>>20)&1)<<11)|(((i>>21)&1023)<<1),21); r=next; next=pc+imm; write=true; break;
				case 0x67: assert(!f); r=next; next=(x[s]+imm)&~1U; write=true; break;
				case 7: assert(f==2); fp[d]=Load(x[s]+imm); break;
				case 0x27: assert(f==2); imm=Sign((hi<<5)|d,12); Store(x[s]+imm,fp[t]); break;
				case 0x53: {
					float a=Flush(F(fp[s])),b=Flush(F(fp[t]));
					switch (hi) {
						case 0: assert(f==7 || !f); fp[d]=B(Flush(a+b)); break;
						case 4: assert(f==7 || !f); fp[d]=B(Flush(a-b)); break;
						case 8: assert(f==7 || !f); fp[d]=B(Flush(a*b)); break;
						case 0x10: assert(f<=2); fp[d]=f==0?((fp[s]&0x7fffffff)|(fp[t]&0x80000000)):f==1?((fp[s]&0x7fffffff)|((~fp[t])&0x80000000)):(fp[s]^(fp[t]&0x80000000)); break;
						case 0x50: assert(f<=2); r=f==0?a<=b:f==1?a<b:a==b; write=true; break;
						case 0x68: assert(t<=1); fp[d]=B(t?float(x[s]):float(int32_t(x[s]))); break;
						case 0x70: assert(!t && !f); r=fp[s]; write=true; break;
						case 0x78: assert(!t && !f); fp[d]=x[s]; break;
						default: fprintf(stderr,"unqualified FP op %08x pc %08x\n",i,pc); assert(false);
					} break; }
				case 0x0f: assert(i==0x0ff0000f); break;
				default: fprintf(stderr,"unqualified RISC op %08x pc %08x\n",i,pc); assert(false);
			}
			if (write && d) {
				x[d]=r;
				if (d==2) { assert(r>=0x76000 && r<=0x78000 && !(r&15)); minStack=std::min(minStack,unsigned(r)); }
			}
			x[0]=0; pc=next;
		}
		if (!image.coordinated || fault) assert(mem[pc/4]==0x0000006f);
		if (fault) assert(mem[0x17ffc8/4]==0x464e5441);
		else assert(drained && mem[0x17ffc0/4]==image.symbols.at("attention_done") && mem[0x17ffc8/4]==0x314e5441);
	}
	double Error(const std::vector<float>& expected) const {
		double delta=0,norm=0;
		assert(expected.size()==256);
		for (unsigned j=0;j<256;j++) { float a=F(mem[0xb2000/4+j]); assert(isfinite(a));
			double d=double(a)-expected[j]; delta+=d*d; norm+=double(expected[j])*expected[j]; }
		return sqrt(delta/(norm?norm:1));
	}
};

static void Reference(const std::vector<float>& q,const std::vector<uint16_t>& k,const std::vector<uint16_t>& v,unsigned n,std::vector<float>& out)
{
	for (unsigned h=0;h<2;h++) {
		std::vector<double> scores(n); double maximum=-INFINITY,total=0;
		for (unsigned t=0;t<n;t++) { double sum=0;
			for (unsigned d=0;d<128;d++) sum+=double(float(_Float16(q[h*128+d]))*PioneerAttentionHalf(k[t*128+d]));
			scores[t]=float(float(sum)*0.0883883461356163f); maximum=fmax(maximum,scores[t]); }
		for (auto& value:scores) { value=expf(float(value-maximum)); total+=value; }
		for (unsigned d=0;d<128;d++) { double sum=0;
			for (unsigned t=0;t<n;t++) sum+=double(float(_Float16(float(scores[t]*float(1.0/total))))*PioneerAttentionHalf(v[t*128+d])); out[h*128+d]=float(sum); }
	}
}

int main(int argc,char** argv)
{
	setvbuf(stdout,NULL,_IONBF,0);
	assert(argc>=2); Image image(argv[1]);
	unsigned maxStack=0; double maximumError=0;
	for (unsigned n:{1U,2U,3U,7U,16U,31U,32U,64U,128U,192U,256U}) {
		std::vector<float> q(256),expected(256); std::vector<uint16_t> k(n*128),v(n*128);
		for (unsigned j=0;j<q.size();j++) q[j]=float(sin(j*.713)*1.7);
		for (unsigned j=0;j<k.size();j++) { k[j]=PioneerFloat16(float(sin(j*.429)*1.3)); v[j]=PioneerFloat16(float(cos(j*.139)*2)); }
		Reference(q,k,v,n,expected);
		Machine m(image,n); m.Inputs(q,k,v); m.Run(); double error=m.Error(expected);
		assert(m.mem[0x17ffc4/4]==n && m.outputWrites==256 && error<.00002 && m.mads>n*2*128);
		maxStack=std::max(maxStack,0x78000-m.minStack); maximumError=fmax(maximumError,error);
		printf("OFFLINE assembled attention n=%u steps=%u stack=%u vs-double-L2=%.9g PASS\n",n,m.steps,0x78000-m.minStack,error);
		if (n==3) {
			for (unsigned fault=0;fault<5;fault++) {
				Machine bad(image,n); bad.Inputs(q,k,v);
				bad.dropMultiply=fault==0; bad.dropValue=fault==1; bad.omitAccumulation=fault==2; bad.transposeLanes=fault==3;
				if (fault==4) for (unsigned j=0;j<64;j++) bad.mem[0x92000/4+j]^=0x80008000;
				bad.Run(); double badError=bad.Error(expected);
				printf("OFFLINE injected arithmetic/layout fault=%u L2=%.9g rejected\n",fault,badError);
				assert(badError>.00002);
			}
		}
	}
	for (unsigned fault=0;fault<8;fault++) {
		Machine m(image,1); std::vector<float> q(256,1); std::vector<uint16_t> k(128,0x3c00),v=k; m.Inputs(q,k,v);
		if (fault==0) m.mem[0x74004/4]=0;
		if (fault==1) m.mem[0x74004/4]=257;
		if (fault==2) m.mem[0x74000/4]^=1;
		if (fault==3) m.mem[0x74008/4]=2;
		if (fault==4) m.mem[0x7400c/4]=1;
		if (fault==5) m.mem[0x90000/4]=0x7f800000;
		if (fault==6) m.mem[0x92000/4]=0x7c00;
		if (fault==7) m.mem[0xa2000/4]=0x7e00;
		m.Run(); assert(!m.mem[0x17ffc4/4] && !m.outputWrites && !m.mmioWrites);
	}
	{
		Machine m(image,1); std::vector<float> q(256,1); std::vector<uint16_t> k(128,0x3c00),v=k; m.Inputs(q,k,v); m.dropAck=true;
		m.Run(true); assert(!m.outputWrites && m.mem[0x17ffc4/4]==0);
	}
	for (int argument=2;argument<argc;argument++) {
		auto file=Read(argv[argument]); assert(file.size()>=48); PioneerAttentionFixture h; memcpy(&h,file.data(),48);
		assert(PioneerAttentionFixtureValid(h) && file.size()==48+h.queryCount*4+h.kvCount*4+h.resultCount*4);
		std::vector<float> q(h.queryCount),expected(h.resultCount),precise(h.resultCount); std::vector<uint16_t> k(h.kvCount),v(h.kvCount);
		unsigned at=48; memcpy(q.data(),file.data()+at,q.size()*4); at+=unsigned(q.size()*4);
		memcpy(k.data(),file.data()+at,k.size()*2); at+=unsigned(k.size()*2); memcpy(v.data(),file.data()+at,v.size()*2); at+=unsigned(v.size()*2);
		memcpy(expected.data(),file.data()+at,expected.size()*4);
		for (unsigned group=0;group<8;group++) {
			std::vector<float> tq(q.begin()+group*256,q.begin()+(group+1)*256),te(expected.begin()+group*256,expected.begin()+(group+1)*256),tp(256);
			std::vector<uint16_t> tk(h.tokens*128),tv(h.tokens*128);
			for (unsigned t=0;t<h.tokens;t++) for (unsigned d=0;d<128;d++) { tk[t*128+d]=k[t*1024+group*128+d]; tv[t*128+d]=v[t*1024+group*128+d]; }
			Reference(tq,tk,tv,h.tokens,tp);
			Machine m(image,h.tokens); m.Inputs(tq,tk,tv); m.Run(); double e=m.Error(te),p=m.Error(tp);
			if (!(e<.02 && p<.00002)) {
				fprintf(stderr,"PRECISION STOP path=%s layer=%u group=%u n=%u captured-L2=%.9g double-L2=%.9g\n",argv[argument],h.layer,group,h.tokens,e,p);
			}
			assert(e<.02 && p<.00002 && m.mem[0x17ffc4/4]==h.tokens && m.outputWrites==256);
			maximumError=fmax(maximumError,p);
			printf("PUBLIC assembled fixture layer=%u group=%u n=%u vs-ggml-f16-L2=%.9g vs-double-L2=%.9g PASS\n",h.layer,group,h.tokens,e,p);
		}
	}
	assert(maxStack<8192);
	printf("PASS: OFFLINE actual RV32IMF/SFPU tile; strict fixed memory/MMIO; partial-FMA model; grouped/causal/softmax, n1..256, input/numeric/timeout faults; stack=%u max-double-L2=%.9g. NO DRIVER OR HARDWARE EXPOSURE\n",maxStack,maximumError);
	return 0;
}
