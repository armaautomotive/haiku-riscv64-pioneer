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
    if (argc!=4) return 2;
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
    FILE* records=std::fopen(argv[3],"rb");if (!records) return 1;
    unsigned embedding=0,conv=0,delta=0,dense=0;
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
        // Walk every actual full-model record: the dynamic patch must switch
        // embedding, convolution and recurrent state, and dense position.
        for (unsigned step=0;step<DataflowGraphDecodeSteps;step++) {
            DataflowStreamRecord before{},after{};
            if (std::fread(&before,1,sizeof(before),records)!=sizeof(before)) return 1;
            after=before;
            if (!DataflowDynamicPatch(after,token,installed[core].binding.epoch,step+1,core)
                || !DataflowStreamRecordStructureValid(after,installed[core].binding.epoch,step+1,core)) return 1;
            switch (before.header[5]) {
            case DataflowEmbeddingMagic: {
                DataflowEmbeddingInvocation in{},out{};DataflowDynamicRead(before,in);DataflowDynamicRead(after,out);
                Range selected{};
                if (in.token!=installed[core].token.token || out.token!=command.token
                    || out.lane!=(command.token&15)
                    || !token.tied.Resolve(command.token&~15U,16,0,token.tied.rowBytes*16,selected)
                    || out.weightBank!=selected.bank || out.weightBase!=selected.address) return 1;
                ++embedding;break;
            }
            case DataflowConvMagic: {
                DataflowConvInvocation in{},out{};DataflowDynamicRead(before,in);DataflowDynamicRead(after,out);
                if (in.position!=0 || out.position!=1
                    || !DataflowRowSpaceMatches(in.historyRead,out.historyWrite)
                    || !DataflowRowSpaceMatches(in.historyWrite,out.historyRead)) return 1;
                ++conv;break;
            }
            case DataflowDeltaShardMagic: {
                DataflowDeltaShardInvocation in{},out{};DataflowDynamicRead(before,in);DataflowDynamicRead(after,out);
                if (in.program.position!=0 || out.program.position!=1
                    || !DataflowRowSpaceMatches(in.program.stateRead,out.program.stateWrite)
                    || !DataflowRowSpaceMatches(in.program.stateWrite,out.program.stateRead)) return 1;
                ++delta;break;
            }
            case DataflowDenseAttentionMagic: {
                DataflowDenseAttentionInvocation in{},out{};DataflowDynamicRead(before,in);DataflowDynamicRead(after,out);
                if (in.program.position!=0 || out.program.position!=1) return 1;
                ++dense;break;
            }
            default:
                if (std::memcmp(&before,&after,sizeof(before))) return 1;
            }
        }
    }
    if (std::fgetc(records)!=EOF || std::ferror(records) || std::fclose(records)
        || embedding!=120 || conv!=24*120 || delta!=48*120 || dense!=24*120) return 1;
    std::printf("PASS second decode ticket and %u recorded embedding, %u conv, %u delta and %u dense patches; NO HARDWARE\n",
        embedding,conv,delta,dense);
}
