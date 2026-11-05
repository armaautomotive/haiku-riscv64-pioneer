/* SPDX-License-Identifier: MIT */
// REAL same-open source method;only fstat identity mutations are explicit fixtures.
// NO original mutation,device IO,or substitution of preparation/arithmetic.
#include "../PioneerQwen38Gguf.h"
#include "../PioneerModelWeightCache.h"
#include <cassert>
#include <sys/stat.h>
static unsigned identityFault=0;
static int IdentityStat(int fd,struct stat* out){
 int status=fstat(fd,out);if(status)return status;
 if(identityFault==1)out->st_dev^=1;
 if(identityFault==2)out->st_ino^=1;
 if(identityFault==3)out->st_size--;
 if(identityFault==4)out->st_uid^=1;
 if(identityFault==5)out->st_gid^=1;
 if(identityFault==6)out->st_mode^=0020;
 if(identityFault==7)out->st_nlink++;
 if(identityFault==8)out->st_mtime++;
 if(identityFault==9)out->st_ctime++;
#if defined(__APPLE__)
 if(identityFault==10)out->st_mtimespec.tv_nsec^=1;
 if(identityFault==11)out->st_ctimespec.tv_nsec^=1;
#else
 if(identityFault==10)out->st_mtim.tv_nsec^=1;
 if(identityFault==11)out->st_ctim.tv_nsec^=1;
#endif
 return identityFault==12?-1:0;
}
#define fstat IdentityStat
#include "../PioneerNativeCompactQ4StreamSource.h"
#undef fstat
int main(int argc,char** argv){
 assert(argc==2);PioneerNativeCompactQ4StreamSource source;std::string error;
 assert(!source.Ready() && source.Prepare(argv[1],error) && error.empty() && source.Ready());
 assert(!source.Prepare(argv[1],error) && !error.empty() && source.Ready());
 for(identityFault=1;identityFault<=12;identityFault++)assert(!source.Ready());
 identityFault=0;assert(source.Ready());
 PioneerNativeCompactQ4StreamSource bad;
 assert(!bad.Prepare(nullptr,error) && !bad.Ready() && !bad.Prepare(argv[1],error));
 puts("PASS_PRIVATE_ACTUAL_COMPACT104_SOURCE same-open full original SHA/epoch,all9 full-K compiled compact payloads/inputs/288 rounded predictions match fresh original;one-shot;12 changed-FD-identity/error refusals including nanoseconds;NO original mutation/GPU/quality/speed authority");
}
