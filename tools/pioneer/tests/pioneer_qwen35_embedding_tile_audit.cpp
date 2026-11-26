/* SPDX-License-Identifier: MIT */
// Independently compare a simulator embedding tile to original GGUF rows.
// Offline diagnostic only; not linked into Work or used on the card.
#include "../PioneerModelWeightCache.h"
#include "ggml.h"
#include "gguf.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static bool SameFile(const struct stat& a,const struct stat& b) {
    if (a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size
        || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime) return false;
#if defined(__APPLE__)
    return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
    return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}

int main(int argc,char** argv) {
    if (argc!=4 || (std::strcmp(argv[3],"42") && std::strcmp(argv[3],"261"))) return 2;
    const unsigned token=!std::strcmp(argv[3],"261")?261:42;
    const int fd=open(argv[1],O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if (fd<0) return 1;
    struct stat before{},after{};PioneerModelWeightCache::Digest digest{};std::string error;
    if (fstat(fd,&before) || !PioneerModelWeightCache::HashFile(fd,2740937888ULL,digest,error)
        || PioneerModelWeightCache::Hex(digest)!="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4") return 1;
    const std::string fdPath="/dev/fd/"+std::to_string(fd);
    gguf_init_params params{true,nullptr};auto* meta=gguf_init_from_file(fdPath.c_str(),params);
    if (!meta) return 1;
    const int64_t index=gguf_find_tensor(meta,"token_embd.weight");
    if (index<0 || gguf_get_tensor_type(meta,index)!=GGML_TYPE_Q6_K) return 1;
    const auto* dims=gguf_get_tensor_ne(meta,index);
    if (dims[0]!=2560 || dims[1]!=248320 || ggml_row_size(GGML_TYPE_Q6_K,2560)!=2100) return 1;
    const uint64_t base=gguf_get_data_offset(meta)+gguf_get_tensor_offset(meta,index);
    FILE* tile=std::fopen(argv[2],"rb");if (!tile) return 1;
    std::array<unsigned char,33920> tiled{};
    const bool tileOkay=std::fread(tiled.data(),1,tiled.size(),tile)==tiled.size()
        && std::fgetc(tile)==EOF && !std::ferror(tile);
    if (std::fclose(tile) || !tileOkay) return 1;
    std::array<unsigned char,2100> row{};
    for (unsigned lane=0;lane<16;lane++) {
        const uint64_t offset=base+uint64_t((token&~15U)+lane)*row.size();
        if (pread(fd,row.data(),row.size(),off_t(offset))!=ssize_t(row.size())) return 1;
        for (unsigned block=0;block<10;block++) for (unsigned byte=0;byte<212;byte++) {
            const unsigned char expected=byte<210?row[block*210+byte]:0;
            if (tiled[block*3392+byte*16+lane]!=expected) return 1;
        }
    }
    const bool stable=!fstat(fd,&after) && SameFile(before,after);
    gguf_free(meta);close(fd);
    if (!stable) return 1;
    std::printf("PASS token %u's 16-row embedding tile matches raw authenticated GGUF; NO HARDWARE\n",token);
}
