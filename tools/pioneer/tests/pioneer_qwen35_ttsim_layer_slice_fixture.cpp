/* SPDX-License-Identifier: MIT */
// Simulator-only focused replay from an authenticated recorded decode graph.
#define main q35_slice_handoff_fixture_main
#include "pioneer_qwen35_ttsim_decode_handoff_verify.cpp"
#undef main
#include <cstdlib>
#include <string>
#include <unistd.h>

static bool EmitSpace(FILE* output,char kind,const RowSpace& space) {
    if (!space.Valid() || space.rows!=Ffn || space.rowBytes!=4 || space.unit!=16) return false;
    for (unsigned i=0;i<space.count;i++) {
        const auto& p=space.parts[i];
        if (std::fprintf(output,"%c %u %u %u %u\n",kind,p.bank,p.address,p.firstRow,p.rows)<0) return false;
    }
    return true;
}
static bool EmitZeroSpace(FILE* output,const RowSpace& space) {
    if (!space.Valid()) return false;
    for (unsigned i=0;i<space.count;i++) {
        const auto& p=space.parts[i];
        const uint64_t bytes=uint64_t(p.rows)*space.rowBytes;
        if (!bytes || bytes>UINT32_MAX || std::fprintf(output,"Z %u %u %u\n",
                p.bank,p.address,unsigned(bytes))<0) return false;
    }
    return true;
}
static bool EmitWarmSpace(FILE* output,char kind,unsigned layer,const RowSpace& space) {
    if (!space.Valid()) return false;
    for (unsigned i=0;i<space.count;i++) {
        const auto& p=space.parts[i];
        if (std::fprintf(output,"%c %u %u %u %u %u %u\n",kind,layer,space.rowBytes,
                p.bank,p.address,p.firstRow,p.rows)<0) return false;
    }
    return true;
}
static bool Mark(const std::string& path) {
    FILE* f=std::fopen(path.c_str(),"wbx");return f && std::fclose(f)==0;
}
int main(int argc,char** argv) {
    (void)&q35_slice_handoff_fixture_main;
    if (argc!=4 && argc!=5) {std::fprintf(stderr,"usage: %s full-fixture empty-output-directory layer [--warm-state]\n",argv[0]);return 2;}
    const bool warm=argc==5 && !std::strcmp(argv[4],"--warm-state");
    if (argc==5 && !warm) return 2;
    char* end=nullptr;const unsigned long parsed=std::strtoul(argv[3],&end,10);
    if (!argv[3][0] || *end || parsed>=Layers) return 2;
    const unsigned layer=unsigned(parsed);const std::string source=argv[1],output=argv[2];
    std::vector<DataflowGraphRunDescriptor> descriptors(DataflowCoreCount);
    if (!ReadExact((source+"/templates.bin").c_str(),descriptors.data(),descriptors.size()*sizeof(descriptors[0]))) return 1;
    FILE* input=std::fopen((source+"/records.bin").c_str(),"rb");if (!input) return 1;
    unsigned start=layer?DataflowGraphDecodeSteps:1,finish=DataflowGraphDecodeSteps,adds=0;
    for (unsigned step=0;step<DataflowGraphDecodeSteps;step++) {
        DataflowStreamRecord record{};
        if (std::fread(&record,1,sizeof(record),input)!=sizeof(record)
            || !DataflowStreamRecordStructureValid(record,descriptors[0].binding.epoch,step+1,0)) return 1;
        if (record.header[5]==DataflowResidualMagic) {
            DataflowResidualInvocation invocation{};DataflowDynamicRead(record,invocation);
            if (!invocation.Valid()) return 1;
            if (invocation.mode==DataflowResidualAdd) {
                ++adds;
                if (layer && adds==2*layer) start=step+1;
                if (adds==2*(layer+1)) {finish=step+1;break;}
            }
        }
    }
    if (start>=finish || finish>DataflowGraphDecodeSteps || finish-start>=DataflowGraphDecodeSteps) return 1;
    const unsigned executed=finish-start;
    FILE* records=std::fopen((output+"/records.bin").c_str(),"wbx");if (!records) return 1;
    RowSpace expectedInput{},expectedOutput{},historyRead{},stateRead{},denseKv{};
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        const uint64_t offset=(uint64_t(core)*DataflowGraphDecodeSteps+start)*sizeof(DataflowStreamRecord);
        if (fseeko(input,off_t(offset),SEEK_SET)) return 1;
        for (unsigned step=0;step<executed;step++) {
            DataflowStreamRecord record{};
            if (std::fread(&record,1,sizeof(record),input)!=sizeof(record)
                || !DataflowStreamRecordStructureValid(record,descriptors[core].binding.epoch,start+step+1,core)) return 1;
            if (!step || step+1==executed) {
                if (record.header[5]!=DataflowResidualMagic) return 1;
                DataflowResidualInvocation invocation{};DataflowDynamicRead(record,invocation);
                if (!invocation.Valid() || invocation.mode!=(!step?DataflowResidualSave:DataflowResidualAdd)) return 1;
                RowSpace& expected=!step?expectedInput:expectedOutput;
                if (!core) expected=invocation.activation;
                else if (!DataflowRowSpaceMatches(expected,invocation.activation)) return 1;
            }
            if (record.header[5]==DataflowConvMagic) {
                DataflowConvInvocation invocation{};DataflowDynamicRead(record,invocation);
                if (!invocation.Valid() || invocation.layer!=layer || invocation.position!=0) return 1;
                const auto& state=warm?invocation.historyWrite:invocation.historyRead;
                if (!core) historyRead=state;
                else if (!DataflowRowSpaceMatches(historyRead,state)) return 1;
            }
            if (record.header[5]==DataflowDeltaShardMagic) {
                DataflowDeltaShardInvocation invocation{};DataflowDynamicRead(record,invocation);
                if (!invocation.Valid() || invocation.program.layer!=layer || invocation.program.position!=0) return 1;
                const auto& state=warm?invocation.program.stateWrite:invocation.program.stateRead;
                if (!core) stateRead=state;
                else if (!DataflowRowSpaceMatches(stateRead,state)) return 1;
            }
            if (warm && record.header[5]==DataflowDenseAttentionMagic) {
                DataflowDenseAttentionInvocation invocation{};DataflowDynamicRead(record,invocation);
                if (!invocation.Valid() || invocation.program.layer!=layer || invocation.program.position!=0
                    || !Dense(layer)) return 1;
                if (!denseKv.count) denseKv=invocation.program.kv;
                else if (!DataflowRowSpaceMatches(denseKv,invocation.program.kv)) return 1;
            }
            record.header[3]=step+1;
            if (!DataflowStreamRecordStructureValid(record,descriptors[core].binding.epoch,step+1,core)
                || std::fwrite(&record,1,sizeof(record),records)!=sizeof(record)) return 1;
        }
        DataflowStreamRecord fault{};
        if (std::fwrite(&fault,1,sizeof(fault),records)!=sizeof(fault)) return 1;
    }
    if (std::fclose(records) || std::fclose(input)) return 1;
    FILE* oldLayout=std::fopen((source+"/layout.txt").c_str(),"r");
    FILE* newLayout=std::fopen((output+"/layout.txt").c_str(),"wx");
    if (!oldLayout || !newLayout || std::fprintf(newLayout,"C %u %u %u 0\n",layer+1,executed,executed+1)<0) return 1;
    char line[256];unsigned weights=0,streams=0;
    while (std::fgets(line,sizeof(line),oldLayout)) {
        if ((line[0]=='W' || line[0]=='S') && line[1]==' ') {
            if (line[0]=='W') ++weights;else ++streams;
            if (std::fputs(line,newLayout)<0) return 1;
        }
    }
    if (std::ferror(oldLayout) || !weights || streams!=DataflowCoreCount
        || !EmitSpace(newLayout,'I',expectedInput) || !EmitSpace(newLayout,'O',expectedOutput)
        || (!warm && (Dense(layer) ? (historyRead.count || stateRead.count)
                         : (!EmitZeroSpace(newLayout,historyRead) || !EmitZeroSpace(newLayout,stateRead)))
            )
        || std::fclose(oldLayout) || std::fclose(newLayout)) return 1;
    if (warm) {
        FILE* state=std::fopen((output+"/warm-state-layout.txt").c_str(),"wx");
        if (!state || (Dense(layer)?!EmitWarmSpace(state,'K',layer,denseKv)
                :(!EmitWarmSpace(state,'H',layer,historyRead) || !EmitWarmSpace(state,'D',layer,stateRead)))
            || std::fclose(state)
            || !Mark(output+"/warm-layer.marker")) return 1;
    }
    for (const char* file:{"weights.bin","templates.bin","command.bin"})
        if (link((source+"/"+file).c_str(),(output+"/"+file).c_str())) return 1;
    if (!Mark(output+"/layer-slice.marker")) return 1;
#ifdef PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY
    if (!Mark(output+"/flash-hybrid.marker")) return 1;
#endif
#ifdef PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY
    if (!Mark(output+"/ffn-pair.marker")) return 1;
#endif
    std::printf("prepared isolated layer %u: original steps %u..%u, %u replay records; SIMULATOR ONLY\n",
        layer,start,finish-1,executed);
}
