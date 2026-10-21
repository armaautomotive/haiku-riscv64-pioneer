/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include "../PioneerQ38LocalQ4.h"
int main()
{
	using Local=PioneerQ38LocalQ4;
	std::array<uint32_t,Local::SourceWords+2> source;
	std::array<uint32_t,Local::OutputWords+2> output;
	std::array<uint8_t,PioneerQ38PredecodedQ4::Bytes> reference;
	std::array<uint8_t,PioneerQ38PredecodedQ4::OriginalBytes> recovered;
	for(unsigned sample=0;sample<65536;sample++) {
		for(unsigned word=0;word<source.size();word++)source[word]=(sample|(sample<<16))^(word*0x9e3779b9U);
		output.fill(0xdeadbeefU);auto unchanged=source;
		assert(Local::Expand(source.data()+1,Local::SourceWords,output.data()+1,Local::OutputWords));
		assert(source==unchanged&&output.front()==0xdeadbeefU&&output.back()==0xdeadbeefU);
		assert(PioneerQ38PredecodedQ4::Prepare(reinterpret_cast<const uint8_t*>(source.data()+1),PioneerQ38PredecodedQ4::OriginalBytes,reference.data(),reference.size()));
		assert(!memcmp(output.data()+1,reference.data(),reference.size()));
		assert(PioneerQ38PredecodedQ4::Recover(reference.data(),reference.size(),recovered.data(),recovered.size()));
		assert(!memcmp(recovered.data(),source.data()+1,recovered.size()));
	}
	unsigned refusals=0;output.fill(0xa5a5a5a5U);auto before=output;
	for(unsigned n:{0U,Local::SourceWords-1,Local::SourceWords+1,UINT32_MAX}) {
		assert(!Local::Expand(source.data()+1,n,output.data()+1,Local::OutputWords)&&output==before);refusals++;
	}
	for(unsigned n:{0U,Local::OutputWords-1,Local::OutputWords+1,UINT32_MAX}) {
		assert(!Local::Expand(source.data()+1,Local::SourceWords,output.data()+1,n)&&output==before);refusals++;
	}
	assert(!Local::Expand(nullptr,Local::SourceWords,output.data()+1,Local::OutputWords)&&output==before);refusals++;
	assert(!Local::Expand(source.data()+1,Local::SourceWords,nullptr,Local::OutputWords));refusals++;
	assert(!Local::Expand(reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(source.data()+1)+1),Local::SourceWords,output.data()+1,Local::OutputWords)&&output==before);refusals++;
	assert(!Local::Expand(source.data()+1,Local::SourceWords,reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(output.data()+1)+1),Local::OutputWords)&&output==before);refusals++;
	// Overlap,exact alias and pointer overflow refuse BEFORE any access.
	assert(!Local::Expand(output.data()+1,Local::SourceWords,output.data()+1,Local::OutputWords)&&output==before);refusals++;
	assert(!Local::Expand(output.data()+2,Local::SourceWords,output.data()+1,Local::OutputWords)&&output==before);refusals++;
	assert(!Local::Expand(reinterpret_cast<const uint32_t*>(~uintptr_t(0)-3),Local::SourceWords,output.data()+1,Local::OutputWords)&&output==before);refusals++;
	assert(!Local::Expand(source.data()+1,Local::SourceWords,reinterpret_cast<uint32_t*>(~uintptr_t(0)-3),Local::OutputWords));refusals++;
	printf("PASS OFFLINE local Q4 word expansion:65536 exhaustive packed-pair patterns/ALL16panels/metadata/exact bytewise inverse,source/canaries preserved,%u invalid/overlap/overflow refusals without publication;NO image/driver/speed authority\n",refusals);
}
