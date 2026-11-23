/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowActivationSums.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
using namespace PioneerQwen35;
int main()
{
    alignas(16) uint8_t cache[DataflowActivationCacheBytes]{};
    float raw[256];
    for (unsigned type: {8U,12U,13U,14U}) {
        const unsigned inputs=(type==8 || type==13)?2560:9216;
        std::memset(cache,0,sizeof(cache));
        for (unsigned chunk=0;chunk<inputs/256;chunk++) {
            for (unsigned i=0;i<256;i++)
                raw[i]=float(int((chunk*181+i*71)%255)-127)/37.0f;
            auto* page=reinterpret_cast<PioneerQ38ProjectionInput*>(cache+chunk*DataflowInputPageBytes);
            assert(PioneerQ38ProjectionQuantize(type==13?12:type,raw,256,page));
        }
        assert(DataflowPrepareActivationSums(cache,type,inputs));
        const unsigned blocks=inputs/(type==8?32:256),panels=type==8?2:16;
        for (unsigned block=0;block<blocks;block++) for (unsigned panel=0;panel<panels;panel++) {
            const auto* page=reinterpret_cast<const PioneerQ38ProjectionInput*>(
                DataflowPackedActivationPage(cache,type,inputs,block));
            assert(page);
            const unsigned wordBase=type==8?(block%8)*16+panel*8:panel*8;
            int reference=0;
            for (unsigned w=0;w<8;w++) for (unsigned lane=0;lane<2;lane++) {
                const uint16_t value=uint16_t(page->operand[wordBase+w]>>(lane*16));
                assert(PioneerInt8OperandValid(value));
                reference+=(value&0x8000)?-int(value&1023):int(value&1023);
            }
            assert(DataflowActivationSum(cache,type,block,panel)==reference);
        }
        auto* first=reinterpret_cast<PioneerQ38ProjectionInput*>(cache);
        first->operand[0]=0x7fff;
        assert(!DataflowPrepareActivationSums(cache,type,inputs));
    }
    assert(!DataflowPrepareActivationSums(nullptr,12,9216));
    assert(!DataflowPrepareActivationSums(cache,12,0));
    assert(!DataflowPrepareActivationSums(cache,12,9472));
    std::puts("PASS packed activation sidecar: exact panel sums, all formats, invalid operand; SIMULATED ONLY");
}
