/* SPDX-License-Identifier: MIT */
// Replace the synthetic Q4_K weights in a simulator fixture with 1,920 rows
// from the authenticated 4B model, using the rewrite's actual tiled reader.
// This prepares an offline fixture; it never opens or commands the card.
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
    if (argc!=3) {
        fprintf(stderr,"usage: %s pinned-model.gguf fixture-directory\n",argv[0]);
        return 2;
    }
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
    const auto* tensor=model.Find("blk.0.ffn_gate.weight");
    if (!tensor || tensor->type!=12 || tensor->inputs!=2560 || tensor->outputs<1920) { fclose(source);return 1; }
    const unsigned tensorIndex=unsigned(tensor-model.Tensors().data());
    PioneerQwen35TiledReader reader;
    if (!reader.Bind(model)) { fclose(source);return 1; }

    const std::string path=std::string(argv[2])+"/fixture.bin";
    FILE* input=fopen(path.c_str(),"rb");
    if (!input) { fclose(source);return 1; }
    std::vector<uint8_t> fixture(2785280);
    const bool inputOkay=fread(fixture.data(),1,fixture.size(),input)==fixture.size()
        && fgetc(input)==EOF && !ferror(input);
    const bool inputClosed=fclose(input)==0;
    if (!inputOkay || !inputClosed) { fclose(source);return 1; }
    std::array<uint8_t,4096> page{};
    for (unsigned i=0;i<675;i++) {
        if (!reader.ReadPage(model,tensorIndex,0,1920,i,page.data(),page.size())) { fclose(source);return 1; }
        memcpy(fixture.data()+i*page.size(),page.data(),page.size());
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
    fclose(source);
    printf("verified 4B model: blk.0.ffn_gate.weight, 1,920 rows x 2,560 inputs; offline fixture only\n");
    return 0;
}
