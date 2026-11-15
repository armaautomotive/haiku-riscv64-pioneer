/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38TokenPrograms.h"
#include <cassert>
#include <cstdio>
namespace P=PioneerQ38TokenPrograms;namespace L=PioneerQ38TokenL1;
int main(){
	static_assert(P::Family==L::Family && P::Workers==L::Workers,"routed image/footprint identity");
	unsigned words=0,spans=0;
	for(unsigned worker=0;worker<P::Workers;worker++){
		assert(P::CodeSpans(worker)==(worker?8U:3U));
		for(unsigned s=0;s<P::CodeSpans(worker);s++){
			const auto current=P::CodeSpan(worker,s);assert(current.words && !(current.address&3));spans++;
			for(unsigned prior=0;prior<s;prior++){
				const auto old=P::CodeSpan(worker,prior);
				assert(current.address+current.words*4<=old.address || old.address+old.words*4<=current.address);
			}
			for(unsigned n=0;n<current.words;n++){
				const uint32_t address=current.address+n*4;uint32_t value=0xdeadbeef;
				assert(L::SnapshotContains(worker,address) && P::ProgramWord(worker,address,value));
				assert(value!=0xdeadbeef);words++;
			}
			assert(L::SnapshotContains(worker,current.address+current.words*4-4));
			uint32_t unchanged=0xdeadbeef;
			assert(!P::ProgramWord(worker,current.address+current.words*4,unchanged) && unchanged==0xdeadbeef);
		}
	}
	assert(P::CodeWords(0)==17477 && P::CodeWords(1)==23170);
	assert(L::PeerSavedWords==94217 && L::SavedBytes==46115048);
	assert(L::SnapshotContains(1,0xc1d3c) && L::SnapshotContains(1,0xc1d40)
		&& L::SnapshotContains(1,0xc1ffc) && !L::SnapshotContains(1,0xc2000));
	uint32_t unchanged=0xdeadbeef;
	assert(!P::ProgramWord(120,0x6000,unchanged) && unchanged==0xdeadbeef);
	assert(!P::ProgramWord(1,0xc1d40,unchanged) && unchanged==0xdeadbeef);
	std::printf("PASS routed token image DATA root17477/peers23170 words,all%u workers/%u nonoverlap spans fit distinct snapshot;SavedBytes%u;NO launch/Work/native/speed grant\n",words,spans,L::SavedBytes);
}
