/* SPDX-License-Identifier: MIT */
// Actual arguments and exclusive output writer ONLY;no model/device session.
#include "../PioneerQ38TokenHiddenValidationCLI.h"
#include "../PioneerQ38TokenHiddenSnapshot.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cassert>
#include <memory>
#include <string>
#include "actual_hidden_snapshot_writer.inc"
int main(int argc,char** argv){
 assert(argc==2);const std::string directory=argv[1];
 PioneerQ38TokenHiddenValidationOptions o;o.preflight=true;o.output="sentinel";
 assert(!PioneerQ38TokenHiddenValidationArguments(0,nullptr,o) && o.preflight && !std::strcmp(o.output,"sentinel"));
 const char* a[]={"client","--capture-first-layer","/private/tmp/result.qhd","extra"};
 assert(PioneerQ38TokenHiddenValidationArguments(3,a,o) && !o.preflight && o.output==a[2]);
 const char* p[]={"client","--preflight-source-only",nullptr};
 assert(PioneerQ38TokenHiddenValidationArguments(2,p,o) && o.preflight && !o.output);
 unsigned rejected=1;
 for(const char* flag:{"","--validate-three-positions","--validate-cancel-boundary","--host-timing","--work","--capture-first-layer"}){
  const char* test[]={"client",flag,nullptr};assert(!PioneerQ38TokenHiddenValidationArguments(2,test,o));rejected++;
 }
 for(const char* path:{"","/","relative.qhd","/private/tmp/"}){
  const char* test[]={"client","--capture-first-layer",path};assert(!PioneerQ38TokenHiddenValidationArguments(3,test,o));rejected++;
 }
 for(int count:{0,1,4}){assert(!PioneerQ38TokenHiddenValidationArguments(count,a,o));rejected++;}
 assert(!PioneerQ38TokenHiddenValidationArguments(3,p,o));rejected++;
 auto data=std::make_unique<PioneerQ38TokenHiddenSnapshot>();
 for(unsigned i=0;i<sizeof(*data)/4;i++)reinterpret_cast<uint32_t*>(data.get())[i]=0x3f000000U^i;
 data->words[3]=0x7fc01234U;
 const std::string file=directory+"/new.qhd",link=directory+"/link.qhd",absent=directory+"/missing/result.qhd";
 assert(WriteSnapshotExclusive(file.c_str(),*data));
 struct stat st={};assert(!lstat(file.c_str(),&st) && S_ISREG(st.st_mode) && (st.st_mode&0777)==0600 && st.st_size==sizeof(*data));
 auto readback=std::make_unique<PioneerQ38TokenHiddenSnapshot>();FILE* f=fopen(file.c_str(),"rb");assert(f);
 assert(fread(readback.get(),1,sizeof(*readback),f)==sizeof(*readback) && fgetc(f)==EOF && !fclose(f));
 assert(!std::memcmp(data.get(),readback.get(),sizeof(*data)));
 data->words[0]^=1;assert(!WriteSnapshotExclusive(file.c_str(),*data));
 assert(!symlink(file.c_str(),link.c_str()) && !WriteSnapshotExclusive(link.c_str(),*data));
 assert(!WriteSnapshotExclusive(directory.c_str(),*data) && !WriteSnapshotExclusive(absent.c_str(),*data));
 f=fopen(file.c_str(),"rb");assert(f && fread(readback.get(),1,sizeof(*readback),f)==sizeof(*readback) && !fclose(f));
 data->words[0]^=1;assert(!std::memcmp(data.get(),readback.get(),sizeof(*data)));
 printf("PASS actual DEBUG75 CLI/2 argument positives/%u refusals/no old modes/exclusive0600 full raw snapshot+NaN readback/existing file+symlink+directory+missing parent refused without overwrite;arguments+writer ONLY,no source/device/native/physical/Work/TPS proof\n",rejected);
}
