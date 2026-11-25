/* SPDX-License-Identifier: MIT */
// Offline synthetic fixture and full-head reference for the shard witness.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define main q35_delta_shards_host_test_main
#include "pioneer_qwen35_dataflow_delta_shards_test.cpp"
#undef main
#include "../PioneerQwen35DataflowResidentDispatch.h"
#include <string>

static bool WriteNew(const std::string& path,const void* bytes,size_t size) {
    FILE* out=std::fopen(path.c_str(),"wbx");if (!out) return false;
    const bool wrote=std::fwrite(bytes,1,size,out)==size;
    const bool closed=std::fclose(out)==0;
    if (!wrote || !closed) { std::remove(path.c_str());return false; }
    return true;
}
int main(int argc,char** argv) {
    (void)&q35_delta_shards_host_test_main;
    if (argc!=2) return 2;
    const auto program=Program(1);
    const unsigned routes[2][Banks]={{786,978,1170,785,977,1169,1361},{786,978,1170,785,977,1169,1361}};
    DataflowDynamicTokenContext token{};token.token=42;token.position=1;token.context=32;
    token.tied={Vocabulary,2120,1,16,{{0,0x50000000,0,Vocabulary}}};
    if (!token.Valid()) return 1;
    std::vector<DataflowGraphRunDescriptor> descriptors(120);
    std::vector<DataflowStreamRecord> records(120*3);
    std::array<DataflowBarrierPacket,2> command{};
    for (unsigned core=0;core<120;core++) {
        const unsigned base=0x04000000+core*0x100000;
        const DataflowStreamBinding binding={DataflowStreamBindingMagic,DataflowStreamVersion,
            99,core,0,routes[0][0],base,DataflowGraphDecodeSteps};
        const Range lease={0,base,DataflowGraphDecodeSteps*DataflowStreamRecordBytes};
        auto& descriptor=descriptors[core];
        if (!DataflowGraphRunBuild(binding,lease,routes,token,{5,1,DataflowGraphPromptSteps},
                1,1000000,0,descriptor)) return 1;
        if (!core) {
            DataflowResidentCommand c{};if (!DataflowResidentCommandBuild(descriptor,c)) return 1;
            command={DataflowResidentCommandPayload(c),DataflowResidentCommandCommit(c)};
        }
        DataflowDeltaShardInvocation update{},normalize{};
        if (!DataflowDeltaShardBind(program,core,DataflowDeltaShardUpdate,update)
            || !DataflowDeltaShardBind(program,core,DataflowDeltaShardNormalize,normalize)
            || !DataflowStreamEncode(update,99,1,records[core*3])
            || !DataflowStreamEncode(normalize,99,2,records[core*3+1])) return 1;
    }
    Memory input,expected;IO io{expected};Scratch scratch;
    for (unsigned core=0;core<120;core++) {
        DataflowDeltaInvocation invocation{};
        if (!DataflowDeltaProgramBuilder::BindCore(program,core,invocation)
            || !PioneerQwen35DataflowDeltaWorker(invocation,scratch.query,scratch.key,scratch.value,
                scratch.stateRead,scratch.stateWrite,scratch.scalars,scratch.head,scratch.norm,scratch.gate,io)) return 1;
    }
    std::vector<float> initial;
    auto append=[&](const auto& values) { initial.insert(initial.end(),values.begin(),values.end()); };
    append(input.qkv);append(input.gates);append(input.alpha);append(input.beta);
    append(input.negativeA);append(input.bias);append(input.norm);append(input.oldState);append(input.newState);
    std::vector<float> result=expected.qkv;
    result.insert(result.end(),expected.newState.begin(),expected.newState.end());
    const std::string dir=argv[1];
    const std::array<std::string,5> paths={dir+"/templates.bin",dir+"/command.bin",dir+"/records.bin",
        dir+"/initial.bin",dir+"/expected.bin"};
    const std::array<const void*,5> data={descriptors.data(),command.data(),records.data(),initial.data(),result.data()};
    const std::array<size_t,5> sizes={descriptors.size()*sizeof(descriptors[0]),sizeof(command),
        records.size()*sizeof(records[0]),initial.size()*sizeof(float),result.size()*sizeof(float)};
    unsigned written=0;
    for (;written<paths.size();written++) if (!WriteNew(paths[written],data[written],sizes[written])) break;
    if (written!=paths.size()) { for (unsigned i=0;i<written;i++) std::remove(paths[i].c_str());return 1; }
    std::puts("prepared synthetic delta-shard simulator fixture and full-head reference; NO HARDWARE");
}
