/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Q6FpuBlockData.h"
#include "../PioneerQ38Q6FpuStreamLayout.h"
#include "../TenstorrentGgufQuant.h"
#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cfenv>
using Data=PioneerQ38Q6FpuBlockData;using Layout=PioneerQ38Q6FpuStreamLayout;
int main() {
	assert(std::fegetround()==FE_TONEAREST);unsigned exact=0,refused=0,packets=0;
	std::array<uint8_t,Data::OriginalBytes> original;
	for(unsigned i=0;i<original.size();i++)original[i]=uint8_t(i*13+i/210*17);
	const uint16_t halves[]={0,0x8000,1,0x83ff,0x7bff,0xfbff,0x0400,0x3555};
	for(unsigned row=0;row<32;row++){uint16_t h=halves[row%8];original[row*210+208]=uint8_t(h);original[row*210+209]=uint8_t(h>>8);}
	auto unchanged=original;std::array<uint8_t,Data::Bytes+64> prepared;prepared.fill(0xa7);
	assert(Data::Prepare(original.data(),original.size(),prepared.data(),prepared.size()) && original==unchanged);
	for(unsigned row=0;row<32;row++) {
		for(unsigned b=0;b<128;b++){assert(prepared[(row/16)*2048+b*16+row%16]==original[row*210+b]);exact++;}
		for(unsigned b=0;b<64;b++){assert(prepared[4096+(row/16)*1024+b*16+row%16]==original[row*210+128+b]);exact++;}
		for(unsigned g=0;g<16;g++) {
			unsigned at=6144+(g*2+row/16)*64+(row%16)*4;uint32_t actual=0;
			for(unsigned b=0;b<4;b++)actual|=uint32_t(prepared[at+b])<<(b*8);
			int scale=original[row*210+192+g];if(scale>=128)scale-=256;
			float value=PioneerHalfFloat(halves[row%8])*float(scale);uint32_t expected;memcpy(&expected,&value,4);
			assert(actual==expected);exact++;
		}
	}
	for(unsigned i=Data::Bytes;i<prepared.size();i++)assert(prepared[i]==0xa7);
	auto good=prepared;
	for(unsigned bad:{0U,Data::OriginalBytes-1,Data::OriginalBytes+1,~0U}){assert(!Data::Prepare(original.data(),bad,prepared.data(),prepared.size()) && prepared==good);refused++;}
	assert(!Data::Prepare(original.data(),original.size(),prepared.data(),Data::Bytes-1) && prepared==good);refused++;
	assert(!Data::Prepare(nullptr,original.size(),prepared.data(),prepared.size()) && prepared==good);refused++;
	assert(!Data::Prepare(original.data(),original.size(),nullptr,prepared.size()));refused++;
	for(unsigned row:{0U,15U,31U})for(unsigned half:{0x7c00U,0xfc00U,0x7e01U}) {
		auto bad=original;bad[row*210+208]=uint8_t(half);bad[row*210+209]=uint8_t(half>>8);
		assert(!Data::Prepare(bad.data(),bad.size(),prepared.data(),prepared.size()) && prepared==good);refused++;
	}
	for(unsigned a:{0U,2048U})for(unsigned b:{0U,1024U,4096U}) {
		std::array<uint8_t,16384> memory{};auto before=memory;
		assert(!Data::Prepare(memory.data()+a,original.size(),memory.data()+b,Data::Bytes) && memory==before);refused++;
	}
	assert(!Data::Prepare(reinterpret_cast<const uint8_t*>(UINTPTR_MAX-3),original.size(),prepared.data(),prepared.size()) && prepared==good);refused++;
	assert(!Data::Prepare(original.data(),original.size(),reinterpret_cast<uint8_t*>(UINTPTR_MAX-3),prepared.size()));refused++;
	for(unsigned k:{5120U,6144U})for(unsigned block:{0U,k/256-1})for(unsigned buffer=0;buffer<2;buffer++) {
		std::array<unsigned,8192> w{};std::array<unsigned,512> x{};std::vector<unsigned> l1(0x180000);
		unsigned flits=0;
		for(unsigned p=0;p<64;p++) {
			Layout::Read read={};assert(Layout::Packet(k,block,buffer,p,read));
			assert(!((read.source|read.destination|read.bytes)&63) && read.bytes==(p<24?256U:64U));
			assert(read.destination>=buffer*0x10000+0x90000 && read.destination+read.bytes<=buffer*0x10000+0x963c0);
			unsigned header=read.destination-64;assert(Layout::HeaderAddress(buffer,p)==header);
			for(unsigned i=0;i<64;i++){assert(!l1[header+i]++);if(!(i&3))assert(Layout::HeaderWord(header+i));}
			for(unsigned i=0;i<read.bytes;i++) {
				assert(!l1[read.destination+i]++);unsigned a=read.source+i;
				if(p<56){assert(a>=Layout::DramWeight+block*8192 && a<Layout::DramWeight+(block+1)*8192);w[a-Layout::DramWeight-block*8192]++;}
				else {assert(a>=Layout::DramInput+block*512 && a<Layout::DramInput+(block+1)*512);x[a-Layout::DramInput-block*512]++;}
			}
			flits+=read.bytes/64;packets++;
		}
		for(unsigned n:w)assert(n==1);for(unsigned n:x)assert(n==1);assert(flits==136);
	}
	for(unsigned kind=0;kind<4;kind++) {
		Layout::Read r={1,2,3};assert(!Layout::Packet(kind?5120:17408,kind==1?20:0,kind==2?2:0,kind==3?64:0,r));
		assert(r.source==1 && r.destination==2 && r.bytes==3);refused++;
	}
	assert(Layout::HeaderAddress(2,0)==~0U && Layout::HeaderAddress(0,64)==~0U && !Layout::HeaderWord(~0U));
	printf("PASS OFFBOARD Q6 data:%u exact unchanged-plane/coefficient comparisons,%u atomic/admission refusals,%u independent packet byte-coverage checks,private headers/gaps/zero source duplication;NO expanded weights/learned products/driver/cache/owner/hardware/quality/speed authority\n",exact,refused,packets);
}
