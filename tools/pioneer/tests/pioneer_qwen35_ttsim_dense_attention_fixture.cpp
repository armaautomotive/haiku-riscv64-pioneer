/* SPDX-License-Identifier: MIT */
// Synthetic dense attention through the real resident service. No hardware.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define main q35_dense_attention_host_test_main
#include "pioneer_qwen35_dataflow_dense_attention_worker_test.cpp"
#undef main
#include "../PioneerQwen35DataflowResidentDispatch.h"
#include "../PioneerQwen35DataflowResidualProgram.h"
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static bool WriteNew(const std::string& path,const void* data,size_t size) {
    FILE* f=std::fopen(path.c_str(),"wbx");if (!f) return false;
    const bool wrote=std::fwrite(data,1,size,f)==size;
    const bool closed=std::fclose(f)==0;
    if (!wrote || !closed) { std::remove(path.c_str());return false; }
    return true;
}
static bool ParseUnsigned(const char* text,unsigned& value) {
    if (!text || !*text || *text=='-') return false;
    char* end=nullptr;errno=0;const unsigned long parsed=std::strtoul(text,&end,10);
    if (errno || *end || parsed>64) return false;
    value=unsigned(parsed);return true;
}
int main(int argc,char** argv) {
    (void)&q35_dense_attention_host_test_main;
    if (argc!=2 && argc!=4) return 2;
    unsigned position=2,context=4;
    if (argc==4 && (!ParseUnsigned(argv[2],position) || !ParseUnsigned(argv[3],context))) return 2;
    if (!context || context>64 || position>=context) return 2;
    auto program=Program(DataflowDenseAttentionPackKey);
    program.position=position;program.context=context;
    FlatRows(program.qkv,ConvChannels,sizeof(float),0,0x10000000,16);
    FlatRows(program.gate,DeltaInner,sizeof(float),0,0x11000000,16);
    FlatRows(program.contextOutput,Ffn,sizeof(float),0,0x12000000,16);
    FlatRows(program.kv,2*DenseKeyHeads,context*DenseHeadWidth*sizeof(uint16_t),0,0x13000000,1);
    FlatRows(program.queryNorm.place,1,DenseHeadWidth*sizeof(float),0,0x14000000,1);
    FlatRows(program.keyNorm.place,1,DenseHeadWidth*sizeof(float),0,0x15000000,1);
    program.rope={0,0x16000000,context*Rotary*unsigned(sizeof(float))};
    if (!program.Valid()) return 1;
    std::vector<float> qg(ConvChannels),gate(DeltaInner),valueInput(Ffn),queryNorm(DenseHeadWidth),keyNorm(DenseHeadWidth),rope(context*Rotary);
    std::vector<uint16_t> kv(2*DenseKeyHeads*context*DenseHeadWidth);
    std::vector<float> output(Ffn);
    for (unsigned i=0;i<qg.size();i++) qg[i]=Sample(i,1);
    for (unsigned i=0;i<gate.size();i++) gate[i]=Sample(i,2);
    for (unsigned i=0;i<valueInput.size();i++) valueInput[i]=Sample(i,3);
    for (unsigned i=0;i<DenseHeadWidth;i++) { queryNorm[i]=0.75f+float(i%23)*0.002f;keyNorm[i]=0.8f+float(i%19)*0.002f; }
    for (unsigned i=0;i<Rotary;i++) rope[position*Rotary+i]=float(int(i%13)-6)*0.025f;
    for (unsigned head=0;head<DenseKeyHeads;head++) for (unsigned at=0;at<position;at++)
        for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
            kv[(head*context+at)*DenseHeadWidth+lane]=PioneerAttentionToHalf(Sample(lane,head+at+5));
            kv[((head+DenseKeyHeads)*context+at)*DenseHeadWidth+lane]=PioneerAttentionToHalf(Sample(lane,head+at+9));
        }
    const auto initialKv=kv;
    std::vector<DataflowGraphRunDescriptor> descriptors(120);
    std::vector<DataflowStreamRecord> records(120*5);
    std::array<DataflowBarrierPacket,2> command{};
    DataflowDynamicTokenContext token{};token.token=42;token.position=position;token.context=context;
    token.tied={Vocabulary,2120,1,16,{{0,0x50000000,0,Vocabulary}}};
    if (!token.Valid()) return 1;
    for (unsigned core=0;core<120;core++) {
        const unsigned base=0x04000000+core*0x100000;
        const DataflowStreamBinding binding={DataflowStreamBindingMagic,DataflowStreamVersion,
            99,core,0,TestRoutes[0][0],base,DataflowGraphDecodeSteps};
        const Range lease={0,base,DataflowGraphDecodeSteps*DataflowStreamRecordBytes};
        if (!DataflowGraphRunBuild(binding,lease,TestRoutes,token,{5,1,DataflowGraphPromptSteps},
                1,1000000,0,descriptors[core])) return 1;
        if (!core) { DataflowResidentCommand c{};if (!DataflowResidentCommandBuild(descriptors[core],c)) return 1;
            command={DataflowResidentCommandPayload(c),DataflowResidentCommandCommit(c)}; }
        DataflowResidualProgram copy{};copy.mode=DataflowResidualSave;copy.activationSlot=0;
        std::memcpy(copy.bankRoute,TestRoutes,sizeof(TestRoutes));
        FlatRows(copy.activation,Ffn,sizeof(float),0,0x17000000,16);
        FlatRows(copy.residual,Hidden,sizeof(float),0,0x11000000,16);
        DataflowResidualInvocation copyInvocation{};
        if (!DataflowResidualProgramBuilder::BindCore(copy,core,copyInvocation)
            || !DataflowStreamEncode(copyInvocation,99,2,records[core*5+1])) return 1;
        for (unsigned mode=1;mode<=3;mode++) {
            program.mode=mode;DataflowDenseAttentionInvocation inv{};
            const unsigned step=mode==1?1:mode+1;
            if (!DataflowDenseAttentionProgramBuilder::BindCore(program,core,inv)
                || !DataflowStreamEncode(inv,99,step,records[core*5+step-1])) return 1;
        }
    }
    // The host operator reference is separate from the device worker and NoC IO.
    for (unsigned head=0;head<DenseKeyHeads;head++) {
        float key[DenseHeadWidth];uint32_t packed[DenseHeadWidth/2];
        std::memcpy(key,gate.data()+head*DenseHeadWidth,sizeof(key));
        if (!Rms<DenseHeadWidth>(key,keyNorm.data(),key) || !Rotate(key,rope.data()+position*Rotary)
            || !PackHead(key,packed)) return 1;
        std::memcpy(kv.data()+(head*context+position)*DenseHeadWidth,packed,sizeof(packed));
        if (!PackHead(valueInput.data()+head*DenseHeadWidth,packed)) return 1;
        std::memcpy(kv.data()+((head+DenseKeyHeads)*context+position)*DenseHeadWidth,packed,sizeof(packed));
    }
    for (unsigned head=0;head<DenseHeads;head++) {
        float query[DenseHeadWidth];uint32_t key[DenseHeadWidth/2],value[DenseHeadWidth/2];
        const float* qgHead=qg.data()+2*head*DenseHeadWidth;
        std::memcpy(query,qgHead,sizeof(query));
        if (!Rms<DenseHeadWidth>(query,queryNorm.data(),query) || !Rotate(query,rope.data()+position*Rotary)) return 1;
        AttentionRow row{output.data()+head*DenseHeadWidth};
        const unsigned kvHead=head/(DenseHeads/DenseKeyHeads);
        for (unsigned at=0;at<=position;at++) {
            std::memcpy(key,kv.data()+(kvHead*context+at)*DenseHeadWidth,sizeof(key));
            std::memcpy(value,kv.data()+((kvHead+DenseKeyHeads)*context+at)*DenseHeadWidth,sizeof(value));
            if (!row.Add(query,key,value)) return 1;
        }
        if (!row.Finish(qgHead+DenseHeadWidth)) return 1;
    }
    std::vector<uint8_t> initial;
    auto append=[&](const void* data,size_t size) { const auto* p=static_cast<const uint8_t*>(data);initial.insert(initial.end(),p,p+size); };
    append(qg.data(),qg.size()*4);append(gate.data(),gate.size()*4);append(valueInput.data(),valueInput.size()*4);append(queryNorm.data(),queryNorm.size()*4);
    append(keyNorm.data(),keyNorm.size()*4);append(rope.data(),rope.size()*4);append(initialKv.data(),initialKv.size()*2);
    const std::string dir=argv[1];
    const std::array<uint32_t,2> metadata={position,context};
    const std::array<std::string,7> paths={dir+"/templates.bin",dir+"/command.bin",dir+"/records.bin",
        dir+"/initial.bin",dir+"/expected-kv.bin",dir+"/expected-output.bin",dir+"/metadata.bin"};
    const std::array<const void*,7> data={descriptors.data(),command.data(),records.data(),initial.data(),kv.data(),output.data(),metadata.data()};
    const std::array<size_t,7> sizes={descriptors.size()*sizeof(descriptors[0]),sizeof(command),records.size()*sizeof(records[0]),
        initial.size(),kv.size()*2,output.size()*4,sizeof(metadata)};
    unsigned written=0;
    for (;written<paths.size();written++) if (!WriteNew(paths[written],data[written],sizes[written])) break;
    if (written!=paths.size()) {for (unsigned i=0;i<written;i++) std::remove(paths[i].c_str());return 1;}
    std::printf("prepared synthetic dense attention position %u/%u; active workers key/value/query: %u/%u/%u; NO HARDWARE\n",
        position,context,DenseKeyHeads,DenseKeyHeads,DenseHeads);
}
