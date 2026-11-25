/* SPDX-License-Identifier: MIT */
// Build four dependent resident stream records using pinned 4B weights.
// This synthetic layout is for simulator testing only and grants no authority.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#include "../PioneerQwen35DataflowResidentDispatch.h"
#include "../PioneerQwen35DataflowResidualProgram.h"
#include "../PioneerQwen35DataflowRmsProgram.h"
#include "../PioneerQwen35DataflowProjectionPair.h"
#include "../PioneerQwen35DataflowActivationSharePlan.h"
#include "../PioneerQwen35TiledReader.h"
#include "../PioneerModelWeightCache.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace PioneerQwen35;
static bool SameFile(const struct stat& a,const struct stat& b)
{
    if (a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size
        || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime) return false;
#if defined(__APPLE__)
    return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec
        && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
    return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
static bool WriteNew(const std::string& path,const void* data,size_t size)
{
    FILE* out=std::fopen(path.c_str(),"wbx");
    if (!out) return false;
    const bool wrote=std::fwrite(data,1,size,out)==size;
    const bool closed=std::fclose(out)==0;
    if (!wrote || !closed) { std::remove(path.c_str());return false; }
    return true;
}
int main(int argc,char** argv)
{
    if (argc!=3) { std::fprintf(stderr,"usage: %s pinned-model.gguf empty-directory\n",argv[0]);return 2; }
    const int fd=open(argv[1],O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
    if (fd<0) return 1;
    FILE* source=fdopen(fd,"rb");
    if (!source) { close(fd);return 1; }
    struct stat before{},after{};
    PioneerModelWeightCache::Digest digest{};std::string error;
    if (fstat(fd,&before) || !PioneerModelWeightCache::HashFile(fd,PioneerQwen35Gguf::FullBytes,digest,error)
        || PioneerModelWeightCache::Hex(digest)!="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4"
        || fstat(fd,&after) || !SameFile(before,after)) { fclose(source);return 1; }
    PioneerQwen35Gguf sourceModel;PioneerQwen35TiledReader reader;
    if (!sourceModel.Open(source) || !reader.Bind(sourceModel)) { fclose(source);return 1; }
    const auto* tensor=sourceModel.Find("token_embd.weight");
    if (!tensor || tensor->type!=14 || tensor->inputs!=Hidden || tensor->outputs!=Vocabulary) {
        fclose(source);return 1;
    }
    const unsigned index=unsigned(tensor-sourceModel.Tensors().data());
    std::vector<uint8_t> weights(DataflowEmbeddingTileCount*DataflowQ6KEmbeddingTileBytes);
    std::array<uint8_t,4096> page{};
    for (unsigned p=0;p*page.size()<weights.size();p++) {
        if (!reader.ReadPage(sourceModel,index,32,16,p,page.data(),page.size())) { fclose(source);return 1; }
        const size_t count=std::min(page.size(),weights.size()-p*page.size());
        std::memcpy(weights.data()+p*page.size(),page.data(),count);
    }
    const auto* normTensor=sourceModel.Find("blk.0.attn_norm.weight");
    std::vector<uint8_t> norm(Hidden*sizeof(float));
    if (!normTensor || normTensor->type!=0 || normTensor->inputs!=Hidden || normTensor->outputs!=1
        || !sourceModel.ReadPacked(unsigned(normTensor-sourceModel.Tensors().data()),0,
            unsigned(norm.size()),norm.data(),unsigned(norm.size()))) { fclose(source);return 1; }
    const char* names[]={"blk.0.attn_qkv.weight","blk.0.ssm_alpha.weight","blk.0.ssm_beta.weight"};
    const unsigned types[]={13,8,8},rows[]={8192,32,32},tiles[]={2816,576,576};
    const unsigned offsets[]={0,14417920,14512128},lengths[]={14417920,92160,92160};
    std::vector<uint8_t> projection(14604288);
    for (unsigned n=0;n<3;n++) {
        const auto* t=sourceModel.Find(names[n]);
        if (!t || t->type!=types[n] || t->inputs!=Hidden || t->outputs!=rows[n]) { fclose(source);return 1; }
        const unsigned tensorIndex=unsigned(t-sourceModel.Tensors().data());
        for (unsigned p=0;p*page.size()<lengths[n];p++) {
            if (!reader.ReadPage(sourceModel,tensorIndex,0,rows[n],p,page.data(),page.size())) {
                fclose(source);return 1;
            }
            const size_t count=std::min(page.size(),size_t(lengths[n])-p*page.size());
            std::memcpy(projection.data()+offsets[n]+p*page.size(),page.data(),count);
        }
    }
    if (!sourceModel.Stable() || fstat(fd,&after) || !SameFile(before,after)) { fclose(source);return 1; }
    fclose(source);

    const unsigned routes[2][Banks]={{786,978,1170,785,977,1169,1361},{786,978,1170,785,977,1169,1361}};
    DeviceLayout model{};model.magic=LayoutMagic;model.version=LayoutVersion;model.context=32;
    model.tied.type=14;model.tied.inputs=Hidden;
    model.tied.place={Vocabulary,2120,1,16,{{0,0x10000000,0,Vocabulary}}};
    model.activation[0]={Ffn,4,1,16,{{0,0x20000000,0,Ffn}}};
    model.activation[1]={Ffn,4,1,16,{{0,0x20020000,0,Ffn}}};
    model.layers[0].weights[AttentionNorm].type=0;
    model.layers[0].weights[AttentionNorm].inputs=Hidden;
    model.layers[0].weights[AttentionNorm].place={1,Hidden*4,1,1,{{0,0x20030000,0,1}}};
    DataflowDynamicTokenContext token{};DataflowEmbeddingProgram embedding{};
    if (!DataflowDynamicTokenContext::Build(model,42,0,token)
        || !DataflowEmbeddingProgramBuilder::Build(model,42,0,routes,embedding)
        || embedding.weightBank!=0 || embedding.weightBase!=0x10010900 || embedding.lane!=10) return 1;
    DataflowResidualProgram residual{};residual.mode=DataflowResidualSave;residual.activationSlot=0;
    std::memcpy(residual.bankRoute,routes,sizeof(routes));residual.activation=model.activation[0];
    residual.residual={Hidden,4,1,16,{{0,0x20010000,0,Hidden}}};
    if (!residual.Valid()) return 1;
    DataflowRmsProgram rms{};
    if (!DataflowRmsProgramBuilder::Build(model,0,AttentionNorm,0,1,routes,rms)) return 1;
    auto programs=std::make_unique<DataflowProjectionProgram[]>(3);
    const unsigned outputBase[]={0x21000000,0x21010000,0x21020000};
    for (unsigned n=0;n<3;n++) {
        const unsigned blocks=Hidden/(types[n]==8?32:256);
        TensorSpace weight{};weight.type=types[n];weight.inputs=Hidden;
        weight.place={rows[n],blocks*(tiles[n]/16),1,16,{{0,0x28000000+offsets[n],0,rows[n]}}};
        RowSpace output{rows[n],4,1,16,{{0,outputBase[n],0,rows[n]}}};
        PioneerQ35ProjectionProgram schedule{};
        if (!DataflowProgramBuilder::Build(weight,model.activation[1],Hidden,output,routes,
                programs[n],schedule,0,n==0?116:120)) return 1;
    }
    if (programs[0].activeCores!=116 || programs[1].activeCores!=2 || programs[2].activeCores!=2
        || !DataflowProjectionTriple::Compatible(programs[0],programs[1],programs[2])) return 1;
    auto projections=std::make_unique<DataflowCoreInvocation[]>(DataflowCoreCount);
    auto sharing=std::make_unique<DataflowActivationSharePlan>();
    for (unsigned core=0;core<DataflowCoreCount;core++)
        if (!DataflowProjectionTriple::BindCore(programs[0],programs[1],programs[2],core,projections[core])
            || !projections[core].assignment.active || !sharing->Add(projections[core])) return 1;
    if (!sharing->Seal() || sharing->Groups()!=2) return 1;
    for (unsigned core=0;core<DataflowCoreCount;core++)
        if (!sharing->Get(core,projections[core].sharing) || !projections[core].Valid()) return 1;
    std::vector<DataflowGraphRunDescriptor> descriptors(DataflowCoreCount);
    std::vector<DataflowStreamRecord> records(DataflowCoreCount*5);
    std::array<DataflowBarrierPacket,2> command{};
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        const unsigned base=0x04000000+core*0x100000;
        const DataflowStreamBinding binding={DataflowStreamBindingMagic,DataflowStreamVersion,
            99,core,0,routes[0][0],base,DataflowGraphDecodeSteps};
        const Range lease={0,base,DataflowGraphDecodeSteps*DataflowStreamRecordBytes};
        auto& descriptor=descriptors[core];
        if (!DataflowGraphRunBuild(binding,lease,routes,token,{5,1,DataflowGraphPromptSteps},
                1,1000000,0,descriptor)) return 1;
        if (!core) {
            DataflowResidentCommand dynamic{};
            if (!DataflowResidentCommandBuild(descriptor,dynamic)) return 1;
            command={DataflowResidentCommandPayload(dynamic),DataflowResidentCommandCommit(dynamic)};
        }
        DataflowEmbeddingInvocation e{};DataflowResidualInvocation r{};DataflowRmsInvocation n{};
        if (!DataflowEmbeddingProgramBuilder::BindCore(embedding,core,e)
            || !DataflowStreamEncode(e,99,1,records[core*5])
            || !DataflowResidualProgramBuilder::BindCore(residual,core,r)
            || !DataflowStreamEncode(r,99,2,records[core*5+1])
            || !DataflowRmsProgramBuilder::BindCore(rms,core,n)
            || !DataflowStreamEncode(n,99,3,records[core*5+2])
            || !DataflowStreamEncode(projections[core],99,4,records[core*5+3])) return 1;
        // The final zero record deliberately terminates after four barriers.
    }
    const std::string directory=argv[2];
    const std::array<std::string,6> paths={directory+"/templates.bin",directory+"/command.bin",
        directory+"/records.bin",directory+"/weights.bin",directory+"/norm.bin",directory+"/projection.bin"};
    const std::array<const void*,6> data={descriptors.data(),command.data(),records.data(),weights.data(),norm.data(),projection.data()};
    const std::array<size_t,6> size={descriptors.size()*sizeof(descriptors[0]),sizeof(command),
        records.size()*sizeof(records[0]),weights.size(),norm.size(),projection.size()};
    unsigned written=0;
    for (;written<paths.size();written++) if (!WriteNew(paths[written],data[written],size[written])) break;
    if (written!=paths.size()) {
        for (unsigned i=0;i<written;i++) std::remove(paths[i].c_str());
        return 1;
    }
    std::puts("prepared authenticated 4B weights and four synthetic resident graph steps; NO HARDWARE");
    return 0;
}
