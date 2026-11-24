/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PROJECTION_QUALIFICATION_H
#define PIONEER_QWEN35_DATAFLOW_PROJECTION_QUALIFICATION_H

#include "PioneerQwen35DataflowProgram.h"

namespace PioneerQwen35 {
// Fixed synthetic Q4_K projection for qualifying the resident graph, not an
// inference fallback. The caller must exclusively lease and restore every
// byte of the scratch range; this fixture grants no device access authority.
// Each of the 120 workers owns one 16-row output group and ten Q4_K blocks.
class DataflowProjectionQualification {
public:
    enum : unsigned {
        Inputs=2560, Rows=DataflowCoreCount*16, Blocks=Inputs/256,
        Q4BlockBytes=144, TileBytes=Q4BlockBytes*16,
        WeightBytes=Rows*Blocks*Q4BlockBytes, InputBytes=Inputs*4,
        OutputBytes=Rows*4, PageBytes=4096,
        // The real fleet route audit resolves page zero in every bank.
        // Keep the fixed diagnostic claim outside that page.
        ScratchOffset=PageBytes, ScratchAddress=unsigned(BankBase)+ScratchOffset,
        WeightPages=(WeightBytes+PageBytes-1)/PageBytes,
        InputPages=(InputBytes+PageBytes-1)/PageBytes,
        OutputPages=(OutputBytes+PageBytes-1)/PageBytes,
        ScratchPages=WeightPages+InputPages+OutputPages,
        ScratchBytes=ScratchPages*PageBytes,
        OutputSentinel=0x7fc00000U
    };

    // The first six blocks encode the row in base four. Combined with the
    // activation weights this makes every row's expected output distinct.
    static unsigned Quant(unsigned row,unsigned block) {
        return block<6 ? 1+((row>>(block*2))&3) : 1;
    }
    // Every 32-value group has a nonzero, changing Q4_K minimum. Alternate
    // positive and negative weights within every core's output group, so a
    // dropped sign or absolute-value result cannot pass qualification.
    static unsigned Minimum(unsigned group) { return 5+(group%4); }
    static bool Negative(unsigned row) { return (row&1U)!=0; }
    static int SignedQuant(unsigned row,unsigned block) {
        const int magnitude=int(Quant(row,block));
        return Negative(row)?-magnitude:magnitude;
    }
    static unsigned ExpectedInteger(unsigned row) {
        if (row>=Rows) return 0;
        unsigned sum=0;
        for (unsigned block=0;block<Blocks;block++)
            sum+=Quant(row,block)*(block<6 ? (1U<<(block*2)) : 1U);
        // Each block contains 144 positive and 112 negative activations.
        return sum*32U*127U;
    }
    // Every expected result is an exactly representable signed FP32 integer.
    // Integer-only bit construction also permits use in a kernel diagnostic.
    static uint32_t ExpectedBits(unsigned row) {
        const uint32_t value=ExpectedInteger(row);
        if (!value || value>=0x01000000U) return 0;
        unsigned exponent=0;
        for (uint32_t v=value;v>>=1;) exponent++;
        return (Negative(row)?0x80000000U:0U)
            |((exponent+127U)<<23)|((value<<(23U-exponent))&0x007fffffU);
    }
    static bool OutputMatches(unsigned row,uint32_t actualBits) {
        const uint32_t expected=ExpectedBits(row);
        return expected && actualBits==expected;
    }

    // Produce the byte-major Q4_K tile stream expected by the resident reader.
    // Arbitrary chunks allow a page-at-a-time upload without a large buffer.
    static bool FillWeightBytes(unsigned offset,uint8_t* output,unsigned bytes) {
        if (!output || !bytes || offset>WeightBytes || bytes>WeightBytes-offset) return false;
        for (unsigned n=0;n<bytes;n++) {
            const unsigned at=offset+n,tile=at/TileBytes,inTile=at%TileBytes;
            const unsigned group=tile/Blocks,block=tile%Blocks,row=group*16+inTile%16;
            const unsigned field=inTile/16,q=Quant(row,block);
            if (field==1 || field==3) output[n]=0x3cU; // d = dmin = 1
            else if (field>=4 && field<8) output[n]=1U;
            else if (field>=8 && field<12) output[n]=uint8_t(Minimum(field-8));
            else if (field>=12 && field<16) output[n]=uint8_t(1U|(Minimum(field-8)<<4));
            else if (field>=16) {
                const unsigned firstGroup=((field-16)/32)*2;
                const unsigned low=Negative(row)?Minimum(firstGroup)-q:Minimum(firstGroup)+q;
                const unsigned high=Negative(row)?Minimum(firstGroup+1)-q:Minimum(firstGroup+1)+q;
                output[n]=uint8_t(low|(high<<4));
            } else output[n]=0;
        }
        return true;
    }
    static bool FillInputBytes(unsigned offset,uint8_t* output,unsigned bytes) {
        if (!output || !bytes || offset>InputBytes || bytes>InputBytes-offset) return false;
        for (unsigned n=0;n<bytes;n++) {
            const unsigned at=offset+n,index=at/4,block=index/256;
            // 127 * (4^block) remains exact in binary32. Four later blocks
            // use weight 1. Encode the exponent without kernel floating point.
            const uint32_t magnitude=0x42fe0000U+((block<6 ? block*2 : 0)<<23);
            const uint32_t bits=magnitude|((index%16>=9)?0x80000000U:0);
            output[n]=uint8_t(bits>>((at%4)*8));
        }
        return true;
    }
    static bool FillOutputBytes(unsigned offset,uint8_t* output,unsigned bytes) {
        if (!output || !bytes || offset>OutputBytes || bytes>OutputBytes-offset) return false;
        for (unsigned n=0;n<bytes;n++) output[n]=uint8_t(OutputSentinel>>(((offset+n)%4)*8));
        return true;
    }

    // The caller supplies a lease-backed, page-aligned address within one
    // bank and both audited route tables. Build and BindCore remain the same
    // code paths used by the resident projection graph.
    static bool Build(unsigned bank,unsigned address,unsigned leasedBytes,
        const unsigned (&routes)[2][Banks],DataflowProjectionProgram& result,
        PioneerQ35ProjectionProgram& schedule) {
        result={};schedule={};
        if (bank>=Banks || (address&(PageBytes-1)) || address<BankBase || address>=BankLimit
            || leasedBytes<ScratchBytes || ScratchBytes>BankLimit-address) return false;
        TensorSpace weight{};
        weight.type=12;weight.inputs=Inputs;
        weight.place={Rows,Blocks*Q4BlockBytes,1,16,{{bank,address,0,Rows}}};
        RowSpace input{Inputs,4,1,16,{{bank,address+WeightPages*PageBytes,0,Inputs}}};
        RowSpace output{Rows,4,1,16,{{bank,address+(WeightPages+InputPages)*PageBytes,0,Rows}}};
        if (!DataflowProgramBuilder::Build(weight,input,Inputs,output,routes,result,schedule)) return false;
        if (result.activeCores!=DataflowCoreCount || result.groups!=DataflowCoreCount) {
            result={};schedule={};return false;
        }
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            const auto& assignment=result.core[core];
            if (!assignment.active || assignment.groups!=1 || assignment.segmentCount!=1
                || assignment.weight[0].firstGroup!=core || assignment.output[0].firstRow!=core*16) {
                result={};schedule={};return false;
            }
        }
        return true;
    }
};
static_assert(DataflowProjectionQualification::WeightBytes==2764800
    && DataflowProjectionQualification::ScratchPages==680
    && DataflowProjectionQualification::Rows==1920,
    "fixed all-worker Q4_K projection geometry");
}
#endif
