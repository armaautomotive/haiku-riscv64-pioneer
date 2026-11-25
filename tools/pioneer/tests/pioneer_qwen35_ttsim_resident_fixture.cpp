/* SPDX-License-Identifier: MIT */
// Synthetic resident graph templates for device-instruction simulation only.
// The bindings do not reserve memory or authorize a real device session.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#include "../PioneerQwen35DataflowResidentDispatch.h"
#include <cstdio>

using namespace PioneerQwen35;
int main(int argc,char** argv)
{
    if (argc!=3) return 2;
    const unsigned routes[2][Banks]={{786,978,1170,785,977,1169,1361},{786,978,1170,785,977,1169,1361}};
    DataflowDynamicTokenContext token{};
    token.token=42;token.position=0;token.context=32;
    token.tied={Vocabulary,2120,1,16,{{0,0x40000000,0,Vocabulary}}};
    if (!token.Valid()) return 1;
    FILE* out=std::fopen(argv[1],"wbx");
    if (!out) return 1;
    DataflowResidentCommand command{};
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        const unsigned base=0x04000000+core*0x100000;
        const DataflowStreamBinding binding={DataflowStreamBindingMagic,DataflowStreamVersion,
            99,core,0,routes[0][0],base,DataflowGraphDecodeSteps};
        const Range lease={0,base,DataflowGraphDecodeSteps*DataflowStreamRecordBytes};
        DataflowGraphRunDescriptor descriptor{};
        if (!DataflowGraphRunBuild(binding,lease,routes,token,{5,1,DataflowGraphPromptSteps},
                1,10000,0,descriptor)
            || (!core && !DataflowResidentCommandBuild(descriptor,command))
            || std::fwrite(&descriptor,1,sizeof(descriptor),out)!=sizeof(descriptor)) {
            std::fclose(out);std::remove(argv[1]);return 1;
        }
    }
    if (std::fclose(out)!=0) { std::remove(argv[1]);return 1; }
    out=std::fopen(argv[2],"wbx");
    if (!out) { std::remove(argv[1]);return 1; }
    const auto payload=DataflowResidentCommandPayload(command),commit=DataflowResidentCommandCommit(command);
    const bool wrote=std::fwrite(&payload,1,sizeof(payload),out)==sizeof(payload)
        && std::fwrite(&commit,1,sizeof(commit),out)==sizeof(commit);
    const bool closed=std::fclose(out)==0;
    if (!wrote || !closed) { std::remove(argv[1]);std::remove(argv[2]);return 1; }
    std::puts("prepared 120 synthetic resident graph templates; NO HARDWARE");
    return 0;
}
