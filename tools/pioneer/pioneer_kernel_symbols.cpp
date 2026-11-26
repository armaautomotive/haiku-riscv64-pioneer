/* Distributed under the MIT License. Read-only kernel symbol inventory. */
#include <OS.h>
#include <image.h>
#include <elf.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" status_t _kern_read_kernel_image_symbols(image_id, Elf64_Sym*, int32*,
    char*, size_t*, addr_t*);

int main(int argc,char** argv)
{
    const char* wanted="kernel_riscv64";
    if (argc==3 && std::strcmp(argv[1],"--image")==0 && std::strcmp(argv[2],"ecam")==0)
        wanted="ecam";
    else if (argc!=1) {std::fprintf(stderr,"usage: pioneer_kernel_symbols [--image ecam]\n");return 2;}
    image_info info{};int32 cookie=0;image_id kernel=-1;
    while (get_next_image_info(B_SYSTEM_TEAM,&cookie,&info)==B_OK) {
        if (std::strcmp(info.name,wanted)==0) {
            if (kernel>=0) return 2;
            kernel=info.id;
        }
    }
    if (kernel<0) return 2;
    int32 count=0;size_t stringsSize=0;
    if (_kern_read_kernel_image_symbols(kernel,nullptr,&count,nullptr,&stringsSize,nullptr)!=B_OK
        || count<=0 || count>1000000 || !stringsSize || stringsSize>64*1024*1024) return 3;
    auto* symbols=static_cast<Elf64_Sym*>(std::calloc(count,sizeof(Elf64_Sym)));
    auto* strings=static_cast<char*>(std::calloc(stringsSize,1));
    if (!symbols || !strings) {std::free(symbols);std::free(strings);return 4;}
    const int32 capacity=count;const size_t stringCapacity=stringsSize;addr_t delta=0;
    status_t result=_kern_read_kernel_image_symbols(kernel,symbols,&count,strings,&stringsSize,&delta);
    if (result!=B_OK || count>capacity || stringsSize>stringCapacity) {std::free(symbols);std::free(strings);return 5;}
    unsigned written=0;
    for(int32 i=0;i<count;i++) {
        const auto& sym=symbols[i];
        if (!sym.st_name || sym.st_name>=stringsSize || !sym.st_shndx) continue;
        const char* name=strings+sym.st_name;
        size_t remaining=stringsSize-sym.st_name;
        if (!std::memchr(name,0,remaining)) {std::free(symbols);std::free(strings);return 6;}
        bool safe=true;
        for(const unsigned char* p=reinterpret_cast<const unsigned char*>(name);*p;p++)
            if (*p<0x21 || *p>0x7e) {safe=false;break;}
        if (!safe) continue;
        std::printf("%s\t%llx\t%llu\t%u\t%u\n",name,
            static_cast<unsigned long long>(sym.st_value),static_cast<unsigned long long>(sym.st_size),
            unsigned(sym.st_info),unsigned(sym.st_shndx));
        ++written;
    }
    std::free(symbols);std::free(strings);
    return written && !std::ferror(stdout)?0:7;
}
