/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_native_bfp_machine.h"
#include "../PioneerModelWeightCache.h"
#include <cstdio>
#include <string>
using Kernel=PioneerQ38NativeBfpMatvec;
using Codec=PioneerBlackholeBlockFloat;
static_assert(Kernel::Format(4)==7 && Kernel::Format(8)==6,
	"independent pinned hardware enum: Bfp4_b7/Bfp8_b6,NOT LF8=10 or bit widths");
static unsigned checked=0;
static inline std::array<float,32> Fixture(unsigned bits,unsigned trial,std::array<uint8_t,1088>& packed,
	std::array<uint16_t,32>& input) {
	std::array<float,1024> original,decoded;
	for(unsigned r=0;r<32;r++)for(unsigned c=0;c<32;c++)original[r*32+c]=float(int((r*37+c*19+trial*11)%255)-127)/128;
	for(unsigned c=0;c<32;c++)input[c]=uint16_t(0x3f00+((c*13+trial*7)%256)+((c&3)==2?0x8000:0));
	assert(Codec::PackTile(Codec::Format(bits),original.data(),1024,packed.data(),packed.size()));
	assert(Codec::UnpackTile(Codec::Format(bits),packed.data(),Codec::Bytes(Codec::Format(bits)),decoded.data(),decoded.size()));
	std::array<float,32> reference{};
	for(unsigned r=0;r<32;r++) {
		long double sum=0;
		for(unsigned c=0;c<32;c++)sum+=static_cast<long double>(decoded[r*32+c])*NativeBfpMachine::Float(uint32_t(input[c])<<16);
		reference[r]=float(sum);
	}
	return reference;
}
static inline bool Equal(const std::array<float,32>& a,const std::array<float,32>& b) {
	for(unsigned i=0;i<32;i++)if(NativeBfpMachine::Bits(a[i])!=NativeBfpMachine::Bits(b[i]))return false;
	return true;
}
#ifndef PIONEER_NATIVE_BFP_FIXTURE_ONLY
int main(int argc,char** argv) {
	assert(argc==2);
	for(unsigned bits:{4U,8U})for(unsigned schedule=0;schedule<4;schedule++) {
		NativeBfpMachine machine;machine.delay[0]=schedule==1?1:schedule==2?1000003:79;
		machine.delay[1]=schedule==1?100003: schedule==2?1:7;
		assert(Kernel::Setup(machine,bits));
		for(unsigned trial=0;trial<32;trial++) {
			std::array<uint8_t,1088> packed;std::array<uint16_t,32> input;
			const auto expected=Fixture(bits,trial,packed,input);
			machine.Stage(packed.data(),Kernel::WeightBytes(bits),input);
			assert(Kernel::Run(machine,bits));const auto got=machine.Results();
			if(!Equal(got,expected)) {
				for(unsigned i=0;i<32;i++)if(got[i]!=expected[i])fprintf(stderr,"bits%u trial%u row%u got%.9g expected%.9g\n",bits,trial,i,got[i],expected[i]);
			}
			assert(Equal(got,expected) && machine.Clean());
			for(unsigned r=0;r<8;r++)for(unsigned n=0;n<32;n++)assert(machine.destination[(n<16?32:40)+r][n%16]==expected[n]);
			assert(machine.matrixOps==(trial+1)*(bits==4?8U:16U));
			assert(machine.unpacks[0]==trial+1 && machine.unpacks[1]==trial+1);
			assert(machine.matrixBank[0]==((trial+1)&1) && machine.matrixBank[1]==machine.matrixBank[0]);checked++;
		}
		for(unsigned phase=0;phase<4;phase++)assert(machine.visits[phase]==(bits==4 && phase&1?0U:128U));
	}
	for(unsigned bits:{4U,8U})for(unsigned phase:bits==4?std::vector<unsigned>{2}:std::vector<unsigned>{1,2,3}) {
		NativeBfpMachine machine;assert(Kernel::Setup(machine,bits));machine.dropPhase=phase;
		std::array<uint8_t,1088> packed;std::array<uint16_t,32> input;
		const auto expected=Fixture(bits,3,packed,input);machine.Stage(packed.data(),Kernel::WeightBytes(bits),input);
		assert(Kernel::Run(machine,bits) && !Equal(machine.Results(),expected));checked++;
	}
	for(unsigned missing:{1U,2U}) {
		NativeBfpMachine machine;assert(Kernel::Setup(machine,4));machine.dropProducer=missing;
		std::array<uint8_t,1088> packed;std::array<uint16_t,32> input;Fixture(4,1,packed,input);
		machine.Stage(packed.data(),576,input);bool refused=false;
		try { Kernel::Run(machine,4); } catch(uint32_t marker) { refused=marker==NativeBfpMachine::Unknown; }
		assert(refused && !machine.matrixOps && !machine.outputReads);checked++;
	}
	for(unsigned fault=0;fault<2;fault++) {
		NativeBfpMachine machine;assert(Kernel::Setup(machine,8));
		std::array<uint8_t,1088> packed;std::array<uint16_t,32> input;const auto expected=Fixture(8,7,packed,input);
		machine.Stage(packed.data(),1088,input);
		if(!fault)machine.config[72]&=~0x100U;else machine.omitFlip=true;
		assert(Kernel::Run(machine,8));
		if(!fault)assert(!Equal(machine.Results(),expected));
		else { bool refused=false;try { (void)machine.Results(); } catch(uint32_t marker) { refused=marker==NativeBfpMachine::Unknown; }assert(refused && !machine.outputReads); }
		checked++;
	}
	{
		struct MissingFinalDrain:NativeBfpMachine { unsigned requests=0;void Drain() { if(++requests!=5)NativeBfpMachine::Drain(); } } machine;
		assert(Kernel::Setup(machine,4));std::array<uint8_t,1088> p;std::array<uint16_t,32> v;Fixture(4,0,p,v);machine.Stage(p.data(),576,v);
		assert(Kernel::Run(machine,4));bool refused=false;
		try { (void)machine.Results(); } catch(uint32_t marker) { refused=marker==NativeBfpMachine::Unknown; }
		assert(refused && !machine.outputReads);checked++;
	}
	{
		NativeBfpMachine machine;const auto before=machine.config[0];
		assert(!Kernel::Setup(machine,0) && !Kernel::Run(machine,16) && !machine.cycle && machine.config[0]==before);checked++;
	}
	{
		using Cache=PioneerModelWeightCache;Cache cache;std::string error;
		assert(cache.Open((std::string(argv[1])+"/bfp-matvec-cache").c_str(),error));
		std::array<uint8_t,1088> packed;std::array<uint16_t,32> input;const auto expected=Fixture(4,17,packed,input);
		Cache::Key key;key.sourceSHA256=Cache::Hash(packed.data(),576);key.conversionSHA256=Cache::Hash("synthetic-native-matvec-v1",25);key.bytes=576;
		unsigned conversions=0;auto produce=[&](uint64_t offset,unsigned bytes,uint8_t* out,std::string&) { conversions++;memcpy(out,packed.data()+offset,bytes);return true; };
		for(unsigned pass=0;pass<2;pass++) {
			Cache::Reader reader;bool hit=false;assert(cache.GetOrCreate(key,[]{return true;},produce,reader,hit,error) && hit==bool(pass));
			std::array<uint8_t,576> cached;assert(reader.Read(0,576,cached.data(),576,error));
			NativeBfpMachine machine;assert(Kernel::Setup(machine,4));machine.Stage(cached.data(),576,input);
			assert(Kernel::Run(machine,4) && Equal(machine.Results(),expected) && conversions==1);checked++;
		}
	}
	printf("PASS OFFBOARD NEW native BFP4_B/BFP8_B matvec:%u checks;1024 weights ->ONE A unpack/ONE B unpack,no GGUF decode/SFPU affine;all32 outputs/eight rows/dual-bank repeated jobs/asymmetric producer delays/full BF16 fidelity,phase/transpose/producer/flip/completion faults,cached bytes ->opcode observer with ZERO conversion on hit;NO physical timing,model quality,driver or Work grant\n",checked);
}
#endif
