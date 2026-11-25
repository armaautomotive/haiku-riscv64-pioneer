/* SPDX-License-Identifier: MIT */
// Generate the existing fixed-projection fixture and real per-core descriptors
// for the Blackhole simulator. This does not perform model inference.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#include "../PioneerQwen35DataflowProjectionQualificationService.h"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace PioneerQwen35;
int main(int argc,char** argv)
{
    if (argc!=2) return 2;
    using F=DataflowProjectionQualification;
    const unsigned routes[2][Banks]={{786,978,1170,785,977,1169,1361},{786,978,1170,785,977,1169,1361}};
    DataflowProjectionProgram program{};PioneerQ35ProjectionProgram schedule{};
    if (!F::Build(0,F::ScratchAddress,F::ScratchBytes,routes,program,schedule)) return 1;
    const std::string directory=argv[1];
    std::ofstream invocations(directory+"/invocations.bin",std::ios::binary);
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        DataflowCoreInvocation original{},shared{};
        if (!DataflowProgramBuilder::BindCore(program,core,original)
            || !DataflowQualificationBindSharing(program,core,original,shared)) return 1;
        invocations.write(reinterpret_cast<const char*>(&shared),sizeof(shared));
    }
    std::vector<uint8_t> scratch(F::ScratchBytes,0);
    if (!F::FillWeightBytes(0,scratch.data(),F::WeightBytes)
        || !F::FillInputBytes(0,scratch.data()+F::WeightPages*F::PageBytes,F::InputBytes)
        || !F::FillOutputBytes(0,scratch.data()+(F::WeightPages+F::InputPages)*F::PageBytes,F::OutputBytes)) return 1;
    std::ofstream fixture(directory+"/fixture.bin",std::ios::binary);
    fixture.write(reinterpret_cast<const char*>(scratch.data()),scratch.size());
    std::ofstream expected(directory+"/expected.bin",std::ios::binary);
    for (unsigned row=0;row<F::Rows;row++) {
        const uint32_t bits=F::ExpectedBits(row);
        expected.write(reinterpret_cast<const char*>(&bits),sizeof(bits));
    }
    std::printf("invocation_bytes=%zu scratch_address=%x scratch_bytes=%u output_offset=%u\n",
        sizeof(DataflowCoreInvocation),F::ScratchAddress,F::ScratchBytes,(F::WeightPages+F::InputPages)*F::PageBytes);
    return !invocations || !fixture || !expected;
}
