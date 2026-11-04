/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Q8FpuBlockData.h"
#include "../PioneerQ38Q8FpuStreamLayout.h"
#include "../TenstorrentGgufQuant.h"
#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cfenv>
using Data=PioneerQ38Q8FpuBlockData;using Layout=PioneerQ38Q8FpuStreamLayout;
int main() {
	assert(std::fegetround()==FE_TONEAREST);unsigned exact=0,refused=0,packets=0;
	std::array<uint8_t,Data::OriginalBytes> original;
	for(unsigned row=0;row<32;row++)for(unsigned g=0;g<8;g++) {
		const uint16_t h=uint16_t((g%2?0x8000:0)|((row*337+g*991)%0x7c00));unsigned at=row*272+g*34;
		original[at]=uint8_t(h);original[at+1]=uint8_t(h>>8);
		for(unsigned i=0;i<32;i++)original[at+2+i]=uint8_t(g*32+i+row*19);
	}
	auto unchanged=original;std::array<uint8_t,Data::Bytes+64> prepared;prepared.fill(0xa7);
	assert(Data::Prepare(original.data(),original.size(),prepared.data(),prepared.size()) && original==unchanged);
	for(unsigned row=0;row<32;row++)for(unsigned g=0;g<8;g++) {
		unsigned at=row*272+g*34;
		for(unsigned c=0;c<32;c++){assert(prepared[(row/16)*4096+(g*32+c)*16+row%16]==original[at+2+c]);exact++;}
		uint32_t actual=0;unsigned meta=8192+(g*2+row/16)*64+row%16*4;
		for(unsigned b=0;b<4;b++)actual|=uint32_t(prepared[meta+b])<<(8*b);
		float value=PioneerHalfFloat(uint16_t(original[at]|(unsigned(original[at+1])<<8)));uint32_t expected;memcpy(&expected,&value,4);
		assert(actual==expected);exact++;
	}
	for(unsigned i=Data::Bytes;i<prepared.size();i++)assert(prepared[i]==0xa7);
	auto good=prepared;
	for(unsigned bad:{0U,Data::OriginalBytes-1,Data::OriginalBytes+1,~0U}){assert(!Data::Prepare(original.data(),bad,prepared.data(),prepared.size()) && prepared==good);refused++;}
	assert(!Data::Prepare(original.data(),original.size(),prepared.data(),Data::Bytes-1) && prepared==good);refused++;
	assert(!Data::Prepare(nullptr,original.size(),prepared.data(),prepared.size()) && prepared==good);refused++;
	assert(!Data::Prepare(original.data(),original.size(),nullptr,prepared.size()));refused++;
	for(unsigned row:{0U,15U,31U})for(unsigned half:{0x7c00U,0xfc00U,0x7e01U}) {
		auto bad=original;bad[row*272+7*34]=uint8_t(half);bad[row*272+7*34+1]=uint8_t(half>>8);
		assert(!Data::Prepare(bad.data(),bad.size(),prepared.data(),prepared.size()) && prepared==good);refused++;
	}
	for(unsigned a:{0U,2048U})for(unsigned b:{0U,1024U,4096U}) {
		std::array<uint8_t,16384> memory{};auto before=memory;
		assert(!Data::Prepare(memory.data()+a,original.size(),memory.data()+b,Data::Bytes) && memory==before);refused++;
	}
	assert(!Data::Prepare(reinterpret_cast<const uint8_t*>(UINTPTR_MAX-3),original.size(),prepared.data(),prepared.size()) && prepared==good);refused++;
	assert(!Data::Prepare(original.data(),original.size(),reinterpret_cast<uint8_t*>(UINTPTR_MAX-3),prepared.size()));refused++;
	for(unsigned rows:{0U,1U,15U,17U,31U,33U,~0U}) {
		assert(!Data::PrepareRows(original.data(),original.size(),rows,prepared.data(),prepared.size()) && prepared==good);refused++;
	}
	std::array<uint8_t,16*272> tail;memcpy(tail.data(),original.data(),tail.size());auto tailUnchanged=tail;
	std::array<uint8_t,Data::Bytes> padded;assert(Data::PrepareRows(tail.data(),tail.size(),16,padded.data(),padded.size()) && tail==tailUnchanged);
	for(unsigned row=0;row<32;row++) {
		for(unsigned c=0;c<256;c++){unsigned at=(row/16)*4096+c*16+row%16;assert(padded[at]==(row<16?good[at]:0));exact++;}
		for(unsigned g=0;g<8;g++)for(unsigned b=0;b<4;b++){unsigned at=8192+(g*2+row/16)*64+row%16*4+b;assert(padded[at]==(row<16?good[at]:0));exact++;}
	}
	auto poisoned=original;memset(poisoned.data()+tail.size(),255,poisoned.size()-tail.size());auto poisonedBefore=poisoned;
	std::array<uint8_t,Data::Bytes> poisonedTail;assert(Data::PrepareRows(poisoned.data(),tail.size(),16,poisonedTail.data(),poisonedTail.size()) && poisonedTail==padded && poisoned==poisonedBefore);
	assert(!Data::PrepareRows(poisoned.data(),poisoned.size(),32,prepared.data(),prepared.size()) && prepared==good);refused++;
	assert(!Data::PrepareRows(tail.data(),tail.size()-1,16,prepared.data(),prepared.size()) && prepared==good);refused++;

	for(unsigned k:{5120U,6144U})for(unsigned block:{0U,k/256-1})for(unsigned buffer=0;buffer<2;buffer++) {
		std::array<unsigned,9216> w{};std::array<unsigned,512> x{};std::vector<unsigned> l1(0x180000);
		unsigned flits=0;
		for(unsigned p=0;p<56;p++) {
			Layout::Read read={};assert(Layout::Packet(k,block,buffer,p,read));
			assert(!((read.source|read.destination|read.bytes)&63) && read.bytes==(p<32?256U:64U));
			assert(read.destination>=buffer*0x10000+0x90000 && read.destination+read.bytes<=buffer*0x10000+0x963c0);
			unsigned header=read.destination-64;assert(Layout::HeaderAddress(buffer,p)==header);
			for(unsigned i=0;i<64;i++){assert(!l1[header+i]++);if(!(i&3))assert(Layout::HeaderWord(header+i));}
			for(unsigned i=0;i<read.bytes;i++) {
				assert(!l1[read.destination+i]++);unsigned a=read.source+i;
				if(p<48){assert(a>=Layout::DramWeight+block*9216 && a<Layout::DramWeight+(block+1)*9216);w[a-Layout::DramWeight-block*9216]++;}
				else {assert(a>=Layout::DramInput+block*512 && a<Layout::DramInput+(block+1)*512);x[a-Layout::DramInput-block*512]++;}
			}
			flits+=read.bytes/64;packets++;
		}
		for(unsigned n:w)assert(n==1);for(unsigned n:x)assert(n==1);assert(flits==152);
	}
	for(unsigned kind=0;kind<4;kind++) {
		Layout::Read r={1,2,3};assert(!Layout::Packet(kind?5120:17408,kind==1?20:0,kind==2?2:0,kind==3?56:0,r));
		assert(r.source==1 && r.destination==2 && r.bytes==3);refused++;
	}
	assert(Layout::HeaderAddress(2,0)==~0U && Layout::HeaderAddress(0,56)==~0U && !Layout::HeaderWord(~0U));
	printf("PASS OFFBOARD Q8 data:%u exact unchanged-TC8/coefficient/padded-tail comparisons,%u atomic/admission refusals,%u independent packet byte-coverage checks,private headers/gaps/zero source duplication/poisoned absent rows ignored;NO expanded weights/learned products/driver/cache/owner/hardware/quality/speed authority\n",exact,refused,packets);
}
