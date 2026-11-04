/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_NATIVE_BFP_BAND_IMAGE_ONLY
#include "pioneer_q38_native_bfp_band_image_test.cpp"
#include "pioneer_blackhole_arithmetic_observer.h"
#include "../PioneerNativeBfpBandLease.h"
#include <memory>
#include <map>
#include <set>
using Lease=PioneerNativeBfpBandLease;
using Data=PioneerNativeBfpBandFixture;
using Band=PioneerQ38NativeBfpBand;
static std::map<unsigned,std::vector<uint8_t>> images;
template<unsigned K>struct Image {
	static constexpr unsigned Width=K,Phases=4;
	static inline uint32_t Boot=0;
	static uint32_t Word(unsigned index){const auto& code=images.at(K);return index<code.size()/4?::Word(code,index*4):0;}
};
struct RoundedImage:BandRV32 {
	using BandRV32::BandRV32;
	void Instruction(uint32_t op) override {
		if((op>>24)==0x26)PioneerBlackholeArithmeticObserver::Matrix(native,op,true,[&]{BandRV32::Instruction(op);});
		else BandRV32::Instruction(op);
	}
};
static uint16_t WeightBits(int n) {
	if(!n)return 0;const unsigned magnitude=unsigned(n<0?-n:n);
	unsigned top=0;for(unsigned scan=magnitude;scan>>=1;)top++;
	return uint16_t((n<0?0x8000:0)|((120+top)<<7)|((magnitude<<(7-top))-128));
}
static std::array<uint32_t,32> Reference(unsigned width) {
	std::array<uint32_t,32> output{};
	for(unsigned row=0;row<32;row++)for(unsigned tile=0;tile<width/32;tile++)for(unsigned half=0;half<2;half++) {
		uint16_t a[16],b[16];for(unsigned k=0;k<16;k++){a[k]=WeightBits(Data::Numerator(tile,row,half*16+k));b[k]=Data::InputBits(tile,half*16+k);}
		for(unsigned phase=0;phase<4;phase++)assert(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,output[row],output[row]));
	}
	return output;
}
struct IO {
	std::vector<uint32_t> memory=std::vector<uint32_t>(0x180000/4),original;
	uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
	uint32_t tlb[2][3]={{1,2,3},{4,5,6}},reset=0x47800;
	unsigned width,reads=0,writes=0,resets=0,tlbWrites=0,pauses=0,dropL1=0,dropReset=0,dropTlb=0;
	bool timeout=false,badPC=false,faultMarker=false,oldMarker=false,badCount=false,sourceChanged=false,failRestore=false,executed=false,started=false;
	std::string directory;uint64_t now=1000000;
	IO(unsigned k,const char* path):width(k),directory(path) {
		for(unsigned i=0;i<memory.size();i++)memory[i]=0x9e370000U^(i*71);original=memory;
	}
	bool Allowed(uint32_t a)const {
		return !(a&3) && (a==0x6000 || BandRV32::Range(a,0x70000,4096) || BandRV32::Range(a,0x6e000,8192)
			|| BandRV32::Range(a,Band::Weight-16,Band::Tiles(width)*Band::WeightStride)
			|| BandRV32::Range(a,Band::Input-16,Band::Tiles(width)*Band::InputStride)
			|| BandRV32::Range(a,Band::Output,256) || BandRV32::Range(a,0x17ffc0,12));
	}
	uint32_t SourceRead(unsigned i){reads++;assert(i<16);return source[i]^((sourceChanged && started)?1U:0U);}
	uint32_t DramTlbRead(unsigned s,unsigned w){reads++;assert(s<2 && w<3);return tlb[s][w];}
	void DramTlbWrite(unsigned s,unsigned w,uint32_t v){assert(s<2 && w<3);if(++tlbWrites!=dropTlb)tlb[s][w]=v;}
	bool Match(unsigned s,unsigned x,unsigned y,bool ro) {
		for(unsigned w=0;w<3;w++)if(tlb[s][w]!=BlackholeDramFixedTlbWord(s?0xffa00000:0,x,y,ro,w))return false;return true;
	}
	uint32_t ResetRead() {
		reads++;if(Match(1,1,2,false) || Match(1,1,2,true))return reset;
		const unsigned xs[]={1,2,3,4,5,6,7,10,11,12,13,14};
		for(unsigned x:xs)for(unsigned y=2;y<=11;y++)if(Match(1,x,y,true))return 0x47800;
		assert(false && "wrong reset route");return 0;
	}
	void Window(){assert(Match(0,1,2,false) && Match(1,1,2,false));}
	uint32_t BfpRead(uint32_t a){Window();reads++;assert(Allowed(a));return memory[a/4];}
	void BfpWrite(uint32_t a,uint32_t v) {
		Window();assert(Allowed(a) && reset==0x47800 && (!started || executed));
		if(++writes!=dropL1 && !(failRestore && started && v==original[a/4]))memory[a/4]=v;
	}
	uint32_t BfpControlRead(unsigned i){Window();reads++;assert(i<3);return 0;}
	uint32_t BfpPcRead(){Window();reads++;return badPC?0x70040:(pauses&1?0x70038:0x7003c);}
	void Execute() {
		RoundedImage image(directory.c_str(),width,4,false);
		assert(memory[0x6000/4]==image.l1[0]);
		for(unsigned i=0;i<1024;i++)assert(memory[(0x70000+i*4)/4]==(i<image.code.size()/4?::Word(image.code,i*4):0));
		for(unsigned i=0;i<8192/4;i++)image.l1[image.Index(0x6e000+i*4)]=memory[(0x6e000+i*4)/4];
		for(unsigned i=0;i<64;i++)image.l1[image.Index(Band::Output+i*4)]=memory[(Band::Output+i*4)/4];
		for(unsigned tile=0;tile<width/32;tile++)for(unsigned stream=0;stream<2;stream++) {
			const unsigned first=(stream?Band::Input:Band::Weight)-16+tile*(stream?Band::InputStride:Band::WeightStride);
			const unsigned bytes=stream?Band::InputStride:Band::WeightStride;
			for(unsigned b=0;b<bytes;b++)image.native.l1[first+b]=uint8_t(memory[(first+b)/4]>>(((first+b)%4)*8));
		}
		assert(image.Execute());
		for(unsigned i=0;i<64;i++)memory[(Band::Output+i*4)/4]=image.l1[image.Index(Band::Output+i*4)];
		for(unsigned i=0;i<3;i++)memory[(0x17ffc0+i*4)/4]=image.l1[image.Index(0x17ffc0+i*4)];
		for(unsigned i=0;i<8192/4;i++)memory[(0x6e000+i*4)/4]=image.l1[image.Index(0x6e000+i*4)];
		executed=true;
	}
	void BfpResetWrite(uint32_t v) {
		Window();assert(v==0x87ffff || v==0x46800 || v==0x47800);if(++resets==dropReset)return;
		if(v==0x46800) {
			assert(reset==0x87ffff && !started);started=true;reset=v;if(!timeout)Execute();
			if(faultMarker)memory[0x17ffc8/4]=0x4542464e;
			if(oldMarker)memory[0x17ffc8/4]=0x3142464e;
			if(badCount)memory[0x17ffc4/4]^=1;
		}else{if(started)assert(executed && !timeout && !badPC && !sourceChanged && !faultMarker && !oldMarker && !badCount);reset=v;}
	}
	uint64_t BfpNow(){return now;}
	void Pause(){pauses++;now+=100000;}
	bool BfpHealthy()const{return true;}
	tt_dram_samples_result Baseline() {
		tt_dram_samples_result b={};b.version=1;b.state=TT_DRAM_SAMPLE_ROUTES_OK;b.tlb_restored=b.data_restored=1;b.routes_verified=7;
		memcpy(b.source,source,sizeof(source));assert(BlackholeDramBuildRoutes(b));return b;
	}
};
template<unsigned K>static void NoMoreIO(Lease& lease,IO& io,tt_dram_samples_result& baseline) {
	const auto reads=io.reads,writes=io.writes,resets=io.resets,tlb=io.tlbWrites;
	assert((!lease.End(io,baseline) && !lease.Run<IO,Image<K>>(io,baseline,K)));lease.Abandon();
	assert(io.reads==reads && io.writes==writes && io.resets==resets && io.tlbWrites==tlb);
}
template<unsigned K>static void Prepare(const char* path) {
	const std::string root=path;images[K]=::Read((root+"/text.bin").c_str());
	const auto boot=::Read((root+"/boot.bin").c_str());assert(boot.size()==4);Image<K>::Boot=::Word(boot,0);
	assert(!images[K].empty() && images[K].size()<=4096 && !(images[K].size()&3));
	// Independent original float->PackTile fixture must match EVERY new pure
	// integer byte,including face ordering,zero signs and32-value input layout.
	const auto packed=NativeBfpBandMachine::Data(K);
	for(unsigned tile=0;tile<K/32;tile++) {
		for(unsigned byte=0;byte<1088;byte++)assert(Data::WeightByte(tile,byte)==packed.packed[tile][byte]);
		for(unsigned column=0;column<32;column++)assert(Data::InputBits(tile,column)==packed.input[tile][column]);
	}
	std::set<unsigned> addresses;for(unsigned i=0;i<Lease::SavedWords(K);i++)assert(addresses.insert(Lease::Address(K,i)).second);
	assert(addresses.size()==Lease::SavedWords(K) && Lease::Address(K,Lease::SavedWords(K))==UINT32_MAX);
}
template<unsigned K>static void Positive(const char* path) {
	IO io(K,path);auto baseline=io.Baseline();auto lease=std::make_unique<Lease>();
	assert((lease->Run<IO,Image<K>>(io,baseline,K)));
	const auto reference=Reference(K);const auto result=lease->Result();
	assert(result.state==PIONEER_NATIVE_BFP_RESULT && result.started && result.completed && !result.cold && io.resets==2);
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)assert(result.output[pass*32+row]==reference[row]);
	assert(lease->End(io,baseline) && lease->Result().state==PIONEER_NATIVE_BFP_CLOSED && io.memory==io.original && io.resets==4);
	for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)assert(io.tlb[s][w]==s*3+w+1);
	NoMoreIO<K>(*lease,io,baseline);
	for(unsigned row=0;row<32;row++)printf("BAND_LEASE_SYNTHETIC_REFERENCE k%u row%u word%08x\n",K,row,reference[row]);
	printf("PASS OFFBOARD full-K owner K%u words%u bytes%u;actual compiled RV32 image,two jobs/exact64 rounded outputs,complete L1/TLB restore,unowned holes unchanged;NO physical/NoC/GDDR/driver/model-speed authority\n",K,Lease::SavedWords(K),Lease::SavedWords(K)*4);
}
int main(int argc,char** argv) {
	assert(argc==4);Prepare<5120>(argv[1]);Prepare<6144>(argv[2]);Prepare<17408>(argv[3]);
	Positive<5120>(argv[1]);Positive<6144>(argv[2]);Positive<17408>(argv[3]);
	unsigned faults=0;
	for(unsigned fault=0;fault<14;fault++) {
		IO io(5120,argv[1]);auto baseline=io.Baseline();auto lease=std::make_unique<Lease>();
		if(fault==0)io.timeout=true;if(fault==1)io.badPC=true;if(fault==2)io.faultMarker=true;if(fault==3)io.sourceChanged=true;
		if(fault==4)io.dropL1=1024;if(fault==5)io.dropReset=1;if(fault==6)io.dropReset=2;if(fault==7)io.dropTlb=2;
		if(fault==12)io.oldMarker=true;if(fault==13)io.badCount=true;
		bool ran=lease->Run<IO,Image<5120>>(io,baseline,5120);
		if(fault>=8 && fault<12) {
			assert(ran);if(fault==8)io.dropReset=3;if(fault==9)io.dropReset=4;
			if(fault==10)io.failRestore=true;if(fault==11)io.dropTlb=io.tlbWrites+370;
			assert(!lease->End(io,baseline));
		}else assert(!ran);
		assert(lease->Result().state==PIONEER_NATIVE_BFP_UNKNOWN && lease->Result().cold);
		if(fault<8 || fault>=12)assert(io.resets<3);
		NoMoreIO<5120>(*lease,io,baseline);faults++;
	}
	for(unsigned bad:{0U,32U,5119U,6144U,17408U,UINT32_MAX}) {
		IO io(5120,argv[1]);auto baseline=io.Baseline();auto lease=std::make_unique<Lease>();
		assert((!lease->Run<IO,Image<5120>>(io,baseline,bad)) && !io.reads && !io.writes && !io.tlbWrites && !io.resets);faults++;
	}
	{
		IO io(5120,argv[1]);auto baseline=io.Baseline();auto lease=std::make_unique<Lease>();
		assert((lease->Run<IO,Image<5120>>(io,baseline,5120)));
		const auto reads=io.reads,writes=io.writes,resets=io.resets,tlb=io.tlbWrites;lease->Abandon();
		assert(lease->Result().cold && reads==io.reads && writes==io.writes && resets==io.resets && tlb==io.tlbWrites);
		NoMoreIO<5120>(*lease,io,baseline);faults++;
	}
	printf("PASS OFFBOARD full-K owner:%u failure/no-retry/close cases;bad geometry NO IO,old NFB1/wrong-width completion refused,uncertain stage/start/park/source/STOP/L1/TLB restore retain owner;NO retry/reset/destructor cleanup\n",faults);
}
