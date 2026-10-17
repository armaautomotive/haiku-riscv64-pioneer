/* SPDX-License-Identifier: MIT */
// Same instruction/SFPU interpreter, extended only for bounded private KV NoC.
#define main legacy_attention_image_main
#include "pioneer_attention_image_test.cpp"
#undef main

struct ResidentMachine:Machine {
	static constexpr uint32_t Base=0x82400000;
	std::map<uint32_t,uint32_t> dram;
	uint32_t noc[0x1c44/4]={};
	unsigned writes=0,reads=0,commands=0,failCommand=0,corruptCounter=0,corruptPayloadCommand=0,corruptPayloadWord=0;
	explicit ResidentMachine(const Image& i,unsigned n):Machine(i,n) { assert(i.resident); }
	void Stage(unsigned n,unsigned layer,const std::vector<float>& q,const std::vector<uint16_t>& k,const std::vector<uint16_t>& v) {
		assert(n && n<=256 && layer<28 && q.size()==2048 && k.size()==1024 && v.size()==1024);
		tokens=n; pc=0x6000; steps=0; outputWrites=0;
		mem[0x74000/4]=0x3152564b; mem[0x74004/4]=n; mem[0x74008/4]=layer; mem[0x7400c/4]=Base;
		for (unsigned j=0;j<3;j++) mem[0x17ffc0/4+j]=0;
		for (unsigned j=0;j<2048;j++) { mem[0x140000/4+j]=B(q[j]); mem[0x144000/4+j]=0x7fc00000; }
		for (unsigned g=0;g<8;g++) for (unsigned d=0;d<128;d+=2) {
			mem[0x142000/4+g*128+d/2]=k[g*128+d]|(uint32_t(k[g*128+d+1])<<16);
			mem[0x142100/4+g*128+d/2]=v[g*128+d]|(uint32_t(v[g*128+d+1])<<16);
		}
	}
	uint32_t Load(uint32_t a) override {
		if (Range(a,0xffb20000,sizeof(noc))) {
			unsigned off=a-0xffb20000;
			assert(off<=36 || off==0x40 || off==0x840 || off==0x1040 || off==0x1840
				|| off==0x204 || off==0x208 || off==0x20c || off==0x210 || off==0x214 || off==0x220 || off==0x228
				|| Range(off,0x240,32*4));
			return noc[off/4];
		}
		if (Range(a,0x140000,0x6000) || Range(a,0xb2000,1024)) return mem[a/4];
		return Machine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0xffb20000,sizeof(noc))) {
			unsigned w=(a-0xffb20000)/4;
			assert(w<10 || w==0x40/4);
			if (w!=0x40/4) { noc[w]=value; return; }
			assert(value==1 && !noc[w]); commands++;
			if (commands==failCommand) { noc[w]=1; return; }
			bool write=noc[7]==0x212; assert(write || !noc[7]);
			uint32_t local=write?noc[0]:noc[3],remote=write?noc[3]:noc[0],bytes=noc[8];
			assert(!noc[1] && !noc[4] && !noc[6] && !noc[9]);
			assert(noc[2]==(write?129U:786U) && noc[5]==(write?786U:129U));
			unsigned layer=remote-Base; assert(remote>=Base && layer<28*0x100000 && bytes && bytes<=16384 && !(bytes&63));
			assert(bytes<=28*0x100000-layer);
			assert((local&31)==(remote&31));
			if (write) {
				assert(bytes==256 && Range(local,0x142000,4096));
				unsigned group=(local-0x142000)/512,kind=(local-0x142000)%512;
				assert(kind==0 || kind==256);
				assert((remote-Base)%0x100000==group*0x20000+(kind?0x10000:0)+(tokens-1)*256);
				for (unsigned j=0;j<bytes;j+=4) dram[remote+j]=mem[(local+j)/4];
				if (commands==corruptPayloadCommand) { assert(corruptPayloadWord<bytes/4); dram[remote+corruptPayloadWord*4]^=1; }
				writes++;
			} else {
				bool key=Range(local,0x92000,0x10000),val=Range(local,0xa2000,0x10000); assert(key || val);
				assert((remote-Base)%0x10000==local-(key?0x92000:0xa2000));
				assert((local-(key?0x92000:0xa2000))+bytes<=tokens*256);
				for (unsigned j=0;j<bytes;j+=4) { auto p=dram.find(remote+j); assert(p!=dram.end()); mem[(local+j)/4]=p->second; }
				reads++;
			}
			const unsigned offsets[4]={write?0x204U:0x208U,write?0x220U:0x20cU,0x210,write?0x228U:0x214U};
			for (unsigned j=0;j<4;j++) noc[offsets[j]/4]+=j==1?bytes/64:1;
			if (commands==corruptCounter) noc[offsets[3]/4]++;
			return;
		}
		if (Range(a,0x74000,16) || Range(a,0x90000,1024) || Range(a,0x144000,8192)) { mem[a/4]=value; return; }
		Machine::Store(a,value);
	}
};

int main(int argc,char** argv)
{
	assert(argc==2); setvbuf(stdout,NULL,_IONBF,0); Image image(argv[1],true);
	ResidentMachine m(image,1);
	std::vector<float> q(2048); std::vector<uint16_t> k(1024),v(1024);
	std::vector<uint16_t> historyK(256*1024),historyV(256*1024);
	for (unsigned n=1;n<=3;n++) {
		for (unsigned j=0;j<q.size();j++) q[j]=float(sin(j*.713+n*.371)*1.7);
		for (unsigned j=0;j<1024;j++) { k[j]=PioneerFloat16(float(sin(j*.429+n)*1.3)); v[j]=PioneerFloat16(float(cos(j*.139+n)*2)); }
		memcpy(historyK.data()+(n-1)*1024,k.data(),2048); memcpy(historyV.data()+(n-1)*1024,v.data(),2048);
		m.Stage(n,0,q,k,v); unsigned oldWrites=m.writes,oldReads=m.reads; m.Run();
		assert(m.writes-oldWrites==16 && m.reads-oldReads==16 && m.outputWrites==2048 && m.mem[0x17ffc4/4]==n);
		for (unsigned g=0;g<8;g++) {
			std::vector<float> tq(q.begin()+g*256,q.begin()+(g+1)*256),expected(256);
			std::vector<uint16_t> tk(n*128),tv(n*128);
			for (unsigned t=0;t<n;t++) for (unsigned d=0;d<128;d++) { tk[t*128+d]=historyK[t*1024+g*128+d]; tv[t*128+d]=historyV[t*1024+g*128+d]; }
			Reference(tq,tk,tv,n,expected);
			double delta=0,norm=0;
			for (unsigned j=0;j<256;j++) { float a=F(m.mem[0x144000/4+g*256+j]); assert(isfinite(a)); double d=double(a)-expected[j]; delta+=d*d; norm+=double(expected[j])*expected[j]; }
			assert(sqrt(delta/(norm+1e-30))<.00002);
		}
		printf("RESIDENT assembled all8 groups n%u newKV4096B query8192B exact NoC counters / double reference PASS\n",n);
	}
	for (unsigned n:{31U,64U,65U,128U,256U}) {
		ResidentMachine boundary(image,n);
		for (unsigned offset:{0x204U,0x208U,0x20cU,0x210U,0x214U,0x220U,0x228U}) boundary.noc[offset/4]=0xfffffff8;
		for (unsigned g=0;g<8;g++) for (unsigned t=0;t<n-1;t++) for (unsigned d=0;d<128;d+=2) {
			uint32_t a=ResidentMachine::Base+27*0x100000+g*0x20000+t*256+d*2;
			boundary.dram[a]=uint32_t(k[g*128+d])|(uint32_t(k[g*128+d+1])<<16);
			boundary.dram[a+0x10000]=uint32_t(v[g*128+d])|(uint32_t(v[g*128+d+1])<<16);
		}
		boundary.Stage(n,27,q,k,v); boundary.Run();
		assert(boundary.writes==16 && boundary.reads==16*((n+63)/64) && boundary.mem[0x17ffc4/4]==n);
		for (unsigned g=0;g<8;g++) {
			std::vector<float> tq(q.begin()+g*256,q.begin()+(g+1)*256),expected(256);
			std::vector<uint16_t> tk(n*128),tv(n*128);
			for (unsigned t=0;t<n;t++) for (unsigned d=0;d<128;d++) { tk[t*128+d]=k[g*128+d]; tv[t*128+d]=v[g*128+d]; }
			Reference(tq,tk,tv,n,expected);
			for (unsigned j=0;j<256;j++) assert(fabsf(F(boundary.mem[0x144000/4+g*256+j])-expected[j])<.00002f);
		}
		printf("RESIDENT maximum layer27 n%u burst boundaries / wrapping counters / reference PASS\n",n);
	}
	for (unsigned fault=0;fault<6;fault++) {
		ResidentMachine bad(image,1); bad.Stage(1,0,q,k,v);
		if (fault==0) bad.mem[0x74000/4]^=1;
		if (fault==1) bad.mem[0x74008/4]=28;
		if (fault==2) bad.mem[0x7400c/4]=0x82401000;
		if (fault==3) bad.mem[0x140000/4]=0x7f800000;
		if (fault==4) bad.mem[0x142000/4]=0x7c00;
		if (fault==5) bad.mem[0x142ffc/4]=0x7c000000;
		bad.Run(); assert(!bad.commands && !bad.outputWrites && !bad.mem[0x17ffc4/4]);
	}
	for (unsigned command:{1U,2U,3U,4U,32U}) for (unsigned fault=0;fault<2;fault++) {
		ResidentMachine bad(image,1); bad.Stage(1,0,q,k,v);
		if (!fault) bad.failCommand=command; else bad.corruptCounter=command;
		bad.Run(true); assert(!bad.mem[0x17ffc4/4]);
	}
	for (unsigned group=0;group<8;group++) for (unsigned kind=0;kind<2;kind++) for (unsigned word:{0U,63U}) {
		ResidentMachine bad(image,1); bad.Stage(1,0,q,k,v);
		bad.corruptPayloadCommand=group*4+kind+1; bad.corruptPayloadWord=word;
		bad.Run(true); assert(!bad.mem[0x17ffc4/4]);
	}
	printf("RESIDENT image bounds, invalid inputs, uncertain transfers: no accepted output PASS\n");
}
