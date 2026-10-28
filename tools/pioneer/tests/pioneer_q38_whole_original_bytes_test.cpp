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
 Q38OriginalTensor missing;missing.name="unchanged";auto saved=missing;
 assert(source.Epoch() && !source.Describe(851,missing) && missing==saved);
 unsigned rowTensors=0;
 for(unsigned i=0;i<851;i++){
  Q38OriginalTensor tensor;std::array<uint8_t,64> row;
  assert(source.Describe(i,tensor) && !tensor.name.empty() && tensor.bytes>=64);
  assert(source.ReadOriginal(i,0,64,row.data(),64) && source.ReadOriginal(i,tensor.bytes-64,64,row.data(),64));rowTensors++;
 }
 assert(!source.ReadOriginal(851,0,64,out.data(),unsigned(out.size())) && out==before);
 assert(!source.ReadOriginal(0,UINT64_MAX,64,out.data(),unsigned(out.size())) && out==before);
 assert(!source.ReadOriginal(0,0,64,out.data(),63) && out==before);
 assert(!source.ReadOriginal(0,0,1048577,out.data(),1048577) && out==before);
 printf("PASS neutral original-row transport/all%u descriptors+first/last rows/invalid index+offset+capacity+size refusals publish NO bytes;no model products\n",rowTensors);
 unsigned parts=0,pages=0,mutableRefusals=0;uint64_t padding=0;
 assert(source.CheckSamples(parts,pages,padding,mutableRefusals) && padding && mutableRefusals && source.Ready() && source.PrepareFork());
 printf("PASS ORIGINAL RAW learned parts%u/first+last+middle pages%u/independent original-row permutation+exact zero padding%llu/all851 tensor layout matched/bounded512KiB RAW cache/mutable+workspace refusals%u publish NO bytes;NOT all-byte tiling/model instructions/products/native/physical/Work/TPS\n",parts,pages,(unsigned long long)padding,mutableRefusals);
 source.Revoke();assert(!source.Ready() && !source.PrepareFork() && !source.Contains(786,0x04000000,64)
  && !source.Read(786,0x04000000,out.data(),64) && out==before && !source.Open(argv[1]));
 assert(!source.Epoch() && !source.Describe(0,missing) && missing==saved && !source.ReadOriginal(0,0,64,out.data(),unsigned(out.size())) && out==before);
 puts("PASS one-way raw-source revocation/no cached byte access/no output change/no reopen;NOT hardware cleanup");
}
