/* SPDX-License-Identifier: MIT */
// Generate synthetic normalization data and the real 120-core invocations.
#include "../PioneerQwen35DataflowRmsProgram.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace PioneerQwen35;
int main(int argc,char** argv)
{
    if (argc!=2) return 2;
    const unsigned routes[2][Banks]={{786,978,1170,785,977,1169,1361},{786,978,1170,785,977,1169,1361}};
    DataflowRmsProgram program{};
    program.layer=0;program.role=AttentionNorm;program.inputSlot=0;program.outputSlot=1;program.scratchRow=Hidden;
    for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
        program.bankRoute[noc][bank]=routes[noc][bank];
    program.input={Hidden,4,1,16,{{0,0x04001000,0,Hidden}}};
    program.weight={1,Hidden*4,1,1,{{0,0x04011000,0,1}}};
    program.output={Ffn,4,1,16,{{0,0x04021000,0,Ffn}}};
    if (!program.Valid()) return 1;
    const std::string directory=argv[1];
    std::ofstream invocations(directory+"/rms-invocations.bin",std::ios::binary);
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        DataflowRmsInvocation invocation{};
        if (!DataflowRmsProgramBuilder::BindCore(program,core,invocation)) return 1;
        invocations.write(reinterpret_cast<const char*>(&invocation),sizeof(invocation));
    }
    std::vector<float> input(Hidden),weight(Hidden);
    for (unsigned row=0;row<Hidden;row++) {
        input[row]=row%2?-1.0f:1.0f;
        weight[row]=row%3==0?-2.0f:row%3==1?0.5f:3.0f;
    }
    std::ofstream inputs(directory+"/rms-input.bin",std::ios::binary);
    std::ofstream weights(directory+"/rms-weight.bin",std::ios::binary);
    inputs.write(reinterpret_cast<const char*>(input.data()),input.size()*sizeof(float));
    weights.write(reinterpret_cast<const char*>(weight.data()),weight.size()*sizeof(float));
    std::printf("invocation_bytes=%zu hidden=%u input=0x04001000 weight=0x04011000 output=0x04021000\n",
        sizeof(DataflowRmsInvocation),Hidden);
    return !invocations || !inputs || !weights;
}
