/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_whole_original_bytes.h"
#include <array>
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);WholeModelOriginalBytes source;
 std::array<uint8_t,16384> out;out.fill(0x5a);auto before=out;
 assert(!source.Ready() && !source.Read(786,0x04000000,out.data(),64) && out==before);
 assert(source.Open(argv[1]) && source.Ready() && !source.Open(argv[1]));
 puts("AUTHENTICATED entire SAME-open public GGUF SHA/851 descriptors/all canonical weight+state+workspace coordinates;RAW-only,no model or native completion");
 assert(!source.Read(~0U,0x04000000,out.data(),64) && out==before);
 assert(!source.Read(786,0,out.data(),64) && out==before);
 assert(!source.Read(786,0xffffffff,out.data(),64) && out==before);
 assert(!source.Read(786,0x04000000,nullptr,64));
 assert(!source.Read(786,0x04000000,out.data(),16385) && out==before);
 unsigned parts=0,pages=0,mutableRefusals=0;uint64_t padding=0;
 assert(source.CheckSamples(parts,pages,padding,mutableRefusals) && padding && mutableRefusals && source.Ready() && source.PrepareFork());
 printf("PASS ORIGINAL RAW learned parts%u/first+last+middle pages%u/independent original-row permutation+exact zero padding%llu/all851 tensor layout matched/bounded512KiB RAW cache/mutable+workspace refusals%u publish NO bytes;NOT all-byte tiling/model instructions/products/native/physical/Work/TPS\n",parts,pages,(unsigned long long)padding,mutableRefusals);
 source.Revoke();assert(!source.Ready() && !source.PrepareFork() && !source.Contains(786,0x04000000,64)
  && !source.Read(786,0x04000000,out.data(),64) && out==before && !source.Open(argv[1]));
 puts("PASS one-way raw-source revocation/no cached byte access/no output change/no reopen;NOT hardware cleanup");
}
