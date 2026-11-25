/* SPDX-License-Identifier: MIT */
// Build synthetic FFN gate/up data and actual 120-core SiLU invocations for
// a simulator test. The test compares device results with an independent
// mathematical reference; this utility does not run model inference.
#include "../PioneerQwen35DataflowSiluProgram.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace PioneerQwen35;
int main(int argc,char** argv)
{
    if (argc!=2) return 2;
    const unsigned routes[2][Banks]={{786,978,1170,785,977,1169,1361},{786,978,1170,785,977,1169,1361}};
    DataflowSiluProgram program{};
    program.gateSlot=0;
    for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
        program.bankRoute[noc][bank]=routes[noc][bank];
    program.gate={Ffn,4,1,16,{{0,0x04001000,0,Ffn}}};
    program.up={Ffn,4,1,16,{{0,0x04011000,0,Ffn}}};
    if (!program.Valid()) return 1;
    const std::string directory=argv[1];
    std::ofstream invocations(directory+"/silu-invocations.bin",std::ios::binary);
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        DataflowSiluInvocation invocation{};
        if (!DataflowSiluProgramBuilder::BindCore(program,core,invocation)) return 1;
        invocations.write(reinterpret_cast<const char*>(&invocation),sizeof(invocation));
    }
    const float gateValues[]={-4.0f,-1.0f,-0.25f,0.0f,0.25f,1.0f,2.0f,4.0f};
    std::vector<float> gate(Ffn),up(Ffn);
    for (unsigned row=0;row<Ffn;row++) {
        gate[row]=gateValues[row%8];
        up[row]=row%3==0?-2.0f:row%3==1?0.5f:3.0f;
    }
    std::ofstream gates(directory+"/silu-gate.bin",std::ios::binary);
    std::ofstream ups(directory+"/silu-up.bin",std::ios::binary);
    gates.write(reinterpret_cast<const char*>(gate.data()),gate.size()*sizeof(float));
    ups.write(reinterpret_cast<const char*>(up.data()),up.size()*sizeof(float));
    std::printf("invocation_bytes=%zu rows=%u gate=0x04001000 up=0x04011000\n",
        sizeof(DataflowSiluInvocation),Ffn);
    return !invocations || !gates || !ups;
}
