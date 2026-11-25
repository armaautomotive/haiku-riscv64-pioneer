/* SPDX-License-Identifier: MIT */
// Prepare an offline QKV/Alpha/Beta simulator fixture from the authenticated
// 4B GGUF. The actual triple binder and sharing plan assign work to all 120
// workers. This fixture grants no card access or inference authority.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#include "../PioneerQwen35DataflowProjectionPair.h"
#include "../PioneerQwen35DataflowActivationSharePlan.h"
#include "../PioneerQwen35DataflowProjectionQualification.h"
#include "../PioneerQwen35TiledReader.h"
#include "../PioneerModelWeightCache.h"
#include <array>
#include <cstdio>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace PioneerQwen35;
static unsigned PageAlign(unsigned value) { return (value+4095)&~4095U; }

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

static bool WriteNew(const std::string& path,const void* bytes,size_t length)
{
    FILE* out=fopen(path.c_str(),"wbx");
    if (!out) return false;
    const bool wrote=fwrite(bytes,1,length,out)==length;
    const bool closed=fclose(out)==0;
    if (!wrote || !closed) { remove(path.c_str());return false; }
    return true;
}

int main(int argc,char** argv)
{
    if (argc!=3) {
        fprintf(stderr,"usage: %s pinned-model.gguf empty-fixture-directory\n",argv[0]);return 2;
    }
    const int fd=open(argv[1],O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
    if (fd<0) return 1;
    FILE* source=fdopen(fd,"rb");
    if (!source) { close(fd);return 1; }
    struct stat before{},after{};
    PioneerModelWeightCache::Digest hash{};std::string error;
    if (fstat(fd,&before) || !PioneerModelWeightCache::HashFile(fd,PioneerQwen35Gguf::FullBytes,hash,error)
        || PioneerModelWeightCache::Hex(hash)!="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4"
        || fstat(fd,&after) || !SameFile(before,after)) { fclose(source);return 1; }
    PioneerQwen35Gguf model;
    if (!model.Open(source) || fstat(fd,&after) || !SameFile(before,after)) { fclose(source);return 1; }
    PioneerQwen35TiledReader reader;
    if (!reader.Bind(model)) { fclose(source);return 1; }

    const char* names[]={"blk.0.attn_qkv.weight","blk.0.ssm_alpha.weight","blk.0.ssm_beta.weight"};
    const unsigned types[]={13,8,8},rows[]={8192,32,32},tiles[]={2816,576,576};
    unsigned offsets[3]{},lengths[3]{},outputs[3]{},cursor=0;
    for (unsigned i=0;i<3;i++) {
        offsets[i]=cursor;
        lengths[i]=(rows[i]/16)*(2560/(types[i]==8?32:256))*tiles[i];
        cursor=PageAlign(cursor+lengths[i]);
    }
    const unsigned inputOffset=cursor;cursor+=12288;
    for (unsigned i=0;i<3;i++) { outputs[i]=cursor;cursor+=PageAlign(rows[i]*4); }
    if (cursor!=14659584 || inputOffset!=14606336 || outputs[0]!=14618624
        || outputs[1]!=14651392 || outputs[2]!=14655488) { fclose(source);return 1; }
    std::vector<uint8_t> fixture(cursor,0);std::array<uint8_t,4096> page{};
    for (unsigned n=0;n<3;n++) {
        const auto* tensor=model.Find(names[n]);
        if (!tensor || tensor->inputs!=2560 || tensor->outputs!=rows[n] || tensor->type!=types[n]) {
            fclose(source);return 1;
        }
        const unsigned index=unsigned(tensor-model.Tensors().data());
        for (unsigned p=0;p*page.size()<lengths[n];p++) {
            if (!reader.ReadPage(model,index,0,rows[n],p,page.data(),page.size())) { fclose(source);return 1; }
            const unsigned take=std::min(unsigned(page.size()),lengths[n]-unsigned(p*page.size()));
            memcpy(fixture.data()+offsets[n]+p*page.size(),page.data(),take);
        }
        for (unsigned i=0;i<rows[n];i++) {
            fixture[outputs[n]+4*i+2]=0xc0;fixture[outputs[n]+4*i+3]=0x7f;
        }
    }
    if (!DataflowProjectionQualification::FillInputBytes(0,fixture.data()+inputOffset,2560*4)
        || !model.Stable() || fstat(fd,&after) || !SameFile(before,after)) { fclose(source);return 1; }

    constexpr unsigned base=0x4001000;
    const unsigned routes[2][Banks]={{786,978,1170,785,977,1169,1361},{786,978,1170,785,977,1169,1361}};
    RowSpace input{2560,4,1,16,{{0,base+inputOffset,0,2560}}};
    auto programs=std::make_unique<DataflowProjectionProgram[]>(3);
    for (unsigned n=0;n<3;n++) {
        const unsigned blocks=2560/(types[n]==8?32:256);
        TensorSpace weight{};weight.type=types[n];weight.inputs=2560;
        weight.place={rows[n],blocks*(tiles[n]/16),1,16,{{0,base+offsets[n],0,rows[n]}}};
        RowSpace output{rows[n],4,1,16,{{0,base+outputs[n],0,rows[n]}}};
        PioneerQ35ProjectionProgram schedule{};
        if (!DataflowProgramBuilder::Build(weight,input,2560,output,routes,programs[n],schedule,0,n==0?116:120)) {
            fclose(source);return 1;
        }
    }
    if (programs[0].activeCores!=116 || programs[1].activeCores!=2 || programs[2].activeCores!=2
        || !DataflowProjectionTriple::Compatible(programs[0],programs[1],programs[2])) { fclose(source);return 1; }
    auto commands=std::make_unique<DataflowCoreInvocation[]>(120);
    auto sharing=std::make_unique<DataflowActivationSharePlan>();
    for (unsigned core=0;core<120;core++) {
        if (!DataflowProjectionTriple::BindCore(programs[0],programs[1],programs[2],core,commands[core])
            || !commands[core].assignment.active || !sharing->Add(commands[core])) { fclose(source);return 1; }
    }
    if (!sharing->Seal() || sharing->Groups()!=2) { fclose(source);return 1; }
    for (unsigned core=0;core<120;core++)
        if (!sharing->Get(core,commands[core].sharing) || !commands[core].Valid()) { fclose(source);return 1; }

    const std::string directory=argv[2],fixturePath=directory+"/fused-fixture.bin";
    if (!WriteNew(fixturePath,fixture.data(),fixture.size())) { fclose(source);return 1; }
    if (!WriteNew(directory+"/fused-invocations.bin",commands.get(),120*sizeof(commands[0]))) {
        remove(fixturePath.c_str());fclose(source);return 1;
    }
    fclose(source);
    printf("verified 4B: QKV + Alpha + Beta, 116+2+2 workers and two sharing groups; offline fixture only\n");
    return 0;
}
