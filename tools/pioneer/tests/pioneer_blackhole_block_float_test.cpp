/* SPDX-License-Identifier: MIT */
#include "../PioneerBlackholeBlockFloat.h"
#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <vector>
using Codec=PioneerBlackholeBlockFloat;
using Format=Codec::Format;
static uint32_t Bits(float f) { uint32_t b;std::memcpy(&b,&f,4);return b; }
static float Float(uint32_t b) { float f;std::memcpy(&f,&b,4);return f; }
// Independent numerical definition,NOT the packer's integer bit shifts.
// Explicit FLOOR models the vendor's pre-round exponent alignment;it is
// not an idealized single rounding of the exact original FP32 value.
static unsigned Reference(float x,unsigned exponent,unsigned width) {
	if(x==0)return 0;
	const long double aligned=std::floor(std::ldexp(std::fabs(static_cast<long double>(x)),150-int(exponent)));
	const long double scaled=std::ldexp(aligned,-int(25-width));
	unsigned q=unsigned(std::floor(scaled));const long double remainder=scaled-q;
	if(remainder>0.5L || (remainder==0.5L && (q&1)))q++;
	q=std::min(q,(1U<<(width-1))-1);
	return q?q|(unsigned(std::signbit(x))<<(width-1)):0;
}
static uint64_t values=0,tiles=0;
static void Check(const std::array<float,1024>& source,Format format) {
	std::array<uint8_t,1096> packed;packed.fill(0xa5);
	std::array<float,1024> decoded;
	assert(Codec::PackTile(format,source.data(),1024,packed.data()+3,Codec::Bytes(format)));
	assert(packed[0]==0xa5 && packed[2]==0xa5 && packed[3+Codec::Bytes(format)]==0xa5);
	assert(Codec::UnpackTile(format,packed.data()+3,Codec::Bytes(format),decoded.data(),1024));
	for(unsigned fr=0;fr<2;fr++)for(unsigned fc=0;fc<2;fc++)for(unsigned r=0;r<16;r++) {
		const unsigned group=(fr*2+fc)*16+r,width=unsigned(format);unsigned exponent=0;
		for(unsigned c=0;c<16;c++)exponent=std::max(exponent,(Bits(source[(fr*16+r)*32+fc*16+c])>>23)&255U);
		assert(packed[3+group]==exponent);
		for(unsigned c=0;c<16;c++) {
			const unsigned index=(fr*16+r)*32+fc*16+c,n=group*16+c;
			const unsigned q=Reference(source[index],exponent,width);
			const unsigned actual=width==8?packed[3+64+n]:(packed[3+64+n/2]>>((n&1)*4))&15;
			assert(actual==q);
			const unsigned magnitude=q&((1U<<(width-1))-1);
			const long double exact=(q>>(width-1)?-1.L:1.L)*std::ldexp(static_cast<long double>(magnitude),int(exponent)-127-int(width-2));
			assert(decoded[index]==exact && (magnitude || Bits(decoded[index])==0));values++;
		}
	}
	tiles++;
}
int main() {
	std::array<float,1024> source{};
	for(auto format:{Format::Bfp4B,Format::Bfp8B}) {
		for(uint32_t constant:{0U,0x80000000U,0x3f800000U,0xbf800000U}) {
			source.fill(Float(constant));Check(source,format);
			std::array<uint8_t,1088> p;assert(Codec::PackTile(format,source.data(),1024,p.data(),p.size()));
			for(unsigned i=0;i<64;i++)assert(p[i]==(constant&0x7fffffffU?127:0));
			const unsigned repeated=!(constant&0x7fffffffU)?0:format==Format::Bfp4B?(constant>>31?0xcc:0x44):(constant>>31?0xc0:0x40);
			for(unsigned i=64;i<Codec::Bytes(format);i++)assert(p[i]==repeated);
		}
		// All representable signed mantissas across EVERY supported shared
		// exponent;face/row/column identities differ,including both nibbles.
		for(unsigned exponent=8;exponent<255;exponent++) {
			const unsigned width=unsigned(format),maximum=(1U<<(width-1))-1;
			for(unsigned i=0;i<1024;i++) {
				const unsigned column=i%32;
				const unsigned q=column%16==15?maximum:(i*37+i/32)% (maximum+1);
				source[i]=float(std::ldexp(static_cast<double>(q),int(exponent)-127-int(width-2)));
				if(i&1)source[i]=-source[i];
			}
			Check(source,format);
		}
		// Rounding ties and one-FP32-ULP neighbours,plus exponent alignment
		// gaps up to246 (no undefined shift >=32),saturation and both signs.
		for(unsigned exponent:{8U,64U,127U,200U,254U})for(unsigned gap=0;gap<=exponent-8;gap++) {
			const unsigned width=unsigned(format),shift=25-width;
			for(unsigned i=0;i<1024;i++) {
				const unsigned at=i%16,bin=(i/16)% (1U<<(width-2));
				uint32_t mantissa=(bin<<shift)+(1U<<(shift-1));
				if(at%3==0)mantissa--;
				if(at%3==2)mantissa++;
				const unsigned own=at==15?exponent:exponent-gap;
				source[i]=Float((unsigned(i&1)<<31)|(own<<23)|(mantissa&0x7fffff));
			}
			Check(source,format);
		}
		uint32_t state=0x47e8b216;
		for(unsigned trial=0;trial<256;trial++) {
			for(auto& v:source) { state^=state<<13;state^=state>>17;state^=state<<5;
				v=Float((state&0x807fffffU)|((8+(state>>8)%247)<<23)); }
			Check(source,format);
		}
		// Inert,atomic refusal at first/middle/last unsupported source value.
		for(unsigned position:{0U,511U,1023U})for(uint32_t invalid:{1U,0x80000001U,0x7f800000U,0xff800000U,0x7fc00001U}) {
			source.fill(1);source[position]=Float(invalid);std::array<uint8_t,1088> p;p.fill(0xc7);const auto before=p;
			assert(!Codec::PackTile(format,source.data(),1024,p.data(),p.size()) && p==before);
		}
		// The lower bound is on the SHARED exponent,not every input:
		// small normals alongside1.0 round to zero;an all-tiny row refuses.
		source.fill(1);source[0]=Float(0x00800000);Check(source,format);
		for(unsigned exponent=1;exponent<8;exponent++) {
			source.fill(Float(exponent<<23));std::array<uint8_t,1088> p;p.fill(0xc7);const auto before=p;
			assert(!Codec::PackTile(format,source.data(),1024,p.data(),p.size()) && p==before);
		}
		source.fill(1);std::array<uint8_t,1088> p;assert(Codec::PackTile(format,source.data(),1024,p.data(),p.size()));
		std::array<float,1024> out;out.fill(13);const auto before=out;
		for(unsigned position:{0U,31U,63U})for(unsigned bad:{1U,7U,255U}) {
			auto q=p;q[position]=uint8_t(bad);
			assert(!Codec::UnpackTile(format,q.data(),Codec::Bytes(format),out.data(),1024) && out==before);
		}
		for(unsigned position:{64U,Codec::Bytes(format)-1}) {
			auto q=p;q[position]=format==Format::Bfp4B?0x88:0x80;
			assert(!Codec::UnpackTile(format,q.data(),Codec::Bytes(format),out.data(),1024) && out==before);
		}
		assert(!Codec::PackTile(format,nullptr,1024,p.data(),p.size()));
		assert(!Codec::PackTile(format,source.data(),1023,p.data(),p.size()));
		assert(!Codec::PackTile(format,source.data(),1024,p.data(),Codec::Bytes(format)-1));
		assert(!Codec::PackTile(format,source.data(),1024,reinterpret_cast<uint8_t*>(source.data()),Codec::Bytes(format)));
		assert(!Codec::UnpackTile(format,p.data(),Codec::Bytes(format)-1,out.data(),1024));
		assert(!Codec::UnpackTile(format,p.data(),Codec::Bytes(format),out.data(),1023));
		assert(!Codec::UnpackTile(format,reinterpret_cast<const uint8_t*>(out.data()),Codec::Bytes(format),out.data(),1024));
		// Integer pack/unpack is independent of the process FP rounding mode.
		for(int mode:{FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO}) {
			assert(!fesetround(mode));std::array<uint8_t,1088> same;
			assert(Codec::PackTile(format,source.data(),1024,same.data(),same.size()));
			assert(std::equal(p.begin(),p.begin()+Codec::Bytes(format),same.begin()));
		}
		assert(!fesetround(FE_TONEAREST));
	}
	assert(!Codec::Bytes(Format(0)));
	printf("PASS CPU-ONLY experimental native BFP4_B/BFP8_B:%llu tiles/%llu values;independent numerical RNE/alignment oracle,all shared exponents,mantissas,faces,nibbles,ties,gaps,atomic refusals;NO GPU,model quality,source authentication,native integration or throughput proof\n",(unsigned long long)tiles,(unsigned long long)values);
}
