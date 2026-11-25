/* SPDX-License-Identifier: MIT */
// Replace the synthetic weights in a simulator fixture with 1,920 rows from
// the authenticated 4B model, using the rewrite's actual tiled reader.
// This prepares an offline fixture; it never opens or commands the card.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#include "../PioneerQwen35DataflowProjectionQualificationService.h"
#include "../PioneerQwen35TiledReader.h"
#include "../PioneerModelWeightCache.h"
#include <array>
#include <cstdio>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

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

int main(int argc,char** argv)
{
    if (argc!=3 && argc!=4) {
        fprintf(stderr,"usage: %s pinned-model.gguf fixture-directory [q4|q5|q6]\n",argv[0]);
        return 2;
    }
    const char* format=argc==4?argv[3]:"q4";
    const unsigned type=!strcmp(format,"q4")?12:!strcmp(format,"q5")?13:!strcmp(format,"q6")?14:0;
    if (!type) return 2;
    const char* name=type==12?"blk.0.ffn_gate.weight":type==13?"blk.0.attn_qkv.weight":"token_embd.weight";
    const int fd=open(argv[1],O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
    if (fd<0) return 1;
    FILE* source=fdopen(fd,"rb");
    if (!source) { close(fd);return 1; }
    struct stat before{},after{};
    PioneerModelWeightCache::Digest hash{};std::string error;
    if (fstat(fd,&before) || !PioneerModelWeightCache::HashFile(fd,PioneerQwen35Gguf::FullBytes,hash,error)
        || PioneerModelWeightCache::Hex(hash)!="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4"
        || fstat(fd,&after) || !SameFile(before,after)) {
        fprintf(stderr,"model authentication failed: %s\n",error.c_str());fclose(source);return 1;
    }
    PioneerQwen35Gguf model;
    if (!model.Open(source) || fstat(fd,&after) || !SameFile(before,after)) { fclose(source);return 1; }
    const auto* tensor=model.Find(name);
    if (!tensor || tensor->type!=type || tensor->inputs!=2560 || tensor->outputs<1920) { fclose(source);return 1; }
    const unsigned tensorIndex=unsigned(tensor-model.Tensors().data());
    PioneerQwen35TiledReader reader;
    if (!reader.Bind(model)) { fclose(source);return 1; }

    const std::string path=std::string(argv[2])+"/fixture.bin";
    FILE* input=fopen(path.c_str(),"rb");
    if (!input) { fclose(source);return 1; }
    std::vector<uint8_t> synthetic(2785280);
    const bool inputOkay=fread(synthetic.data(),1,synthetic.size(),input)==synthetic.size()
        && fgetc(input)==EOF && !ferror(input);
    const bool inputClosed=fclose(input)==0;
    if (!inputOkay || !inputClosed) { fclose(source);return 1; }
    const unsigned tile=PioneerQ35TileBytes(type),weights=120*10*tile;
    const unsigned inputOffset=(weights+4095)&~4095U,outputOffset=inputOffset+12288;
    std::vector<uint8_t> fixture(outputOffset+8192,0);
    memcpy(fixture.data()+inputOffset,synthetic.data()+2764800,2560*4);
    memcpy(fixture.data()+outputOffset,synthetic.data()+2777088,1920*4);
    std::vector<PioneerQwen35::DataflowCoreInvocation> invocations(120);
    const unsigned routes[2][PioneerQwen35::Banks]={{786,978,1170,785,977,1169,1361},
        {786,978,1170,785,977,1169,1361}};
    constexpr unsigned base=0x4001000;
    PioneerQwen35::TensorSpace weightsSpace{};
    weightsSpace.type=type;weightsSpace.inputs=2560;
    weightsSpace.place={1920,10*(tile/16),1,16,{{0,base,0,1920}}};
    PioneerQwen35::RowSpace inputSpace{2560,4,1,16,{{0,base+inputOffset,0,2560}}};
    PioneerQwen35::RowSpace outputSpace{1920,4,1,16,{{0,base+outputOffset,0,1920}}};
    PioneerQwen35::DataflowProjectionProgram program{};PioneerQ35ProjectionProgram schedule{};
    if (!PioneerQwen35::DataflowProgramBuilder::Build(weightsSpace,inputSpace,2560,outputSpace,
            routes,program,schedule) || program.activeCores!=120) { fclose(source);return 1; }
    for (unsigned core=0;core<120;core++) {
        auto& invocation=invocations[core];
        if (!PioneerQwen35::DataflowProgramBuilder::BindCore(program,core,invocation)) { fclose(source);return 1; }
        for (unsigned worker=0;worker<120;worker++)
            invocation.sharing.members[worker/32]|=uint32_t(1)<<(worker%32);
        invocation.sharing.leader=0;
        invocation.sharing.packing=PioneerQwen35::DataflowActivationSharePacking(type);
        invocation.sharing.inputKey=PioneerQwen35::DataflowActivationShareInputKey(invocation);
        if (!PioneerQwen35::DataflowCoreHeaderValid(invocation)) { fclose(source);return 1; }
    }
    std::array<uint8_t,4096> page{};
    for (unsigned i=0;i*page.size()<weights;i++) {
        if (!reader.ReadPage(model,tensorIndex,0,1920,i,page.data(),page.size())) { fclose(source);return 1; }
        const unsigned take=std::min(unsigned(page.size()),weights-unsigned(i*page.size()));
        memcpy(fixture.data()+i*page.size(),page.data(),take);
    }
    if (!model.Stable() || fstat(fd,&after) || !SameFile(before,after)) { fclose(source);return 1; }
    const std::string temporary=path+".model.tmp";
    FILE* output=fopen(temporary.c_str(),"wbx");
    if (!output) { fclose(source);return 1; }
    const bool wrote=fwrite(fixture.data(),1,fixture.size(),output)==fixture.size();
    const bool closed=fclose(output)==0;
    if (!wrote || !closed || rename(temporary.c_str(),path.c_str())) {
        remove(temporary.c_str());fclose(source);return 1;
    }
    const std::string invPath=std::string(argv[2])+"/invocations.bin";
    const std::string invTemporary=invPath+".model.tmp";
    FILE* inv=fopen(invTemporary.c_str(),"wbx");
    if (!inv) { fclose(source);return 1; }
    const bool invWrote=fwrite(invocations.data(),sizeof(invocations[0]),invocations.size(),inv)==invocations.size();
    const bool invClosed=fclose(inv)==0;
    if (!invWrote || !invClosed || rename(invTemporary.c_str(),invPath.c_str())) {
        remove(invTemporary.c_str());fclose(source);return 1;
    }
    fclose(source);
    printf("verified 4B model: %s, 1,920 rows x 2,560 inputs; offline fixture only\n",name);
    return 0;
}
