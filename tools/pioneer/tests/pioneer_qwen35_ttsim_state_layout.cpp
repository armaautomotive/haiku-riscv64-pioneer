/* SPDX-License-Identifier: MIT */
// Extract position-zero state destinations from an authenticated recorded
// simulator graph. This describes addresses, not hardware access authority.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowResidentLifecycle.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace PioneerQwen35;

static bool ReadExact(const char* path,void* data,size_t bytes) {
    FILE* input=std::fopen(path,"rb");if (!input) return false;
    const bool okay=std::fread(data,1,bytes,input)==bytes && std::fgetc(input)==EOF && !std::ferror(input);
    return std::fclose(input)==0 && okay;
}
static bool Accept(RowSpace& original,const RowSpace& candidate,bool first) {
    if (!candidate.Valid()) return false;
    if (first) {original=candidate;return true;}
    return DataflowRowSpaceMatches(original,candidate);
}
static bool Emit(char kind,unsigned layer,const RowSpace& space) {
    if (!space.Valid()) return false;
    for (unsigned part=0;part<space.count;part++) {
        const auto& p=space.parts[part];
        if (std::printf("%c %u %u %u %u %u %u\n",kind,layer,space.rowBytes,
                p.bank,p.address,p.firstRow,p.rows)<0) return false;
    }
    return true;
}
int main(int argc,char** argv) {
    if (argc!=3 && argc!=5) {std::fprintf(stderr,"usage: %s templates.bin records.bin [layers executed]\n",argv[0]);return 2;}
    const bool partial=argc==5;unsigned layers=Layers,executed=DataflowGraphDecodeSteps;
    if (partial) {
        char *endLayers=nullptr,*endSteps=nullptr;
        const unsigned long l=std::strtoul(argv[3],&endLayers,10),s=std::strtoul(argv[4],&endSteps,10);
        if (!argv[3][0] || *endLayers || !l || l>=Layers || !argv[4][0] || *endSteps
            || !s || s>=DataflowGraphPromptSteps) return 2;
        layers=unsigned(l);executed=unsigned(s);
    }
    std::vector<DataflowGraphRunDescriptor> descriptors(DataflowCoreCount);
    if (!ReadExact(argv[1],descriptors.data(),descriptors.size()*sizeof(descriptors[0]))) return 1;
    FILE* input=std::fopen(argv[2],"rb");if (!input) return 1;
    std::array<RowSpace,Layers> conv{},delta{},kv{};
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        const auto& descriptor=descriptors[core];
        if (!DataflowGraphRunValid(descriptor,core) || descriptor.token.position!=0
            || descriptor.ticket.count!=(partial?DataflowGraphPromptSteps:DataflowGraphDecodeSteps)) return 1;
        std::array<unsigned,Layers> convCount{},deltaCount{},kvCount{};
        for (unsigned step=0;step<executed;step++) {
            DataflowStreamRecord record{};
            if (std::fread(&record,1,sizeof(record),input)!=sizeof(record)
                || !DataflowStreamRecordStructureValid(record,descriptor.binding.epoch,step+1,core)) return 1;
            if (record.header[5]==DataflowConvMagic) {
                DataflowConvInvocation v{};DataflowDynamicRead(record,v);
                if (!v.Valid() || v.position || v.layer>=Layers || Dense(v.layer)
                    || !Accept(conv[v.layer],v.historyWrite,core==0 && convCount[v.layer]==0)) return 1;
                ++convCount[v.layer];
            } else if (record.header[5]==DataflowDeltaShardMagic) {
                DataflowDeltaShardInvocation v{};DataflowDynamicRead(record,v);
                const unsigned layer=v.program.layer;
                if (!v.Valid() || v.program.position || layer>=Layers || Dense(layer)
                    || !Accept(delta[layer],v.program.stateWrite,core==0 && deltaCount[layer]==0)) return 1;
                ++deltaCount[layer];
            } else if (record.header[5]==DataflowDenseAttentionMagic) {
                DataflowDenseAttentionInvocation v{};DataflowDynamicRead(record,v);
                const unsigned layer=v.program.layer;
                if (!v.Valid() || v.program.position || layer>=Layers || !Dense(layer)
                    || !Accept(kv[layer],v.program.kv,core==0 && kvCount[layer]==0)) return 1;
                ++kvCount[layer];
            }
        }
        if (partial) {
            std::array<unsigned char,sizeof(DataflowStreamRecord)> fault{};
            if (std::fread(fault.data(),1,fault.size(),input)!=fault.size()) return 1;
            for (unsigned char byte:fault) if (byte) return 1;
        }
        for (unsigned layer=0;layer<Layers;layer++) {
            if (layer>=layers) {if (convCount[layer] || deltaCount[layer] || kvCount[layer]) return 1;continue;}
            if (Dense(layer)) {if (convCount[layer] || deltaCount[layer] || kvCount[layer]!=2) return 1;}
            else if (convCount[layer]!=1 || deltaCount[layer]!=2 || kvCount[layer]) return 1;
        }
    }
    if (std::fgetc(input)!=EOF || std::ferror(input) || std::fclose(input)) return 1;
    for (unsigned layer=0;layer<layers;layer++) {
        if (Dense(layer)) {if (!Emit('K',layer,kv[layer])) return 1;}
        else if (!Emit('H',layer,conv[layer]) || !Emit('D',layer,delta[layer])) return 1;
    }
    return 0;
}
