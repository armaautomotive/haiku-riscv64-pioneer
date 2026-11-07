/* SPDX-License-Identifier: MIT */
// Negative source gates only; never opens the device or any real model.
#include "../PioneerNativeBfp4DramModelSource.h"
#include <cassert>
#include <cstdio>
#include <unistd.h>
int main(int argc,char** argv){
 assert(argc==2);const std::string directory=argv[1],path=directory+"/short",link=directory+"/link";
 FILE* f=fopen(path.c_str(),"wb");assert(f && fwrite("short",1,5,f)==5 && !fclose(f));
 assert(!symlink(path.c_str(),link.c_str()));std::string error;
 {PioneerNativeBfp4DramModelSource source;assert(!source.Ready() && !source.Prepare(nullptr,error) && !source.Ready() && !source.Prepare(path.c_str(),error));}
 {PioneerNativeBfp4DramModelSource source;assert(!source.Prepare((directory+"/absent").c_str(),error) && !source.Ready());}
 {PioneerNativeBfp4DramModelSource source;assert(!source.Prepare(link.c_str(),error) && !source.Ready());}
 {PioneerNativeBfp4DramModelSource source;assert(!source.Prepare(path.c_str(),error) && !source.Ready());}
 {const int mode=std::fegetround();assert(!std::fesetround(FE_DOWNWARD));PioneerNativeBfp4DramModelSource source;
  assert(!source.Prepare(path.c_str(),error) && !source.Ready());assert(!std::fesetround(mode));}
 puts("PASS BFP4 source early refusal: one-shot/null, absent, symlink, wrong size and non-default rounding; NO DEVICE");
}
