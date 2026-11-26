/* SPDX-License-Identifier: MIT */
// Offline audit: the final residual result must not alias later output ranges.
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
static bool ReadExact(const char* path,void* data,size_t size) {
    FILE* f=std::fopen(path,"rb");if (!f) return false;
    const bool okay=std::fread(data,1,size,f)==size && std::fgetc(f)==EOF && !std::ferror(f);
    return std::fclose(f)==0 && okay;
}
static bool OutputLayout(const char* path,RowSpace& output) {
    FILE* f=std::fopen(path,"r");if (!f) return false;
    char line[256];output={};output.rows=Ffn;output.rowBytes=4;output.unit=16;
    while (std::fgets(line,sizeof(line),f)) if (line[0]=='O' && line[1]==' ') {
        unsigned bank=0,address=0,first=0,rows=0;char trailing=0;
        if (output.count>=Banks || std::sscanf(line,"O %u %u %u %u %c",&bank,&address,&first,&rows,&trailing)!=4) {
            std::fclose(f);return false;
        }
        output.parts[output.count++]={bank,address,first,rows};
    }
    const bool okay=!std::ferror(f) && output.Valid();return std::fclose(f)==0 && okay;
}
int main(int argc,char** argv) {
    if (argc!=4) return 2;
    std::vector<DataflowGraphRunDescriptor> descriptors(DataflowCoreCount);
    RowSpace expected{};
    if (!ReadExact(argv[1],descriptors.data(),descriptors.size()*sizeof(descriptors[0]))
        || !OutputLayout(argv[3],expected)) return 1;
    FILE* records=std::fopen(argv[2],"rb");if (!records) return 1;
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        DataflowStreamRecord tail[4]{};
        const auto start=(uint64_t(core)*DataflowGraphDecodeSteps+DataflowGraphDecodeSteps-4)*sizeof(tail[0]);
        if (fseeko(records,off_t(start),SEEK_SET) || std::fread(tail,sizeof(tail[0]),4,records)!=4) return 1;
        for (unsigned n=0;n<4;n++)
            if (!DataflowStreamRecordStructureValid(tail[n],descriptors[core].binding.epoch,
                    DataflowGraphDecodeSteps-3+n,core)) return 1;
        if (tail[0].header[5]!=DataflowResidualMagic || tail[1].header[5]!=DataflowRmsMagic
            || tail[2].header[5]!=DataflowProgramMagic || tail[3].header[5]!=DataflowArgmaxMagic) return 1;
        DataflowResidualInvocation residual{};DataflowRmsInvocation norm{};
        DataflowCoreInvocation tied{};DataflowArgmaxInvocation argmax{};
        DataflowDynamicRead(tail[0],residual);DataflowDynamicRead(tail[1],norm);
        DataflowDynamicRead(tail[2],tied);DataflowDynamicRead(tail[3],argmax);
        if (!residual.Valid() || !norm.Valid() || !tied.Valid() || !argmax.Valid()
            || residual.mode!=DataflowResidualAdd || norm.layer!=Layers || norm.role!=FinalNorm
            || residual.activationSlot!=norm.inputSlot || !DataflowRowSpaceMatches(residual.activation,expected)
            || !DataflowRowSpaceMatches(norm.input,expected) || !DataflowRowSpaceMatches(tied.input,norm.output)
            || !DataflowRowSpaceMatches(argmax.program.logits,tied.output)
            || !DataflowGraphWorkspace::Disjoint(expected,norm.output)
            || !DataflowGraphWorkspace::Disjoint(expected,tied.output)
            || !DataflowGraphWorkspace::Disjoint(norm.output,tied.output)) return 1;
    }
    if (std::fclose(records)) return 1;
    std::printf("PASS %u-worker final residual, normalization and logits output ranges are disjoint; RECORDED METADATA ONLY\n",DataflowCoreCount);
}
