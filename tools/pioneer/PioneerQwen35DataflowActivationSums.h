/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ACTIVATION_SUMS_H
#define PIONEER_QWEN35_DATAFLOW_ACTIVATION_SUMS_H
#include "PioneerQwen35DataflowKernels.h"
#include "PioneerQ38ProjectionInput.h"
namespace PioneerQwen35 {
// The raw input and packed pages share this cache. The sidecar is written only
// after all raw chunks have been packed, and is immutable until the next miss.
enum : unsigned {
    DataflowActivationSumsOffset=(PioneerQwen35Projection::MaxInputs/256)*DataflowInputPageBytes,
    DataflowActivationSumsPerChunk=16,
    DataflowActivationSumsBytes=(PioneerQwen35Projection::MaxInputs/256)*DataflowActivationSumsPerChunk*sizeof(int32_t)
};
static_assert(DataflowActivationSumsOffset+DataflowActivationSumsBytes<=DataflowActivationCacheBytes,
    "activation sums must fit after every packed chunk");
inline bool DataflowPrepareActivationSums(uint8_t* cache,unsigned type,unsigned inputs)
{
    if (!cache || !PioneerQwen35Projection::Supports(type,inputs)
        || !inputs || inputs%256 || inputs>PioneerQwen35Projection::MaxInputs) return false;
    auto* sums=reinterpret_cast<int32_t*>(cache+DataflowActivationSumsOffset);
    const unsigned chunks=inputs/256;
    for (unsigned chunk=0;chunk<chunks;chunk++) {
        const auto& page=*reinterpret_cast<const PioneerQ38ProjectionInput*>(cache+chunk*DataflowInputPageBytes);
        for (unsigned panel=0;panel<DataflowActivationSumsPerChunk;panel++) {
            int sum=0;
            for (unsigned word=0;word<8;word++) {
                const uint32_t value=page.operand[panel*8+word];
                for (unsigned lane=0;lane<2;lane++) {
                    const uint16_t encoded=uint16_t(value>>(lane*16));
                    if (!PioneerInt8OperandValid(encoded)) return false;
                    const int magnitude=int(encoded&1023);
                    sum+=(encoded&0x8000)?-magnitude:magnitude;
                }
            }
            if (sum< -2048 || sum>2048) return false;
            sums[chunk*DataflowActivationSumsPerChunk+panel]=sum;
        }
    }
    DataflowL1StoresVisible(reinterpret_cast<volatile uint32_t*>(sums)+chunks*DataflowActivationSumsPerChunk-1);
    return true;
}
inline int DataflowActivationSum(const uint8_t* cache,unsigned type,unsigned block,unsigned panel)
{
    const unsigned chunk=DataflowActivationChunk(type,block);
    const unsigned index=type==8?(block%8)*2+panel:panel;
    return reinterpret_cast<const int32_t*>(cache+DataflowActivationSumsOffset)
        [chunk*DataflowActivationSumsPerChunk+index];
}
}
#endif
