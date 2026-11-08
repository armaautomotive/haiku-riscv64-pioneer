/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC 1
#include "pioneer_q38_qgw8_cpu_hidden_snapshot.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

int main(int argc,char** argv){
 using namespace Q38Qgw8CpuHiddenSnapshot;
 assert(argc==2 && argv[1][0]=='/');std::string path=std::string(argv[1])+"/cpu-hidden.bin";
 std::vector<uint32_t> original((Layers+1)*Width,0x3f800000U),converted=original,output={99};
 original[Layers*Width]=0x40000000U;converted[Layers*Width+Width-1]=0xbf000000U;
 assert(Build(original,converted,70253,70253,output) && Shape(output));
 assert(output[Header]==0x40000000U && output[Header+2*Width-1]==0xbf000000U);
 const auto saved=output;
 assert(!Build(original,converted,Vocabulary,0,output) && output==saved);
 assert(!Build(original,converted,0,0,original) && original[Layers*Width]==0x40000000U);
 converted[Layers*Width]=0x7fc00000U;assert(!Build(original,converted,0,0,output) && output==saved);
 converted[Layers*Width]=0x3f800000U;
 auto malformed=output;malformed[9]=1;assert(!Shape(malformed) && !SaveNew(path.c_str(),malformed));
 assert(!SaveNew("relative",output) && SaveNew(path.c_str(),output) && !SaveNew(path.c_str(),output));
 struct stat info={};assert(!stat(path.c_str(),&info) && S_ISREG(info.st_mode) && (info.st_mode&077)==0);
 FILE* input=fopen(path.c_str(),"rb");assert(input);std::vector<uint32_t> read(output.size());
 assert(fread(read.data(),sizeof(uint32_t),read.size(),input)==read.size() && fgetc(input)==EOF && !fclose(input));
 assert(read==output);
 const std::string link=std::string(argv[1])+"/link";assert(!symlink(path.c_str(),link.c_str()));
 assert(!SaveNew(link.c_str(),output));input=fopen(path.c_str(),"rb");assert(input);
 assert(fread(read.data(),sizeof(uint32_t),read.size(),input)==read.size() && !fclose(input) && read==output);
 puts("PASS CPU hidden snapshot data/finite/alias/exclusive write/symlink refusals; NO device or model");
}
