/* SPDX-License-Identifier: MIT */
// Read-only artifact authentication tests; isolated temporary fixture only.
#define main PioneerPromptQualityExistingMain
#include "../pioneer_bfp4_prompt_quality.cpp"
#undef main
#include <cassert>
int main(int argc,char** argv){
 assert(argc==2);std::string root=argv[1],path=root+"/sample",link=root+"/link";
 int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);assert(fd>=0 && write(fd,"test",4)==4 && !close(fd));
 const auto pin=PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash("test",4));
 std::vector<uint8_t> out={9};assert(ReadPinned(path.c_str(),4,pin.c_str(),out) && out==std::vector<uint8_t>({'t','e','s','t'}));
 const auto saved=out;assert(!ReadPinned(path.c_str(),3,pin.c_str(),out) && out==saved);
 assert(!ReadPinned(path.c_str(),4,"0000000000000000000000000000000000000000000000000000000000000000",out) && out==saved);
 assert(!symlink(path.c_str(),link.c_str()));assert(!ReadPinned(link.c_str(),4,pin.c_str(),out) && out==saved);
 assert(!chmod(path.c_str(),0644));assert(!ReadPinned(path.c_str(),4,pin.c_str(),out) && out==saved);
 puts("PASS BFP4 source reader: exact bytes, size/hash/symlink/permissions refusal, atomic output; NO MODEL OR DEVICE");
}
