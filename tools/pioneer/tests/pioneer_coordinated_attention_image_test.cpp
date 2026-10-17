/* SPDX-License-Identifier: MIT */
// Actual linked group bytes;no native ABI or device authority.
#ifndef PIONEER_ATTENTION_MACHINE_PROVIDED
#define main legacy_attention_image_main
#include "pioneer_attention_image_test.cpp"
#undef main
#endif
#include "../PioneerCoordinatedAttentionProof.h"
struct GroupMachine:Machine {
	static constexpr uint32_t Base=0x82400000;
	std::map<uint32_t,uint32_t> dram;
	std::map<uint32_t,uint32_t> masterInput;
	uint32_t noc[0x1c44/4]={};
	unsigned group,layer,commands=0,reads=0,writes=0,failCommand=0,badCounter=0,corruptPayload=0,proofWrites=0;
	unsigned corruptPull=0,pullReads=0;
	unsigned badCommandWord=0,badCounterWord=4,busyOffset=0,busyAfter=0;
	bool commandInjected=false;
	GroupMachine(const Image& image,unsigned n,unsigned l,unsigned g):Machine(image,n),group(g),layer(l) {
		assert(image.coordinated);
		mem[0x74000/4]=0x31475441; mem[0x74004/4]=n; mem[0x74008/4]=l; mem[0x7400c/4]=g;
		for (unsigned i=0;i<8;i++) mem[0x70500/4+i]=i==3?1:i==4?1+((g+3)<<6):i==7?0x31475441:0;
		for (unsigned off:{0x204U,0x208U,0x20cU,0x210U,0x214U,0x220U,0x228U}) noc[off/4]=0xfffffff8;
	}
	void Inputs(const std::vector<float>& q,const std::vector<uint16_t>& k,const std::vector<uint16_t>& v) {
		assert(q.size()==256 && k.size()==tokens*128 && v.size()==k.size());
		uint32_t base=Base+layer*0x100000+group*0x20000;
		for (unsigned i=0;i<256;i++) { mem[0x90000/4+i]=image.inputPull?0xcafebabe:B(q[i]); masterInput[0x140000+group*1024+i*4]=B(q[i]); }
		for (unsigned t=0;t<tokens;t++) for (unsigned i=0;i<64;i++) {
			uint32_t key=k[t*128+i*2]|(uint32_t(k[t*128+i*2+1])<<16),val=v[t*128+i*2]|(uint32_t(v[t*128+i*2+1])<<16);
			if (t+1==tokens) { mem[0x90400/4+i]=image.inputPull?0xcafebabe:key; mem[0x90500/4+i]=image.inputPull?0xcafebabe:val;
				masterInput[0x142000+group*512+i*4]=key; masterInput[0x142100+group*512+i*4]=val; }
			else { dram[base+t*256+i*4]=key; dram[base+0x10000+t*256+i*4]=val; }
		}
	}
	uint32_t Load(uint32_t a) override {
		if (Range(a,0xffb20000,sizeof(noc))) {
			unsigned off=a-0xffb20000;
			assert(off<=36 || off==0x40 || off==0x840 || off==0x1040 || off==0x1840 || off==0x204 || off==0x208
				|| off==0x20c || off==0x210 || off==0x214 || off==0x220 || off==0x228 || Range(off,0x240,128));
			if (badCommandWord && off==(badCommandWord-1)*4 && !commands) { commandInjected=true; return noc[off/4]^1; }
			if (busyOffset && busyOffset==off && commands>=busyAfter) return 1;
			return noc[off/4];
		}
		if (Range(a,0x70500,32) || Range(a,0x90400,608)) return mem[a/4];
		return Machine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0xffb20000,sizeof(noc))) {
			unsigned w=(a-0xffb20000)/4; assert(w<10 || w==0x40/4);
			if (w!=0x40/4) { noc[w]=value; return; }
			assert(value==1 && !noc[w]); commands++;
			if (commands==failCommand) { noc[w]=1; return; }
			bool write=noc[7]==0x212; assert(write || !noc[7]);
			uint32_t local=write?noc[0]:noc[3],remote=write?noc[3]:noc[0],bytes=noc[8];
			assert(!noc[1] && !noc[4] && !noc[6] && !noc[9] && bytes && bytes<=16384 && !(bytes&255));
			bool master=!write && noc[2]==129; assert(!master || image.inputPull);
			unsigned self=1+((group+3)<<6); assert(noc[2]==(write?self:master?129U:786U) && noc[5]==(write?786U:self));
			uint32_t base=Base+layer*0x100000+group*0x20000;
			assert((local&31)==(remote&31));
			if (master) {
				bool query=bytes==1024 && remote==0x140000+group*1024 && local==0x90000;
				bool kv=bytes==512 && remote==0x142000+group*512 && local==0x90400;
				bool gate=bytes==2048 && remote==0x87000+group*2048 && local==0x90000;
				bool up=bytes==2048 && remote==0x84000+group*2048 && local==0x90800;
				bool scratch=local==0x91800 && ((bytes==1024 && remote==0x140000+group*1024) || (bytes==512 && remote==0x142000+group*512)
					|| (bytes==2048 && (remote==0x87000+group*2048 || remote==0x84000+group*2048)));
				assert(query || kv || gate || up || scratch);
				for (unsigned i=0;i<bytes;i+=4) { assert(masterInput.count(remote+i)); mem[(local+i)/4]=masterInput.at(remote+i); }
				pullReads++; if (pullReads==corruptPull) mem[(local+bytes-4)/4]^=1;
				reads++;
			} else if (write) {
				assert(remote>=base && remote+bytes<=base+0x20000);
				assert(bytes==256 && (local==0x90400 || local==0x90500));
				assert(remote==base+(local==0x90500?0x10000:0)+(tokens-1)*256);
				for (unsigned i=0;i<bytes;i+=4) dram[remote+i]=mem[(local+i)/4];
				if (commands==corruptPayload) dram[remote+252]^=1;
				writes++;
			} else {
				assert(remote>=base && remote+bytes<=base+0x20000);
				bool key=Range(local,0x92000,0x10000); assert(key || Range(local,0xa2000,0x10000));
				unsigned offset=local-(key?0x92000:0xa2000);
				assert(offset+bytes<=tokens*256 && remote==base+(key?0:0x10000)+offset);
				for (unsigned i=0;i<bytes;i+=4) { assert(dram.count(remote+i)); mem[(local+i)/4]=dram.at(remote+i); }
				reads++;
			}
			const unsigned offsets[]={write?0x204U:0x208U,write?0x220U:0x20cU,0x210,write?0x228U:0x214U};
			for (unsigned i=0;i<4;i++) noc[offsets[i]/4]+=i==1?bytes/64:1;
			if (commands==badCounter) { assert(badCounterWord && badCounterWord<=4); noc[offsets[badCounterWord-1]/4]++; }
			return;
		}
		if (Range(a,0x74000,16) || Range(a,0x90600,96)) { if (Range(a,0x90600,96)) proofWrites++; mem[a/4]=value; return; }
		Machine::Store(a,value);
	}
	void Verify() {
		assert(mem[0x17ffc4/4]==tokens && outputWrites==256 && reads==2*((tokens+63)/64)+(image.inputPull?4:0) && writes==2 && proofWrites==unsigned(image.inputPull?37:36));
		assert(pullReads==unsigned(image.inputPull?4:0));
		const unsigned deltas[]={reads,8*tokens+(image.inputPull?48:0),reads+writes,reads,writes,8,reads+writes,writes};
		for (unsigned i=0;i<8;i++) assert(uint32_t(mem[0x90620/4+i]-mem[0x90600/4+i])==deltas[i]);
		assert(mem[0x90640/4]==tokens && mem[0x90644/4]==layer && mem[0x90648/4]==group && mem[0x9064c/4]==0x31475441);
		uint32_t proof[24]; for (unsigned i=0;i<24;i++) proof[i]=mem[0x90600/4+i];
		assert(PioneerCoordinatedAttentionProof(proof,tokens,layer,group,image.inputPull));
		for (unsigned i=0;i<24;i++) { proof[i]^=1; assert(!PioneerCoordinatedAttentionProof(proof,tokens,layer,group,image.inputPull)); proof[i]^=1; }
	}
};
#ifndef PIONEER_COORDINATED_ATTENTION_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2); setvbuf(stdout,NULL,_IONBF,0); Image image(argv[1],false,true,getenv("PIONEER_ATTENTION_CACHED_QUERY") || getenv("PIONEER_COORDINATED_PULL_INPUT"));
	for (unsigned n:{1U,2U,3U,64U,65U,256U}) for (unsigned group=0;group<8;group++) {
		GroupMachine m(image,n,n&1?27:0,group); std::vector<float> q(256),expected(256); std::vector<uint16_t> k(n*128),v(n*128);
		for (unsigned i=0;i<256;i++) q[i]=float(sin(i*.173+group*.137)*1.7);
		for (unsigned i=0;i<k.size();i++) { // Independent full-IEEE half conversion.
			_Float16 kh=_Float16(sin(i*.371)*1.3),vh=_Float16(cos(i*.217)*2); memcpy(&k[i],&kh,2); memcpy(&v[i],&vh,2); }
		m.Inputs(q,k,v); Reference(q,k,v,n,expected); m.Run(); m.Verify(); assert(m.Error(expected)<.00002);
		printf("OFFLINE actual fixed KV group%u layer%u n%u wrapped counters/readback/fused attention PASS\n",group,m.layer,n);
	}
	for (unsigned fault=0;fault<16;fault++) {
		GroupMachine m(image,1,0,0); std::vector<float> q(256,1); std::vector<uint16_t> k(128,0x3c00),v=k; m.Inputs(q,k,v);
		if (fault<4) m.mem[0x74000/4+fault]=fault==1?0:fault==2?28:fault==3?8:0;
		else if (fault==4) m.mem[0x74004/4]=257;
		else if (fault<13) m.mem[0x70500/4+fault-5]^=1;
		else if (image.inputPull) m.masterInput[fault==13?0x140000: fault==14?0x142000:0x1421fc]=fault==13?0x7f800000:fault==14?0x7c00:0x7c000000;
		else if (fault==13) m.mem[0x90000/4]=0x7f800000;
		else if (fault==14) m.mem[0x90400/4]=0x7c00;
		else m.mem[0x90500/4+63]=0x7c000000;
		m.Run(); assert(!m.mem[0x17ffc4/4] && m.commands==unsigned(image.inputPull && fault>=13?4:0) && !m.outputWrites && !m.mmioWrites && m.proofWrites==unsigned(image.inputPull && fault>=13?8:0));
	}
	for (unsigned fault=0;fault<4;fault++) {
		GroupMachine m(image,1,27,7); std::vector<float> q(256,1); std::vector<uint16_t> k(128,0x3c00),v=k; m.Inputs(q,k,v);
		if (!fault) m.failCommand=1; else if (fault==1) m.badCounter=1; else m.corruptPayload=(fault==2?1:2)+(image.inputPull?4:0);
		m.Run(true); assert(!m.mem[0x17ffc4/4] && !m.outputWrites);
	}
	if (image.inputPull) {
		for (unsigned word=1;word<=10;word++) { GroupMachine m(image,1,27,7); std::vector<float> q(256,1); std::vector<uint16_t> k(128,0x3c00),v=k; m.Inputs(q,k,v);
			m.badCommandWord=word; m.Run(true); assert(m.commandInjected && !m.commands && !m.mem[0x17ffc4/4] && !m.outputWrites); }
		for (unsigned packet=1;packet<=4;packet++) for (unsigned fault=0;fault<6;fault++) { GroupMachine m(image,1,27,7); std::vector<float> q(256,1); std::vector<uint16_t> k(128,0x3c00),v=k; m.Inputs(q,k,v);
			if (fault<4) { m.badCounter=packet; m.badCounterWord=fault+1; } else if (fault==4) m.corruptPull=packet; else m.failCommand=packet;
			m.Run(true); assert(!m.mem[0x17ffc4/4] && !m.outputWrites); }
		for (unsigned offset:{0x840U,0x1040U,0x1840U,0x244U,0x2bcU}) { GroupMachine m(image,1,27,7); std::vector<float> q(256,1); std::vector<uint16_t> k(128,0x3c00),v=k; m.Inputs(q,k,v);
			m.busyOffset=offset; m.busyAfter=1; m.Run(true); assert(!m.mem[0x17ffc4/4]); }
	}
	puts("PASS OFFLINE fixed-group actual instructions,all8 group/layer ranges,n1..256,full KV readback/proof,counter wrap,bad data/descriptor/route/timeout/counter faults;NOT concurrent native qualification");
}
#endif
