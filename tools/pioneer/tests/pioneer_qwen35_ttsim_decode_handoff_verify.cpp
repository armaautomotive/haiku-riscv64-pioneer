/* SPDX-License-Identifier: MIT */
// Cross-check the simulator's dynamic second decode ticket against the actual
// resident protocol and every installed production-recorder descriptor.
// This reads fixtures only; it does not access hardware.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowResidentLifecycle.h"
#include <cstdio>
#include <cstring>
#include <vector>
using namespace PioneerQwen35;

static bool ReadExact(const char* path, void* data, size_t bytes) {
    FILE* f=std::fopen(path,"rb");if (!f) return false;
    const bool okay=std::fread(data,1,bytes,f)==bytes && std::fgetc(f)==EOF && !std::ferror(f);
    return std::fclose(f)==0 && okay;
}
int main(int argc,char** argv) {
    if (argc!=3) return 2;
    std::vector<DataflowGraphRunDescriptor> installed(DataflowCoreCount);
    DataflowBarrierPacket packets[2]{};
    if (!ReadExact(argv[1],installed.data(),installed.size()*sizeof(installed[0]))
        || !ReadExact(argv[2],packets,sizeof(packets))) return 1;
    DataflowResidentCommand command{};
    if (DataflowResidentCommandRead(packets[0].word,packets[1].word,1,5,command)!=DataflowBarrierReady
        || command.trigger!=2 || command.epoch!=6 || command.firstSequence!=525
        || command.count!=DataflowGraphDecodeSteps || command.position!=1
        || command.trigger>DataflowResidentRunLimit
        || command.firstSequence+command.count-1>DataflowResidentRunLimit) return 1;
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        DataflowGraphRunDescriptor expanded{},expected{};
        auto token=installed[core].token;token.token=command.token;token.position=command.position;
        if (!DataflowResidentCommandExpand(command,installed[core],core,1,5,524,expanded)
            || !DataflowGraphRunBuild(installed[core].binding,installed[core].lease,
                installed[core].routes,token,{6,525,DataflowGraphDecodeSteps},2,
                command.pollLimit,command.inputSlot,expected)
            || std::memcmp(&expanded,&expected,sizeof(expected))
            || DataflowResidentCommandExpand(command,installed[core],core,2,5,524,expanded)
            || DataflowResidentCommandExpand(command,installed[core],core,1,6,524,expanded)
            || DataflowResidentCommandExpand(command,installed[core],core,1,5,525,expanded)) return 1;
    }
    std::puts("PASS second decode ticket expanded on all 120 installed descriptors; NO HARDWARE");
}
